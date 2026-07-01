# ADR: NATIVE_BUILD Cleanup

## Context
After migrating hardware dependencies to the include-path priority stub model in PR 41, the `NATIVE_BUILD` preprocessor macro became obsolete.

## Decision
- Removed `-DNATIVE_BUILD` from all `platformio.ini` environment definitions.
- Kept `-DUNIT_TEST` in native test environments since it is actively used in production headers like `AuthManager.hpp` to expose internal test hooks.
- Deleted `NativeBuildStubs.hpp` completely.
- Stripped all `#ifndef NATIVE_BUILD` and `#ifdef NATIVE_BUILD` guards from source and test code.
- Moved the `ESP_LOGI` (and similar) printf shim macros directly into the test stub header `test/stubs/platform/esp_log.h`. Previously, the test code expected `LogShim.h` to conditionally provide these, but without `NATIVE_BUILD` the fallback caused a circular inclusion that failed compilation.
- Refactored all inline comments mentioning `NATIVE_BUILD` to instead refer to "native build" or "native test stubs".
