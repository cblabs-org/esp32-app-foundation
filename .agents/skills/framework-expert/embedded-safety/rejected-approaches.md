# Rejected Approaches (Graveyard)

The framework's architecture is shaped by past failures. These designs were tested and explicitly rejected. Do not attempt to implement them.

## 1. Unified `IDisplayDriver` (ADR-02)
- **Concept:** A single interface abstracting U8G2, TFT_eSPI, and e-ink libraries.
- **Why Rejected:** Lowest-common-denominator API restricted advanced features. Different displays require radically different rendering paradigms (e.g., damage rects for e-ink vs full buffers for OLED).

## 2. `std::function` EventBus (GRAVE-05)
- **Concept:** Passing lambdas or `std::function` to the EventBus for callbacks.
- **Why Rejected:** Capturing lambdas exceed the SBO (Small Buffer Optimization) threshold, causing hidden, continuous heap allocations and catastrophic fragmentation. Static function pointers are used instead.
- **Scope Note:** This rejection applies to ALL stored `std::function` callback members, not just EventBus — including entity command callbacks, timer callbacks, and any stored callable that persists beyond a single function scope. See [memory-rules.md § 6](memory-rules.md) for the generalised rule and correct pattern.

## 3. Dynamic Board Factory
- **Concept:** Instantiating `IDisplayBoard` objects on the fly when their display time arrives.
- **Why Rejected:** Violates the "No Heap in Tick" rule. Causes unpredictable latency spikes and OOM crashes.

## 4. Buffered OTA
- **Concept:** Downloading the entire OTA payload to LittleFS before flashing.
- **Why Rejected:** The ESP32's flash size is too small. A 2MB firmware plus a 2MB buffer leaves no room. Streaming decryption directly to the OTA partition is required (ADR-13).

## 5. Monolithic Config
- **Concept:** One giant JSON struct representing the entire application state.
- **Why Rejected:** Creates impossible merge conflicts, coupling between unrelated modules, and requires massive continuous memory allocations to serialize/deserialize. Config is distributed via `IConfigurable`.

## 6. MessagePool as IPC
- **Concept:** Using a complex object-pool system for passing messages between FreeRTOS tasks.
- **Why Rejected:** Over-engineered. `std::atomic` and simple thread-safe queues provided the necessary synchronization with drastically lower overhead.

## 7. LittleFS-only Portal
- **Concept:** Serving all Web Portal HTML/JS directly from LittleFS files without compiled-in fallbacks.
- **Why Rejected:** If LittleFS corrupts, the device cannot be configured to fix it. The core portal UI must be compiled into the firmware as fallback constants.

## 8. String-based Topic Matching
- **Concept:** MQTT-style wildcard matching (`topic/*`) for EventBus.
- **Why Rejected:** Too computationally expensive for the tick loop. Exact string matching or enum-based routing is required.

## 9. Heavy Hub Orchestrator
- **Concept:** A central "Hub" module that polls sensors and pushes data to the display.
- **Why Rejected:** The Orchestrator Anti-Pattern. Creates a God Object. Modules must use Dependency Injection and observe the EventBus instead.

## 10. Unqualified `delay()` Usage

`delay()` blocks the calling core entirely — no other code on that core can execute.

**Tiered Rule:**
1. **Prohibited** in `tick()` — blocks the main loop, triggers TWDT panic after ~5 seconds.
2. **Acceptable** in pre-begin bootstrap — no `tick()` loop exists yet; blocking animations and splash screens are expected here.
3. **Acceptable** in shutdown/reboot paths — the device is about to restart; allowing event propagation or HTTP response flushing is reasonable.
4. **Acceptable** for hardware settling — some peripherals (USB UART, WiFi stack, display controllers) require mandatory post-power-on delays. Use the minimum documented settling time.
5. **Discouraged** in `begin()` for non-hardware reasons — prefer non-blocking initialization or `vTaskDelay()` in a background task where a FreeRTOS scheduler context exists.
