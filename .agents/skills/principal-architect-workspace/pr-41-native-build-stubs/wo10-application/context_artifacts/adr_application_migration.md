# ADR: Application Module Migration to Native Build Stubs

## Context
`Application.cpp` and `Application.hpp` are the composition root for the ESP32 App Foundation framework. They previously relied heavily on `#ifdef NATIVE_BUILD` guards to mock out hardware interactions during native GoogleTest execution. This led to fragmented, unreadable code and hindered testing fidelity since hardware and test builds were executing fundamentally different conditional paths.

## Decision
We migrated `Application` to run the exact same logic paths for both hardware and native environments. Instead of conditional compilation branches, we now rely entirely on link-time stubs placed in `test/stubs/platform/`. 
- `esp_heap_caps.h` handles heap status mocking.
- `inttypes.h` was added for `PRIu32` support since the native compiler doesn't provide it automatically without ESP-IDF.
- Synchronous calls like `_dataManager.processNext()` now execute unconditionally.
- `LittleFS.begin()` continues to attempt mounting, with test stubs correctly imitating the hardware initialization behavior.

## Consequences
- `Application` source files are now cleaner and easier to read.
- We achieve greater test coverage on the exact paths that run in production.
- Future dependencies added to `Application` will need stub equivalents in the test environment.
