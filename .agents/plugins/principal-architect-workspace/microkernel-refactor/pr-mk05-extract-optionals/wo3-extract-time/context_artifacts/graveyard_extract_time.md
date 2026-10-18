# Graveyard: TimeManager Extraction Lessons & Pitfalls (`pr-mk05-wo3`)

- **Issue 1: Transitive Accessor Failures in Unit Tests**
  - **Symptom**: `test/test_unit/test_application/test_application.cpp` compilation error.
  - **Cause**: Test assumed `app.getTimeManager()` was an owned member accessor on `Application`.
  - **Fix**: Removed `GetTimeManager_ReturnsReference` from `test_application.cpp`.

- **Issue 2: Fail-Fast Sentinels for Disabled Optional Modules (EO-07)**
  - **Symptom**: `OTAUpdateManager.cpp:714` compilation error during `native` test suite build when `WITH_OTA` was OFF.
  - **Cause**: `ApplicationEvents::OTA_PROGRESS` was configured in WO2 as a fail-fast sentinel `OTA_PROGRESS_requires_WITH_OTA` when `WITH_OTA` is omitted. `OTAUpdateManager.cpp` and `test_eventbus_integration.cpp` lacked `#ifdef WITH_OTA` guards.
  - **Fix**: Wrapped `OTAUpdateManager` implementation and test handlers in `#ifdef WITH_OTA`.
