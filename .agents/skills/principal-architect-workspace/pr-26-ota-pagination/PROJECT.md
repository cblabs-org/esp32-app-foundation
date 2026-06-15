# PR-26: OTA Paginated Release Scanner + Logging

**PR Branch:** `pr-26-ota-pagination`
**Base:** `main`
**Status:** 🔄 **IN PROGRESS**

## Objective

Fix the broken beta/dev channel OTA update flow: paginate release scanning, fix version comparison, improve logging, and align portal progress bar.

## Work Order Ledger

- **[x] [wo1-paginated-scanner](wo1-paginated-scanner/WORKORDER.md):** Rewrite `getLatestReleaseForChannel()` with paginated scan + 3 early-exit conditions
- **[ ] [wo2-ota-logging](wo2-ota-logging/WORKORDER.md):** Update all OTA log lines to include channel and BUILD_VERSION; fix status API version string
- **[ ] [wo3-local-version-channel](wo3-local-version-channel/WORKORDER.md):** Replace hardcoded `Channel::STABLE` with `VERSION_CHANNEL`/`VERSION_PRE_NUM` build flags
- **[ ] [wo4-portal-progress](wo4-portal-progress/WORKORDER.md):** Expose real download progress to web portal via status API

## Serialisation

All WOs are serialised (WO1 → WO2 → WO3 → WO4). No file overlap between WOs.

## Files Touched (per WO)

| WO | Files |
|----|-------|
| WO1 | `lib/GithubClient/GithubClient.cpp` |
| WO2 | `modules/OTAUpdateManager/OTAUpdateManager.cpp` |
| WO3 | `modules/OTAUpdateManager/OTAUpdateManager.cpp`, `platformio.ini` |
| WO4 | `modules/OTAUpdateManager/OTAUpdateManager.hpp`, `modules/OTAUpdateManager/OTAUpdateManager.cpp`, `web/portal/ota.js` |
