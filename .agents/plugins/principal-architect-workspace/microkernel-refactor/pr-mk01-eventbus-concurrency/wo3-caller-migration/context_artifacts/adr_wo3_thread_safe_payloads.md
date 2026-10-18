# ADR: Thread-Safe Payload Storage & Cross-Task EventBus Concurrency (WO3)

## Context & Problem
Under WO2, `EventBus` on target (ESP32) delegates to ESP-IDF `esp_event` for thread-safe cross-task event publication. When an event is published on target, `esp_event_post` queues the event and returns immediately while the event loop task dispatches subscribers asynchronously.

If a publisher creates an event payload on the stack (such as `OTAProgressEvent` in `OTAUpdateManager::publishProgress()` or `AppConfiguringPayload` in `Application::_publishConfiguringEvent()`) and passes a pointer to that stack variable to `EventBus::publish()`, the stack frame is popped as soon as `publish()` finishes. When `esp_event`'s event task asynchronously executes the subscriber callback, the payload pointer dereferences dangling/freed stack memory.

## Architectural Decision
1. **Persistent Member Storage**:
   Replaced stack-allocated event payloads in `OTAUpdateManager` and `Application` with persistent class member variables:
   - `OTAUpdateManager::_progressEvent` in `OTAUpdateManager.hpp` / `OTAUpdateManager.cpp`
   - `Application::_systemHeapPayload` and `Application::_configuringPayload` in `Application.hpp` / `Application.cpp`
   Since both `OTAUpdateManager` and `Application` instances persist for the lifetime of the application, their member variable addresses remain valid across asynchronous `esp_event` task dispatch boundaries.

2. **Cross-Task Concurrency Verification**:
   Added multi-threaded GoogleTest cases (`CrossTaskPublishSubscribeConcurrency` and `CrossTaskSubscribeUnsubscribeConcurrency`) in `test/test_unit/test_eventbus/test_main.cpp` using `std::thread`, `std::atomic`, and `std::chrono` to verify lock safety, re-entrancy, and thread safety under concurrent cross-task publish/subscribe operations.

## Consequences
- **Target Safety**: Safe asynchronous `esp_event` dispatch on ESP32 without dangling stack pointers.
- **Native Testability**: Preserves zero-allocation synchronous GoogleTest semantics on native builds. All 491 test cases pass.
