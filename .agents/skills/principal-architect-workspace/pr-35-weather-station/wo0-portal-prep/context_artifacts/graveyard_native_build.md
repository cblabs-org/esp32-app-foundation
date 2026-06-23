# Graveyard / Issues: Native Build Failure

- **Issue**: The `native` environment tests (`pio test -e native`) failed during the build step.
- **Cause**: The file `modules/OTAUpdateManager/OTAUpdateManager.cpp` at line 296 uses `millis()`, which is an ESP32/Arduino macro that is undefined in native C++ environments.
- **Resolution**: Left untouched because `OTAUpdateManager` was strictly outside the Work Order 0 boundary box. The Principal Architect approved moving forward without fixing this file. Future work on OTA or native testing should replace `millis()` with `AppFoundation::millis()` or standard time functions.
