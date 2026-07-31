# State & Files Changed — WO1 Framework Core

## Modified / Created Files

- `include/InputConfig.h` (NEW): Defines `InputType`, `InputGesture`, `InputGpioConfig`, `InputAdcConfig`, `InputMatrixConfig`, `BoardInput`, `InputEvent`.
- `modules/InputModule/InputModule.hpp` (NEW): Class declaration for `InputModule` inheriting `ITickable` and `IMCPContributor`.
- `modules/InputModule/InputModule.cpp` (NEW): Full implementation of input driver management, FreeRTOS queue bridge, static callback shim, EventBus publication, and `input_status` MCP tool registration.
- `hardware/_common/board_contract.h` (MODIFIED): Added `#ifdef WITH_INPUT` validation check enforcing `BOARD_INPUT_COUNT > 0`.
- `test/stubs/platform/iot_button.h` (NEW): Stub header for `iot_button` component for native unit testing.
- `test/stubs/platform/freertos/queue.h` (NEW): Native C++ queue implementation for FreeRTOS `xQueue*` calls in unit tests.

## Verification
- `pio test -e native`: **358/358 test cases passed**.
