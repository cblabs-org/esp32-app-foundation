# Graveyard: Espressif Button v4.2.0 API Incompatibilities

## Issue 1: `iot_button` C API Signature Mismatches

- **Symptom**: Compilation failed in `InputModule.cpp` with `too few arguments to function 'esp_err_t iot_button_create(...)'` and `cannot convert 'void (*)(void*, void*)' to 'button_event_args_t*'`.
- **Root Cause**: `espressif/button` 4.2.0 introduced individual device creators (`iot_button_new_gpio_device`, `iot_button_new_adc_device`, `iot_button_new_matrix_device`) and updated `iot_button_register_cb` signature to 5 arguments: `(btn_handle, event, event_args, cb, usr_data)`.
- **Resolution**: Updated `InputModule.cpp` to use the 4.x creator functions and pass `nullptr` for `event_args` in `iot_button_register_cb`. Updated native stub `test/stubs/platform/iot_button.h` to match the 5-parameter callback signature.

## Issue 2: Missing Header Includes for Device Types

- **Symptom**: `button_gpio_config_t` and `BUTTON_TYPE_GPIO` undeclared in `InputModule.cpp`.
- **Root Cause**: In `espressif/button` 4.x, device-specific configurations are split into `#include "button_gpio.h"`, `#include "button_adc.h"`, and `#include "button_matrix.h"`.
- **Resolution**: Added device header includes guarded by `#ifndef UNIT_TEST`.

## Issue 3: Missing PlatformIO LDF Dependencies

- **Symptom**: `fatal error: ArduinoJson.h: No such file or directory` and `fatal error: PsychicHttp.h: No such file or directory` when building `InputModule.cpp`.
- **Root Cause**: `InputModule` did not have a `library.json` file in `modules/InputModule/`, causing PlatformIO's Library Dependency Finder (LDF) to omit transitive include directories when compiling `InputModule` as a separate library.
- **Resolution**: Created `modules/InputModule/library.json` declaring dependencies on `Application`, `ArduinoJson`, and `PsychicHttp`.
