# State: Test Coverage & Verification Matrix (WO3)

## Test Targets Summary

Total native GoogleTest cases: **368** (358 pre-existing + 10 new `InputModule` cases).

### `test/test_unit/test_inputModule/test_InputModule.cpp`

| Test Case Name | Description / Validated Behavior |
|----------------|----------------------------------|
| `InputConfigTypesTest.BoardInput_GpioConstruction` | Validates `BoardInput` struct layout with `InputType::GPIO`. |
| `InputConfigTypesTest.BoardInput_AdcLadderConstruction` | Validates `BoardInput` struct layout with `InputType::ADC_LADDER`. |
| `InputConfigTypesTest.BoardInput_MatrixConstruction` | Validates `BoardInput` struct layout with `InputType::MATRIX`. |
| `InputConfigTypesTest.InputEvent_StructHolding` | Validates `InputEvent` struct initialization and gesture values. |
| `InputModuleRegistrationTest.AddMultipleInputs_AndQueryMCPStatus` | Validates `addInput()` accumulation and `"input_status"` MCP tool JSON output. |
| `InputModuleDrainTest.EmptyQueue_TickDoesNothing` | Confirms `tick()` emits zero EventBus publications when queue is empty. |
| `InputModuleDrainTest.SingleEvent_DispatchesToEventBus` | Verifies single event queue receipt, topic generation (`input.click`), and payload matching. |
| `InputModuleDrainTest.Gestures_MapToCorrectTopics` | Verifies `CLICK` -> `input.click`, `DOUBLE_CLICK` -> `input.double_click`, `LONG_PRESS` -> `input.long_press`, `PRESS_DOWN` -> `input.press_down`. |
| `InputModuleDrainTest.DrainCap_RespectedMax4PerTick` | Pushes 6 queue events, confirms first `tick()` drains 4, second `tick()` drains remaining 2. |
| `InputModuleDrainTest.RingBuffer_WrapsCorrectly` | Pushes 10 events across ticks, confirms ring buffer wraps at 8 and `ring_head` equals 2. |

## Modified / Created Source Files

- `hardware/_TEMPLATE.board.h` — added `WITH_INPUT` configuration section
- `modules/InputModule/library.json` — removed unused `Application` library dependency
- `modules/KeyManager/KeyManager.hpp` — corrected `DeviceCrypto.hpp` relative include path
- `test/stubs/platform/freertos/queue.h` — added `g_lastCreatedQueue` tracking
- `test/test_unit/test_inputModule/test_InputModule.cpp` — new GoogleTest suite
