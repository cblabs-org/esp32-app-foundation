# Project: P1 and P2 Issue Sweep

## Architectural Intent
A sweep to clear the critical P1 blocker (#175) and all P2 enhancements/bugs identified during the post-PR-191 triage. The work spans several distinct domains (DataManager bug fix, test infra, CI pipeline, OTA GitHub channels, and Wifi roaming) but will be delivered as a single stacked PR sequence.

## Work Order Ledger

- **[x] [wo1-data-manager-uaf](wo1-data-manager-uaf/WORKORDER.md):** [P1] Fix `DataManager` stack-local `EventBus` payload (Issue #175). Replace stack allocation with a class member payload.
- **[x] [wo2-native-test-repair](wo2-native-test-repair/WORKORDER.md):** [P2] Fix native test suite build errors for `pngWriter` and `mcpServer` (Issue #134). Required before CI can be trusted.
- **[x] [wo3-ci-pipeline](wo3-ci-pipeline/WORKORDER.md):** [P2] Add GitHub Actions CI pipeline (Issue #144) to ensure quality gates run automatically on PRs.
- **[x] [wo4-ota-beta-check](wo4-ota-beta-check/WORKORDER.md):** [P2] Debug and harden the beta-channel OTA GitHub check (Issue #102). 
- **[x] [wo5-wifi-roaming](wo5-wifi-roaming/WORKORDER.md):** [P2] Implement AP re-evaluation/roaming for `WifiManager` on mesh networks (Issue #192).

## Session Log

- **[2026-08-29]** Project initialized following triage session. `wo1` delegated.

## Modules & Components
- DataManager
- Tests
- CI
- OTAUpdateManager
- WifiManager
- **[2026-08-29]** WO1 complete: Replaced `DataManager` stack allocation with member variable `_publishPayload`.
- **[2026-08-29]** WO2 complete: Guarded ESP32 ROM headers for host compilation, added stubs, native suite is 100% green.
- **[2026-08-29]** WO3 complete: Added `.github/workflows/ci.yml` and enforced CI constraints in web test runner.
- **[2026-08-29]** WO4 complete: Confirmed beta-check bug was already resolved by the PR-36 manifest architecture.
- **[2026-08-29]** WO5 complete: Implemented 802.11k/v roaming features and RSSI thresholds in `WifiManager` to resolve Issue #192.

## Project Wrap-Up
- **Outcome:** The P1 and P2 issue sweep successfully cleared one critical blocker (#175) and four high-priority enhancements/fixes (#134, #144, #102, #192) in 5 stacked work orders.
- **Architectural Notes:** The addition of a fully green native test suite enforcing GitHub Actions CI is a significant milestone for project stability. 
- **Unresolved:** None. All scoped issues are addressed.
