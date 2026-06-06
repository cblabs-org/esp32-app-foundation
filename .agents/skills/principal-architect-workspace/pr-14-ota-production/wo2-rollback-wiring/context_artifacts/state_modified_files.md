# State: Modified Files Summary

## Files Modified

| File | Change Summary |
|------|---------------|
| `modules/Application/Application.cpp` | Inserted `markAppValid()` call as Step 17b between `_compactAndLock()` and heap report |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Fail-closed sig checks, TWDT subscribe/unsubscribe on all paths, `checkSecurityVersion()` impl, `Preferences.h` + `esp_task_wdt.h` includes |
| `modules/OTAUpdateManager/OTAUpdateManager.hpp` | Added `checkSecurityVersion()` private method declaration with Doxygen |
| `lib/OTADownloadEngine/OTADownloadEngine.cpp` | Fail-closed Step 10, TWDT feed in download loop, `esp_task_wdt.h` include |
| `sdkconfig.defaults` | Added `CONFIG_ESP_TASK_WDT_PANIC=y` |
| `sdkconfig.defaults.esp32s3nano` | Added `CONFIG_ESP_TASK_WDT_PANIC=y` |
| `platformio.ini` | Added `-DSECURITY_VERSION=1` to `base_esp32` and `native` build_flags |

## Files Created

| File | Description |
|------|------------|
| `test/test_unit/test_ota_fail_closed/test_ota_fail_closed.cpp` | 8 GoogleTest cases: enum values, string coverage, native stub contract, `shouldAbortOnMissingSig()` helper |

## Key Architectural Points

- `markAppValid()` placement at Step 17b means the self-test barrier covers: display bootstrap, EventBus, LittleFS, Crypto, Config, WiFi, Time, Data, OTA, Web, display integration, and registry lock.
- `checkSecurityVersion()` is called BEFORE `esp_ota_mark_app_valid_cancel_rollback()` in `markAppValid()`, ensuring the NVS counter is updated even if the blessing call is a no-op (e.g., when `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` is not defined).
- TWDT unsubscribe is present on all 10 exit paths from `performUpdateTask()` (1 success + 9 error returns, including the sig-fetch early return which was added by this WO).
