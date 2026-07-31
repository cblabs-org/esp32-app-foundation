# ADR: Board Wiring & Espressif Button Integration

## Architecture Decisions

1. **Stock Board Descriptors**:
   - `hardware/esp32dev/board.h`: Wired GPIO 32 as `PIN_MODE_BUTTON` (`InputType::GPIO`, active low).
   - `hardware/esp32s3nano/board.h`: Wired GPIO 8 as `PIN_MODE_BUTTON` (`InputType::GPIO`, active low).
   - `hardware/native/board.h`: Stubbed `BOARD_INPUTS[]` with `BOARD_INPUT_COUNT = 0` (native target does not define `WITH_INPUT`).

2. **Sample Application Integration**:
   - `sample-app/src/main.cpp`: Guarded under `#ifdef WITH_INPUT`. Instantiates static `InputModule inputModule`. Populates descriptors via `addInput()`, calls `inputModule.begin(app)` before `app.registerModule(&inputModule)`.
   - Wired `"input.click"` EventBus topic to `displayModule.carousel.nextBoard(millis())` and `displayModule.resetActivity()`.

3. **Build Configuration & IDF Component Manager**:
   - `sample-app/platformio.ini`: Added `-D WITH_INPUT` to `esp32dev` and `esp32s3nano` hardware environment `build_flags`. Added `InputModule` to `lib_deps`.
   - `sample-app/src/idf_component.yml`: Created manifest with dependency `espressif/button: "^4.2.0"`.
   - `modules/InputModule/library.json`: Created PlatformIO library descriptor with `Application`, `ArduinoJson`, and `PsychicHttp` dependencies to ensure clean LDF resolution across dual-framework builds.
