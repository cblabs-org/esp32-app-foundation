# Architecture Decision Record: KeyManager Native Stubs

## Context
Work Order 6 required removing `#ifdef NATIVE_BUILD` guards from the KeyManager module to allow it to be tested natively, leveraging the provided native stubs.

## Decision
Instead of bypassing KeyManager code during native builds, we extended the native stubs (`test/stubs/platform/PsychicHttp.h` and `test/stubs/platform/LittleFS.h`) to fulfill the module's dependencies:
- **LittleFS**: Added stream-compatible `int read()` and `size_t write(uint8_t)` methods to the `File` stub class to satisfy `ArduinoJson` serialization requirements.
- **PsychicHttp**: Added `hasParam()` and `getParam()` with a stub `PsychicWebParameter` class to the `PsychicRequest` stub.
- **Test Cleanup**: Refactored `KeyManagerTest::SetUp` and `TearDown` to use `LittleFS.remove()` instead of `<stdio.h>` `remove()`, ensuring test cleanup correctly targets the mocked filesystem rather than the host system's root directory (`/etc/keys.json`).
