# State: Work Order 2 Board Wiring Validation

## Verified Board Configurations

- **`hardware/esp32dev/board.h`**:
  ```cpp
  #ifdef WITH_INPUT
  #include "InputConfig.h"
  constexpr int8_t PIN_MODE_BUTTON = 32;
  #define PIN_MODE_BUTTON PIN_MODE_BUTTON
  constexpr BoardInput BOARD_INPUTS[] = {
      {
          .id       = "mode",
          .type     = InputType::GPIO,
          .gpio     = { .pin = PIN_MODE_BUTTON, .activeLow = true, .powerSave = false }
      },
  };
  constexpr size_t BOARD_INPUT_COUNT = sizeof(BOARD_INPUTS) / sizeof(BOARD_INPUTS[0]);
  #define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
  #endif
  ```

- **`hardware/esp32s3nano/board.h`**:
  ```cpp
  #ifdef WITH_INPUT
  #include "InputConfig.h"
  constexpr int8_t PIN_MODE_BUTTON = 8;
  #define PIN_MODE_BUTTON PIN_MODE_BUTTON
  constexpr BoardInput BOARD_INPUTS[] = {
      {
          .id       = "mode",
          .type     = InputType::GPIO,
          .gpio     = { .pin = PIN_MODE_BUTTON, .activeLow = true, .powerSave = false }
      },
  };
  constexpr size_t BOARD_INPUT_COUNT = sizeof(BOARD_INPUTS) / sizeof(BOARD_INPUTS[0]);
  #define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
  #endif
  ```

- **`hardware/native/board.h`**:
  ```cpp
  #ifdef WITH_INPUT
  #include "InputConfig.h"
  constexpr BoardInput BOARD_INPUTS[] = {};
  constexpr size_t BOARD_INPUT_COUNT = 0;
  #define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
  #endif
  ```

## Verification Log

- `platformio/build_project` (`esp32dev`): `success: true`
- `platformio/build_project` (`esp32s3nano`): `success: true`
- `platformio/run_tests` (`native` sample-app): 15/15 unit tests passed
- `platformio/run_tests` (`native` workspace root): 358/358 unit tests passed
