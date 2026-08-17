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
