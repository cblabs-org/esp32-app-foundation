# Graveyard: OTA Native Stubs

## Failed Attempt: Minimal FreeRTOS task stubbing
- **What happened:** I initially stubbed `xTaskCreateWithCaps` in `freertos/task.h` with an empty implementation that just returned `pdPASS`.
- **Why it failed:** `OTAUpdateManagerTest` tests triggered `manager.performUpdate()` which executes `xTaskCreateWithCaps`. Because the stub was empty, the `performUpdateTask` logic never actually ran in native builds. Later, when we removed obsolete GoogleTests, the test runner crashed with `SIGHUP` because the system executed `ESP.restart()` inside `OTAUpdateManager.cpp` without any stub interception.
- **Resolution:** Modified the `xTaskCreateWithCaps` stub to actually call the `pxTaskCode` function synchronously.

## Failed Attempt: Ignoring obsolete Native_Build tests
- **What happened:** In `test_otaUpdateManager.cpp`, there were several tests like `BeginUpdate_OnNative_ReturnsFalse` that explicitly tested that `#ifndef NATIVE_BUILD` guards functioned correctly (e.g. they expected `false` or failure).
- **Why it failed:** After we removed the `NATIVE_BUILD` guards, those APIs began executing successfully in native mode using our new stubs. Consequently, tests like `EXPECT_FALSE(manager.beginUpdate(1024, "abc123"))` failed because the underlying logic actually worked and returned true/success.
- **Resolution:** Identified that these tests were obsolete because they tested guard bypass behavior. Removed them entirely from `test_otaUpdateManager.cpp` and `test_ota_fail_closed.cpp`.
