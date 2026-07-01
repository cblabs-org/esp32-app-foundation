# Graveyard: Millis Redefinition Issues

During the stub expansion, we added an implementation of `inline unsigned long millis()` to `test/stubs/platform/Arduino.h` to satisfy the newly activated `WifiManager.cpp` code.

## Dead End
This caused multiple "redefinition of 'millis'" linker and compilation errors because:
1. `test/stubs/platform/esp_chip_info.h` already defined `millis()`.
2. `test/stubs/platform/freertos/idf_additions.h` already defined `millis()`.
3. `modules/Application/Application.cpp` had a `#ifdef NATIVE_BUILD static unsigned long millis()` stub.
4. `modules/OTAUpdateManager/OTAUpdateManager.cpp` had a weak stub implementation.

## Resolution
We centralized `millis()` strictly inside `test/stubs/platform/Arduino.h`. All other ad-hoc stub definitions inside individual module files (`Application.cpp`, `OTAUpdateManager.cpp`) and other non-Arduino stub headers (`esp_chip_info.h`, `idf_additions.h`) were completely removed.
