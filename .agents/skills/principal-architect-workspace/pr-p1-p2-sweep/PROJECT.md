# Project: P1 and P2 Issue Sweep

## Architectural Intent
A sweep to clear the critical P1 blocker (#175) and all P2 enhancements/bugs identified during the post-PR-191 triage. The work spans several distinct domains (DataManager bug fix, test infra, CI pipeline, OTA GitHub channels, and Wifi roaming) but will be delivered as a single stacked PR sequence.

## Work Order Ledger

- **[ ] [wo1-data-manager-uaf](wo1-data-manager-uaf/WORKORDER.md):** [P1] Fix `DataManager` stack-local `EventBus` payload (Issue #175). Replace stack allocation with a class member payload.
- **[ ] [wo2-native-test-repair](wo2-native-test-repair/WORKORDER.md):** [P2] Fix native test suite build errors for `pngWriter` and `mcpServer` (Issue #134). Required before CI can be trusted.
- **[ ] [wo3-ci-pipeline](wo3-ci-pipeline/WORKORDER.md):** [P2] Add GitHub Actions CI pipeline (Issue #144) to ensure quality gates run automatically on PRs.
- **[ ] [wo4-ota-beta-check](wo4-ota-beta-check/WORKORDER.md):** [P2] Debug and harden the beta-channel OTA GitHub check (Issue #102). 
- **[ ] [wo5-wifi-roaming](wo5-wifi-roaming/WORKORDER.md):** [P2] Implement AP re-evaluation/roaming for `WifiManager` on mesh networks (Issue #192).

## Session Log

- **[2026-08-29]** Project initialized following triage session. `wo1` delegated.

## Modules & Components
- DataManager
- Tests
- CI
- OTAUpdateManager
- WifiManager
