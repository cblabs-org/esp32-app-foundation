# Architecture Decision Record: OTA Native Stubs

## Context
The OTAUpdateManager, OTADownloadEngine, and MbedTLSSignatureVerifier modules relied heavily on `#ifndef NATIVE_BUILD` guards to skip execution during native tests. This caused the native unit tests to only verify the bypass behavior, leaving the core orchestration logic completely un-tested in native builds.

## Decision
All `#ifndef NATIVE_BUILD` guards were removed from these modules. To enable native builds, extensive stubs were added to `test/stubs/platform/`. 
Specifically:
- We stubbed FreeRTOS `xTaskCreateWithCaps`, `vTaskDelete`, etc. to enable `performUpdateTask` to compile and run synchronously.
- We stubbed `PsychicUploadHandler` inside `PsychicHttp.h` and explicitly passed it inside `OTAUpdateManager` route registration to satisfy compilation.
- We stubbed ESP-IDF APIs such as `esp_ota_get_next_update_partition` and `esp_ota_set_boot_partition` inside `esp_ota_ops.h`.
- We removed legacy GoogleTests that explicitly asserted `#ifndef NATIVE_BUILD` fail-safes (e.g., `BeginUpdate_OnNative_ReturnsFalse`), since these paths now execute successfully in the native test environment.

## Consequences
- The orchestration flow for OTA updates is now properly validated in the native test environment.
- Any future changes to `OTAUpdateManager` or `OTADownloadEngine` will throw immediate errors in `pio test -e native` if they break dependencies or logic flows, significantly improving test coverage.
- Test suites relying on native `EXPECT_FALSE` aborts needed to be refactored or deleted.
