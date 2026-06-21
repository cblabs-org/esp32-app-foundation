# Graveyard: Home Assistant Implementation

## Native Test Linking Issues
**Attempt:** Using `#include "modules/HomeAssistant/HomeAssistantModule.hpp"` in `test_homeassistant.cpp` without explicitly including external module dependencies (`EventBus.hpp`, `KeyManager.hpp`, `WifiManager.hpp`).
**Result:** PlatformIO's Library Dependency Finder (LDF) failed to link the external modules into the native test executable, resulting in `Undefined symbols for architecture arm64: "EventBus::subscribe..."`.
**Solution:** Explicitly add `#include "../../../modules/EventBus/EventBus.hpp"` etc. to the bottom of the test file to force the LDF to include them in the build graph.

## Native Test GoogleTest Main
**Attempt:** Wrapping `int main(int argc, char **argv)` in `#ifndef PIO_UNIT_TESTING` inside `test_homeassistant.cpp`.
**Result:** PIO defines `PIO_UNIT_TESTING` for native tests, causing the `main` function to be stripped out, leading to `Undefined symbols for architecture arm64: "_main"`.
**Solution:** Removed the `#ifndef PIO_UNIT_TESTING` guard. The `test_framework = googletest` in `platformio.ini` requires a custom `main` when `lib_extra_dirs` is used or tests are isolated.

## ESP-IDF Header Issues in Native Mode
**Attempt:** Including `<esp_log.h>` and `<esp_mac.h>` unconditionally in `HomeAssistantModule.cpp`.
**Result:** Native builds failed because these ESP-IDF specific headers don't exist on the host.
**Solution:** Wrapped ESP-IDF headers in `#ifndef NATIVE_BUILD` and included `platform/LogShim.h` and `../Logger/Logger.hpp` to ensure `ESP_LOGI` macros function correctly in native environments.
