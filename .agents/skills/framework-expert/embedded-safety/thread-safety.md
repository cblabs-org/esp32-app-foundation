# Thread Safety

The ESP32 is a dual-core processor. The App Foundation heavily leverages FreeRTOS tasks, making thread safety a primary concern.

## FreeRTOS Task Rules

- **Core Affinity:** `app_main` (and the main Arduino `loop()`) runs on Core 1. Background tasks (like `DataManager` fetches) are often pinned to Core 0 (PRO_CPU) or left unpinned.
- **No Blocking in `tick()`:** See [ITickable](../interfaces/tickable.md).
- **Concurrency:** Never access shared global state from multiple tasks without synchronization.

## DataManager Task Context

Network fetches in `DataManager` (implementing [IDataSource](../interfaces/data-source.md)) execute in a separate FreeRTOS task, distinct from the main thread.
- Data fetched and parsed in the fetch task must be safely transferred to the main thread for rendering.

## `std::atomic` Patterns

For simple flags or numerical values passed between threads (e.g., between a fetch task and a display module), use `std::atomic`.

```cpp
std::atomic<int> current_temperature{0};
std::atomic<bool> data_ready{false};
```
This is significantly faster and less prone to deadlocks than FreeRTOS Mutexes for primitive types.

## `WiFiClientSecure` Placement (ADR-14)

`WiFiClientSecure` has a massive memory footprint (~500 bytes object + MbedTLS context buffers).

- **Heap allocation (general use):** Preferred via `std::unique_ptr<WiFiClientSecure>`. Use `new (std::nothrow)` and check for `nullptr`.
- **Stack allocation:** Acceptable ONLY when the FreeRTOS task stack is ≥12KB and verified via `uxTaskGetStackHighWaterMark()`.
- **Class member:** Avoid — bloats the module's static memory footprint permanently, even when TLS is not in use.

## PSRAM / SPI Cache Constraint

During SPI flash write operations (OTA flashing, NVS writes, LittleFS operations), the ESP-IDF disables the SPI cache. This makes PSRAM inaccessible because PSRAM is accessed via the same SPI bus.

- **Rule:** FreeRTOS tasks that perform flash write operations MUST allocate their stack in internal DRAM, not PSRAM.
- **Pattern:** Use `xTaskCreateWithCaps()` with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`.
- **Consequence:** If a task's stack is in PSRAM and it triggers a flash write, the ESP32 will hard-fault because the return address on the stack becomes inaccessible.

## `Serial.print` Constraints

`Serial.print` (and `ESP_LOG` macros) are generally thread-safe in ESP-IDF, but excessive logging from high-frequency tasks or ISRs can block or crash the system.

## ISR (Interrupt Service Routine) Constraints

If you implement hardware interrupts (e.g., for a button press):
- Keep the ISR extremely short.
- Do NOT call blocking functions (no `vTaskDelay`, no I2C/SPI transactions).
- Only use FreeRTOS functions ending in `FromISR` (e.g., `xQueueSendFromISR`).
- Use `IRAM_ATTR` on the ISR function definition to ensure it stays in instruction RAM.

## Task Watchdog Timer (TWDT)

The ESP-IDF Task Watchdog Timer monitors registered tasks and panics the device if any task fails to reset (feed) the watchdog within the configured timeout.

- The main Arduino `loop()` task is automatically subscribed to the TWDT.
- **Default timeout:** Configured via `CONFIG_ESP_TASK_WDT_TIMEOUT_S` in sdkconfig (typically 5 seconds).
- **Consequence:** If `tick()` implementations collectively block the main loop for longer than the TWDT timeout, the device will panic and reboot.
- **Custom tasks:** Long-running FreeRTOS tasks (e.g., OTA download) should add themselves to the TWDT via `esp_task_wdt_add(NULL)` and periodically call `esp_task_wdt_reset()` during long operations.
- **Opt-out:** Tasks that intentionally block for extended periods (e.g., waiting on a queue) may need to be removed from the TWDT via `esp_task_wdt_delete(NULL)`.

## Audit Cross-References

When reviewing thread safety, also check these related concerns:

- **Found a `WiFiClientSecure`?** → Check the task stack size (≥12KB for stack allocation). Check if the task does flash writes (PSRAM/SPI constraint above). See also [TLS Tradeoffs](../security/tls-tradeoffs.md).
- **Found a custom FreeRTOS task?** → Verify it registers with TWDT. Verify its stack size is adequate for its workload. Verify stack memory type (DRAM vs PSRAM) matches its I/O operations.
- **Found a `new` or `malloc` in a task?** → Check if it uses `std::nothrow`. See [Error Handling](../coding-patterns/error-handling.md).
- **Found `Redactor::registerSecret()`?** → Must only be called during single-threaded `setup()` before `begin()` returns. The internal `_secrets` vector has no mutex; late registration races with `redact()` from background tasks.
- **Found `AuditWriter` task or `AUDIT_SYNC()`?** → `AuditWriter` allocates its 4 KB stack in internal DRAM (`MALLOC_CAP_INTERNAL`) because it writes to LittleFS flash. `AUDIT_SYNC()` blocks the caller for up to 5 seconds — never use in tick loops, EventBus callbacks, or the main loop (triggers TWDT). See [Audit Module Guide](../references/audit-module-guide.md) §9.
