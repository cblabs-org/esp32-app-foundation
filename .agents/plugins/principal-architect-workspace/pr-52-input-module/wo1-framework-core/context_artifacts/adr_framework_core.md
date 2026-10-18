# Architecture Decision Record — WO1 Framework Core

## Decision 1: Member-Level Ring Buffer for EventBus Payloads
- **Context**: `EventBus::publish()` on ESP32 target uses `esp_event_post()` which copies payload pointers across FreeRTOS task boundaries. Passing pointers to stack-local variables results in dangling pointer memory corruption when subscribers read event data asynchronously.
- **Decision**: Implemented `InputEvent _eventRing[EVENT_RING_SIZE]` as a class member ring buffer. When `tick()` receives an event from the FreeRTOS queue, it copies it into `_eventRing[_ringHead]`, passes `&_eventRing[_ringHead]` to `EventBus::publish()`, and increments `_ringHead` modulo `EVENT_RING_SIZE`.

## Decision 2: Static Shim Callback without `std::function` (GRAVE-05)
- **Context**: `iot_button` callbacks fire in `esp_timer` task context. Using `std::function` creates heap allocation overhead and potential lifecycle race condition issues in timer interrupt contexts.
- **Decision**: Implemented plain static callback `_buttonShim(void* handle, void* userData)`. Each button gesture registers a heap-allocated `ShimContext` containing button ID, gesture enum, target FreeRTOS `QueueHandle_t`, and a dropped event counter. The shim safely posts events using non-blocking `xQueueSend(queue, &evt, 0)`.

## Decision 3: Two-Phase ADC Resistor Ladder Aggregation
- **Context**: Multiple `BoardInput` entries sharing the same `adcPin` must share a single `iot_button` ADC driver instance while maintaining distinct voltage thresholds.
- **Decision**: Implemented a two-phase initialization pattern inside `begin(Application& app)`:
  1. Iterate `_pendingInputs`, collecting `InputType::ADC_LADDER` entries grouped by `adcPin` in a `std::map<int8_t, std::vector<const BoardInput*>>`.
  2. Create grouped ADC button handlers for each unique pin.

## Decision 4: Interface Contract — `ITickable` + `IMCPContributor`
- **Context**: `InputModule` is a pure hardware service event engine. It does not require persistent NVS settings or HTTP REST endpoints.
- **Decision**: `InputModule` implements `ITickable` (for queue draining in `tick()`) and `IMCPContributor` (registering the `input_status` JSON-RPC diagnostic tool). It explicitly omits `IConfigurable` and `IWebContributor`.
