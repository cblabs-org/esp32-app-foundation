# Graveyard: WO1 Kernel Tests

This document catalogs failed attempts and dead ends encountered during the implementation of WO1 to prevent future agents from repeating them.

### `test_timeManager` Linker Errors (Missing `_main`)
- **Attempt**: Appended `#if defined(NATIVE_BUILD)` wrapping a `main` function using a simple `cat EOF >>` operation.
- **Failure**: Led to `ld: symbol(s) not found for architecture arm64: "_main"` because of cached `.pio/build/native` artifacts conflicting with the new `main` definition and older object files compiled as `x86_64`.
- **Resolution**: A full `pio run -t clean -e native` must be executed whenever core test entry points or preprocessor guards are fundamentally changed to clear the PIO object cache.

### Duplicate `AsyncWebServer` Mocks
- **Attempt**: Defined a custom inline mock `class AsyncWebServer {};` at the top of `test_otaUpdateManager.cpp` to satisfy the `registerRoutes(AsyncWebServer&)` signature.
- **Failure**: This created an ODR (One Definition Rule) violation / conflicting mock since `modules/webServer/webServer.hpp` already defines a native mock for `AsyncWebServer`. This caused `test_webServer` to fail downstream when executing its `lastMaxBodySize` checks.
- **Resolution**: Removed the inline mock and instead `#include "../../../modules/webServer/webServer.hpp"` to leverage the canonical project-wide mock.

### Pre-existing `test_webServer` Failures
- **Observation**: `test_webServer` consistently fails in `NATIVE_BUILD` with an assertion on `AsyncWebServer::lastMaxBodySize == 4096`. 
- **Cause**: The production code in `webServer.cpp` has `AsyncWebServer::setMaxBodySize(4096);` explicitly commented out. 
- **Action**: Left untouched as it falls entirely outside the WO1 boundary box. Future agents should not attempt to "fix" this test failure under WO1 execution constraints.
