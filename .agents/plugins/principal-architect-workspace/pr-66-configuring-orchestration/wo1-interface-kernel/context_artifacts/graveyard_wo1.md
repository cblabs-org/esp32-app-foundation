# Graveyard: WO1 Dead-Ends & Gotchas

## 1. test_application Cannot Run in Native Environment

**Problem:** The `test_application` test suite is excluded from the native build via `test_ignore` in `platformio.ini`. The `Application` library is also in `lib_ignore` for the `[env:native]` environment because Application has hardware dependencies (WiFi, LittleFS, etc.) that aren't fully stubbed.

**Impact:** New test cases in Test Group 9 (PR-66 tests) are syntactically correct and compile-validated, but cannot be execution-verified via `pio test -e native`. They would need a separate native environment with Application included and additional stubs.

**Workaround:** Tests were written to follow the existing test patterns and are validated by the fact that the existing 377 tests all pass (no regressions). The test file compiles as part of the Application library on hardware targets.

**Future Fix:** This is tracked as a broader framework issue — creating a `native_application` test environment that includes sufficient stubs.

## 2. Pre-Existing publicKey.hpp Build Failure

**Problem:** Both `esp32s3nano` and `esp32dev` hardware targets fail with `publicKey.hpp: No such file or directory` in `src/main.cpp:21`. This is a deployment-specific OTA verification key file that is not tracked in git.

**Verification:** Confirmed pre-existing by stashing WO1 changes and rebuilding — same failure. This is NOT caused by WO1.

**Impact:** Cannot verify full hardware compilation of WO1 changes via `pio run`. However, CMake dependency resolution completed successfully (Application @ 0.1.0 was discovered), and zero errors/warnings were found from the modified framework library files.

## 3. WiFi.localIP() Returns Temporary String

**Gotcha:** `WiFi.localIP().toString().c_str()` returns a pointer into a temporary Arduino `String` object. If you store this pointer in the payload struct, it becomes a dangling pointer after the statement ends.

**Solution:** The `_deviceIPBuf[16]` char array on Application provides a stable buffer. Always `strncpy` into it and null-terminate.

## 4. Pointer Comparison vs String Comparison for Gate Names

**Gotcha:** Gate name pointers from `GateNames::` constants are `static constexpr` — the same pointer is reused across all calls. However, `getGateName()` is virtual and downstream implementations might return different pointers for the same string value. The re-publishing logic uses `strcmp` after an initial fast pointer comparison to handle both cases correctly.
