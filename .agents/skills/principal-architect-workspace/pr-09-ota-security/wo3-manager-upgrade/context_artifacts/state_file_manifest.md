# State: File Manifest — WO3 Deliverables

## Modified Files

| File | Lines | Key Symbols |
|------|-------|-------------|
| `include/FrameworkConfig.hpp` | 47 | `FrameworkConfig.otaPublicKey`, `.otaGithubRepo`, `.otaGithubToken` |
| `modules/application/Application.hpp` | ~210 | `Application::getConfig()` |
| `modules/application/Application.cpp` | ~376 | `Application::getConfig()` impl |
| `modules/otaUpdateManager/otaUpdateManager.hpp` | 255 | `OtaState`, `OtaProgressEvent`, `otaUpdateManager` class |
| `modules/otaUpdateManager/otaUpdateManager.cpp` | 613 | `begin()`, `checkForUpdate()`, `performUpdate()`, `performUpdateTask()`, `otaTaskWrapper()`, `publishProgress()`, legacy API, REST routes |
| `test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp` | 148 | 16 GoogleTest cases |

## Test Coverage

| Test Name | Covers |
|-----------|--------|
| `DefaultState_IsIdle` | Initial state machine state |
| `CheckForUpdate_WithoutBegin_ReturnsFalse` | Guard: app_ == nullptr |
| `PerformUpdate_WithoutBegin_SetsError` | Guard: app_ == nullptr → Error |
| `GetLastOtaError_Default_ReturnsNull` | Default error state |
| `PerformUpdate_OnNative_SetsError` | Native: no FreeRTOS → Error |
| `CheckForUpdate_NoRepo_ReturnsFalse` | Guard: otaGithubRepo == nullptr |
| `BeginUpdate_OnNative_ReturnsFalse` | Legacy: native stub |
| `EndUpdate_OnNative_ReturnsFalse` | Legacy: native stub |
| `BeginUpdate_NullptrMD5_ReturnsFalse` | Legacy: null guard |
| `BeginUpdate_ZeroSize_ReturnsFalse` | Legacy: size guard |
| `MarkAppValid_NoCrash` | Legacy: no-op on native |
| `RollbackFirmware_NoCrash` | Legacy: no-op on native |
| `GetTabName_ReturnsOTA` | IWebContributor |
| `WriteStream_OnNative_ReturnsZero` | Legacy: native stub |
| `RegisterRoutes_NoCrash` | IWebContributor route registration |
| `GlobalInstance_Accessible` | Global `otaManager` symbol |

## Commit

- SHA: `9a07d0b`
- Message: `feat(ota): upgrade otaUpdateManager to full OTA orchestrator with GitHub pipeline [PR-09/WO3]`
- Branch: `pr-09-ota-security-wo3-manager-upgrade`
- Stats: 6 files changed, +807 / -109
