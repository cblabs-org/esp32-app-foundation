# ADR: WifiManager Native Migration

## Problem
The `WifiManager.cpp` file had many `#ifndef NATIVE_BUILD` guards effectively disabling almost all logic in native testing environments. When we removed the guards, the native build failed because the platform stubs (`WiFiClass`, `IPAddress`) did not have the methods used by `WifiManager` (e.g. `persistent()`, `setSleep()`, `scanComplete()`, `encryptionType()`). Also, we had ESP-specific headers (`<ESPmDNS.h>`).

## Decision
1. Removed `<ESPmDNS.h>` entirely as it was not actually used in `WifiManager.cpp` after removing guards.
2. Created a stub for `<esp_wifi.h>` with a no-op `esp_wifi_restore()` method.
3. Added missing constants (`WIFI_OFF`, `WIFI_PS_NONE`, `WIFI_SCAN_FAILED`, etc.) and mock methods (`scanDelete()`, `channel()`, `scanComplete()`, `encryptionType()`) to `test/stubs/platform/WiFi.h` so that `WifiManager.cpp` compiles correctly natively.
4. Added an `operator[]` to the native `IPAddress` stub.

## Consequence
The `WifiManager` logic is now fully tested natively without guards, relying on robust stubs instead of bypassing code. The project now compiles seamlessly in both `native` and `esp32s3nano` targets.
