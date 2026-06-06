# State: Modified Files (WO6)

## Files Modified

### `web/portal/ota.js`

| Line Range | Change |
|---|---|
| L1-5 | Updated header comment — removed "hidden PR-14 stub" reference |
| L21-34 | Activated `#ota-channel-wrapper` (removed `display:none`), added `#ota-channel-status` feedback div |
| L33-34 | Activated `#ota-manifest-info` (removed `display:none`), added article with `#ota-manifest-content` |
| L78-99 | Enhanced `loadFirmwareInfo()` — added second fetch to status endpoint, populates channel dropdown, firmware info channel row, and build info section |
| L264-304 | Replaced 3-second armed rollback with `prompt("Type ROLLBACK...")` + confirm token in payload |
| L306-318 | Added `#ota-channel` change event listener wiring to `setChannel` API |

### `modules/OTAUpdateManager/OTAUpdateManager.cpp`

| Line Range | Change |
|---|---|
| L847-850 | Rollback action now validates `doc["confirm"] == "ROLLBACK"`, returns 403 if invalid |
| L867-885 | Added `setChannel` action handler with validation + ConfigManager persistence |
| L867-885 | Extended `status` response with `version`, `channel`, `boardId`, `autoCheck` fields |

## Files NOT Modified (Boundary Box Compliance)

- `modules/Application/` — WO2
- `lib/OTADownloadEngine/` — WO2
- `lib/OTAManifest/` — WO3
- `lib/GithubClient/` — WO4
- `lib/ConfigManifest/` — WO5
- `modules/ConfigManager/` — WO5
- `modules/OTAUpdateManager/OTAUpdateManager.hpp` — No changes needed (all new fields/methods already existed)
