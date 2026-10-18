# ADR — WO4 Flag Coordination & App Templates (`pr-mk05-extract-optionals`)

## Context
With the extraction of MCP (WO1), OTA (WO2), and Time (WO3) into standalone optional modules, Work Order 4 coordinates and verifies the compilation of `sample-app` and `template-app` across the flag matrix (`WITH_MCP`, `WITH_OTA`, `WITH_TIME`).

## Architectural Decisions

1. **Display Module Integration in `sample-app`**:
   - `sample-app/src/main.cpp` was updated to remove legacy `app.getDisplay()` calls, replaced by the extracted `displayModule` bridge pattern consistent with `template-app`.
   - `displayModule.carousel.add(&weatherBoard)` and `displayModule.carousel.add(&diagnosticsBoard)` manually add boards to the display carousel.

2. **HomeAssistant Guarding in `WeatherModule`**:
   - `_tempSensor`, `_windSensor`, and `_condSensor` member initialization in `sample-app/src/WeatherModule.cpp` constructor was wrapped with `#ifdef WITH_HOMEASSISTANT` to prevent compilation errors when building without HomeAssistant enabled.

3. **Explicit Environment Flag Matrix**:
   - `sample-app/platformio.ini` and `template-app/platformio.ini` both define `esp32dev_all_on` (`-DWITH_MCP -DWITH_OTA -DWITH_TIME`) and `esp32dev_all_off` (`-UWITH_TIME`) environments for automated CI/MCP matrix builds.

4. **Payoff Validation**:
   - Disabling optional modules (`WITH_MCP`, `WITH_OTA`, `WITH_TIME`) in `template-app` reduces flash consumption by **105,524 bytes (-8.47%)** and RAM by **7,904 bytes (-16.68%)**.
