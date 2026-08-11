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

## Stack in PSRAM

For large FreeRTOS tasks that don't fit in internal RAM, the stack can be placed in PSRAM:
- Use `xTaskCreateStatic()` or explicitly configure FreeRTOS to allocate tasks from SPIRAM if enabled in `sdkconfig`.
- **Warning:** A stack in PSRAM is significantly slower, increasing execution time for function calls and local variable access.

## Speed Tradeoffs

- PSRAM operates over SPI/OPI. It is inherently slower than internal SRAM.
- On ESP32-S3, Octal SPI at 80MHz provides high throughput but still suffers higher latency than internal memory.
