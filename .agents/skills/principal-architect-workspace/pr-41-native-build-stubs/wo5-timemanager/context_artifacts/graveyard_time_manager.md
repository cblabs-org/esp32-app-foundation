# Graveyard: TimeManager Native Stubs

## Failed ArduinoJson Serialization in Native
When running the native tests, `test_homeassistant` and `test_timeManager` failed because ArduinoJson could not serialize to the `String` class. 
**Error:** `error: no member named 'write' in 'String'`
**Fix:** ArduinoJson looks for a `write` method to treat the target as a stream/print object when serializing. Added `size_t write(uint8_t)` and `size_t write(const uint8_t*, size_t)` to the mock `String` class in `Arduino.h`.

## Missing LittleFS Methods
`test_timeManager` also failed to build because `readBytes` and `rename` were missing from our mock `LittleFS` and `File` classes.
**Fix:** Added these to `test/stubs/platform/LittleFS.h`. `rename` was implemented using `std::filesystem::rename`.

## Failing TimeManager Unit Tests
After removing the mock logic, the `test_timeManager` tests failed: `TimeManagerTest.BeginSetsSynced: Failure`.
**Why:** The mock `begin()` immediately set `_syncStatus = TimeSyncStatus::SYNCED`. The real `begin()` only sets it to `IN_PROGRESS` and waits for `tick()` to poll the SNTP status. 
**Fix:** Added `timeManager.tick(0)` calls immediately following `timeManager.begin()` in `test_timeManager.cpp` to correctly exercise the state machine.
