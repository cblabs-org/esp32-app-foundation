# State: Files Changed in WO2

## Commit `928466e` — 3 files, +316/-75

### `include/FrameworkConfig.hpp`
- Added `const char* otaManifestUrl = nullptr;` between `otaPublicKey` and `otaGithubRepo`
- Annotated `otaGithubRepo` as legacy (WO3 removes)

### `modules/OTAUpdateManager/OTAUpdateManager.hpp`
- Schema version docstring updated: v4 → v5
- IConfigurable field list updated to include `learnedManifestUrl`
- Added `char learnedManifestUrl_[256] = "";` private member (~line 413)
- Added `bool fetchAndVerifyManifest_(OTAManifest& manifest);` private method declaration (~line 506)
- Guarded old `downloadAndParseManifestVersion_()` declaration with `#if 0` (~lines 510-524)

### `modules/OTAUpdateManager/OTAUpdateManager.cpp`
- `getSchemaVersion()` returns 5 (~line 108)
- `applyConfig()` reads `learnedManifestUrl` (~line 130)
- `serializeConfig()` writes `learnedManifestUrl` (~line 148)
- `migrateConfig()` adds v4→v5 migration (~line 169)
- `resetConfig()` clears `learnedManifestUrl_` (~line 188)
- `downloadRawFile_()` static helper (~lines 828-924): HTTPS download with CA/insecure fallback
- `fetchAndVerifyManifest_()` (~lines 926-1074): Full implementation with URL fallback, sig verification, manifest parse, learned URL persistence
- Legacy `downloadAndParseManifestVersion_()` guarded with `#if 0` (~lines 1076-1203)
- Legacy pipeline shims in `performUpdateTask()` (~lines 537-598): Floating tag path returns error, manifest path falls through to legacy asset search
