# PSRAM Utilisation

Pseudo-Static RAM (PSRAM) extends the ESP32's available memory, but comes with critical performance and safety caveats.

## Hardware Differences

- **ESP32 (Classic):** Typically has no built-in PSRAM. External chips can be added but require specific pin configurations.
- **ESP32-S3:** Frequently includes 2MB or 8MB of PSRAM connected via Octal SPI (OPI).

## Memory Allocation

To explicitly allocate memory in PSRAM:

```cpp
void* buffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
```

If PSRAM is unavailable, this returns `nullptr`. Always check the return value.

## When to use PSRAM

**Do use PSRAM for:**
- Large display buffers (e.g., e-ink or high-res TFT framebuffers).
- Large JSON parsing payloads.
- Audio buffers.

**Do NOT use PSRAM for:**
- ISR (Interrupt Service Routine) context variables. PSRAM is not accessible during certain cache disabling operations (e.g., flash writes).
- High-frequency data access (e.g., tight DSP loops).
- `ITickable` core logic that requires absolute deterministic timing.

## Stack in PSRAM & DeepStackExecutor

For heavy operations requiring deep call stacks (e.g. image/PNG compression), task stacks can be offloaded to PSRAM:
- Requires `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y` in Kconfig/sdkconfig.
- Use the framework's [`DeepStackExecutor`](../../../../lib/DeepStackExecutor/DeepStackExecutor.h) service, which spawns a persistent worker using `xTaskCreateWithCaps(..., MALLOC_CAP_SPIRAM)`.
- > [!CAUTION]
  > **Flash Safety Rule:** The SPI flash cache controller is disabled during flash writes (e.g. OTA firmware updates). **Never** run a task with a PSRAM stack while an OTA update or flash write is active. Tasks that write flash (like `httpd` or `OTAUpdateManager`) MUST maintain their stack in internal DRAM (`MALLOC_CAP_INTERNAL`). See [Web Server Stack & DRAM Tuning](../systems/web-portal/dram-tuning.md).

## Speed Tradeoffs

- PSRAM operates over SPI/OPI. It is inherently slower than internal SRAM.
- On ESP32-S3, Octal SPI at 80MHz provides high throughput but still suffers higher latency than internal memory.

## Consumer Warning: `LV_CONF_PATH` Override

> [!WARNING]
> **If your project sets `-DLV_CONF_PATH` to its own `lv_conf.h`, the framework's
> LVGL memory configuration does not apply to your build.** Framework updates to
> `modules/LVGLDisplay/lv_conf.h` — including the PSRAM pool placement — will
> never reach you. You must mirror them in your own file.

In particular, ensure `LV_MEM_SIZE` is PSRAM-backed via `LV_MEM_POOL_ALLOC`; on a
PSRAM board an internal-DRAM LVGL pool is likely to be your **single largest DRAM
consumer** (measured: 128 KB on one consumer project, reducing `heap_min_free` to
1,004 bytes).

### Diagnostic Recipe — Finding Internal-DRAM Hogs

```bash
xtensa-esp32s3-elf-nm -C --size-sort -r -S firmware.elf | grep -iE "^3f[cd]"
```

Internal DRAM lives at `0x3FC…`/`0x3FD…`; `0x3C…` is flash-mapped rodata and costs
no RAM. The naive `nm` output is dominated by images and fonts that cost nothing —
this filter isolates the real DRAM consumers.

### After Bumping the Submodule

After upgrading the framework submodule, always check whether any `lv_conf.h` change
needs mirroring into your own override. The changelog and PR description will note
LVGL memory changes, but the `LV_CONF_PATH` override makes them invisible to your
build unless you manually apply them.

## PSRAM-Preference Audit

When reviewing or building any module on a PSRAM-equipped board, apply this checklist.
The principle is simple: **internal DRAM is scarce and non-expandable — use PSRAM for
everything that can tolerate its slightly higher latency, and reserve DRAM for the
things that physically require it.**

### Mandatory PSRAM Candidates
- [ ] **Large buffers (>4 KB)** — image buffers, JSON documents, TLS scratch,
      compression workspace → allocate with `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`
- [ ] **LVGL widget heap** — `LV_MEM_POOL_ALLOC` must point to PSRAM
      (see LV_CONF_PATH warning above)
- [ ] **Task stacks for non-flash-critical tasks** — use
      `xTaskCreateWithCaps(..., MALLOC_CAP_SPIRAM)` for worker tasks that
      don't touch SPI flash (e.g. DeepStackExecutor, data processing)
- [ ] **String/JSON accumulation buffers** — ArduinoJson `JsonDocument`,
      `String` concatenation for large payloads

### Must Stay in Internal DRAM
- [ ] **httpd task stack** — OTA direct-upload and LittleFS writes execute here
- [ ] **OTA worker task stack** — flash write routines
- [ ] **DMA buffers** — SPI, I2S, LCD DMA descriptors
- [ ] **ISR-accessible data** — anything touched from an interrupt context
- [ ] **lwIP pbufs** — TCP/IP stack requires internal DRAM

### Red Flags (Audit Triggers)
- `malloc()` or `new` for buffers >4 KB without `MALLOC_CAP_SPIRAM`
- `LV_MEM_SIZE` set to >64 KB without `LV_MEM_POOL_ALLOC`
- Task stacks >8 KB in internal DRAM for non-flash-critical work
- Static arrays >4 KB in `.bss` or `.data` segments
- Any module allocating >10% of available internal DRAM at init
