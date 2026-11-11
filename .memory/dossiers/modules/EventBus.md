# modules/EventBus

## Invariants & contracts

### [docseed-02bc081e] `app.configuring`: Emitted when entering CONFIGURING state with details about the blocked gate.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-07b81323] **Use `vTaskDelay()`** inside a dedicated FreeRTOS background task (not the main loop) if blocking is unavoidable.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-0811756b] **`EventBus`:** Asynchronous publish/subscribe messaging for cross-module notifications.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-0889d0dd] [Creating a Module](creating-a-module.md) — Step-by-step module tutorial
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-0e15eb83] **Never** use `delay()` on the main core.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-0e77f7f1] **Preferred:** keep the `EventBus` *interface* (for native testability + clean API) but **back it with ESP-IDF `esp_event` on-target** — thread-safe, ISR-safe, already linked, and *already used by the HA module* ([HomeAssistantModule.cpp:332](../modules/HomeAssistant/HomeAssistantModule.cpp)) — and stub it synchronously on native. This fixes the race and the allocation in one move by reusin...
`contract · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-1e2d804b] **`payload`:** A generic `const void*` pointer. Cast this to the expected struct or type.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-1f3df895] **Inactivity Policy:** Displays should dim or turn off after a configurable timeout.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-1fd7824d] [Event Bus](event-bus.md) — Publish-subscribe communication
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-2477f843] `std::atomic` is safe for individual primitive values. For complex state (multi-field structs, buffers), use a FreeRTOS mutex or a thread-safe queue instead.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-440836a5] This is transparent to application code — WiFi remains "connected" but the radio sleeps between beacons.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-6188fed0] [Architecture Overview](architecture-overview.md) — System design and lifecycle
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-68fb673d] `system.heap`: Emitted periodically with the free heap size.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-697efe58] [`EventBus::publish`](../modules/EventBus/EventBus.cpp) has **no lock** and **copies the entire subscriber vector (with `std::string` topics) on every publish**. That is (a) a data race if any task subscribes/publishes concurrently → heap corruption, and (b) a per-event heap allocation on the hot path, contradicting the efficiency mandate.
`contract · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-6fa42eeb] **Returns:** `true` if subscription succeeded; `false` if the subscription table is full or the topic name is invalid.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-841ba210] `app.running`: Published when the system successfully transitions to the RUNNING state.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-8f52f0b1] Use ESP-IDF's **automatic modem sleep** features. When enabled, the ESP32 synchronises with the router's DTIM beacon, turning off the RF module between beacons while maintaining the WiFi connection.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-a4a57c70] **Static function pointers only.** The `EventBus` will never accept `std::function`, lambdas with captures, or `std::bind` results. This is a permanent constraint, not a temporary limitation.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-a6e01d04] **`ctx`:** A user-supplied pointer passed back to your callback function (often used to pass the calling class instance `this`).
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-a9d76537] **Decoupling**: Never directly includes another optional module header. Inter-module communication is conducted via `EventBus` pub/sub topics.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b5fc93f2] **Set a flag** during `begin()` and handle the wait in early `tick()` calls using the standard `currentMillis` comparison pattern.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-bb2ee8a6] [Performance Guide](performance-guide.md) — RAM budgeting, heap management, profiling
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-c722c470] Excessive logging from high-frequency tasks can block the UART and stall other tasks.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-ce2fca64] [Coding Patterns](coding-patterns.md) — Modules vs Libraries, SOLID, anti-patterns
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-d061edec] ISR-context logging should be strictly avoided — use `ESP_EARLY_LOG` or `ESP_DRAM_LOG` if absolutely necessary.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-d6304d47] **Returns:** `true` if the subscription was successfully found and removed; `false` otherwise.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-e283984e] **`topic`:** The exact string topic that was published.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-e711d4f1] Heavy logging from multiple cores simultaneously can cause interleaved output.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-f2114e0f] **Never directly `#include`s another optional module** (decoupling via interfaces and `EventBus`)
`contract · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f4392d59] **Returns:** The number of active subscribers that received the event.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-f645f0e8] As defense-in-depth, you may also pass a max length to the parser (`deserializeJson(doc, body, maxLen)`) or set `DeserializationOption::NestingLimit`.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

## Ruled out, failure signatures and gotchas

### [docseed-bb49f4c5] There is no compiler warning for this. The `registerModule()` call succeeds, `begin()` completes, and the system appears to boot normally — until `tick()` calls a virtual method on a destroyed object, causing a crash minutes or hours later.
`gotcha · kind: semantic · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

## Procedures / Recipes

### [docseed-5709340f] **Wake Events:** Modules should publish `system.display.wake` events to the EventBus when user interaction occurs (button press, web portal command, etc.).
`recipe · kind: procedural · anchors: modules/EventBus/EventBus.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`
