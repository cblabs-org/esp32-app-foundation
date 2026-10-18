# Graveyard: KeyManager Refactoring

## Failed Test Attempt 1: Incomplete Stubs
**Attempt:** Ran `pio test -e native` immediately after removing `NATIVE_BUILD` guards.
**Result:** Build failed.
- Missing `hasParam` and `getParam` on `PsychicRequest` stub.
- Missing 0-argument `read()` and 1-argument `write(uint8_t)` in `LittleFS` stub's `File` class, which are required by `ArduinoJson` serialization engine.

## Failed Test Attempt 2: Filesystem Conflicts
**Attempt:** Re-ran `pio test -e native` after fixing stubs.
**Result:** Tests failed and crashed with `SIGHUP` on `MetadataPersistsAcrossInstances`.
**Reason:** The native test suite for KeyManager used `<stdio.h>` `remove(KEY_MANAGER_PATH)` in `SetUp()` and `TearDown()`. Since `KEY_MANAGER_PATH` changed from `etc_keys.json` to `/etc/keys.json`, `remove()` attempted to delete from the host system's root partition (failing with permission denied). The old data was never cleared from the mocked LittleFS environment.
**Fix:** Modified the test file to use `LittleFS.remove(KEY_MANAGER_PATH)` which correctly roots the path under the mock test directory (`test/tmp/`).
