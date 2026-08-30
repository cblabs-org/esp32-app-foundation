# Project: P1 and P2 Issue Sweep

## Architectural Intent
A sweep to clear the critical P1 blocker (#175) and all P2 enhancements/bugs identified during the post-PR-191 triage. The work spans several distinct domains (DataManager bug fix, test infra, OTA investigation, and WiFi roaming) and was delivered as a stacked PR sequence.

## Work Order Ledger

- **[x] [wo1-data-manager-uaf](wo1-data-manager-uaf/WORKORDER.md):** [P1] Fix `DataManager` stack-local `EventBus` payload (Issue #175). Replaced stack allocation with a class member payload. **Merged: PR #195.**
- **[x] [wo2-native-test-repair](wo2-native-test-repair/WORKORDER.md):** [P2] Fix native test suite build errors for `pngWriter` and `mcpServer` (Issue #134). Added `#if defined(ESP_PLATFORM)` guards and LVGL stubs. **Merged: PR #196.**
- **[x] [wo3-ci-pipeline](wo3-ci-pipeline/WORKORDER.md):** [P2] Add GitHub Actions CI pipeline (Issue #144). **Dropped after review** — CI for embedded PIO toolchains with OTA signing is not practical via GitHub Actions. PR #197 closed.
- **[x] [wo4-ota-beta-check](wo4-ota-beta-check/WORKORDER.md):** [P2] Debug beta-channel OTA GitHub check (Issue #102). Investigation confirmed the bug was superseded by PR-36's signed manifest architecture. **No code changes.** PR #198 closed, Issue #102 closed with documented rationale.
- **[x] [wo5-wifi-roaming](wo5-wifi-roaming/WORKORDER.md):** [P2] Implement AP re-evaluation/roaming for `WifiManager` on mesh networks (Issue #192). **Merged: PR #199** (revised after review).

## Session Log

### 2026-08-29T17:00:00Z — Project Initialized
Triaged 18 open issues into P1/P2/P3 buckets. Decomposed into 5 WOs. Delegated WO1–WO5 to worker agents sequentially.

### 2026-08-29T19:00:00Z — WO1–WO5 Complete (Initial)
All 5 WOs executed by worker agents. Stack of 5 PRs submitted (#195–#199).

### 2026-08-29T22:09:00Z — Review Feedback Received
Human reviewer posted detailed reviews on all 5 PRs:
- **#195, #196, #198**: Approved.
- **#197 (CI)**: Rejected — `pull_request: branches: [main]` filters on base branch so CI never runs on stacked PRs. Also, `SKIP_WEB_TESTS=1` in compile-check makes the `run_web_tests.py` CI hardening unreachable.
- **#199 (WiFi)**: Rejected — three verified findings: (1) all sdkconfig files have `CONFIG_ESP_WIFI_11KV_SUPPORT` disabled, so `btm_enabled`/`rm_enabled` assignments are no-ops; (2) `esp_wifi_set_rssi_threshold()` called but no `WIFI_EVENT_STA_BSS_RSSI_LOW` handler registered; (3) `threshold.rssi = -75` is an association filter, not a roaming trigger, causing a connectivity regression for devices at fringe -80 dBm.

### 2026-08-29T22:19:00Z — WO3 Dropped
User confirmed CI for embedded PIO with OTA signing is not practical. PR #197 closed. Stack retargeted to skip WO3.

### 2026-08-29T22:27:00Z — WO5 Revised per Review
Reviewer's key insight: `CONFIG_ESP_WIFI_11KV_SUPPORT=y` in `sdkconfig.defaults` is the correct fix — enables the IDF supplicant to natively parse BTM steering requests from mesh controllers, with zero application code. Changes applied:
- **Stripped**: Entire RSSI threshold machinery (member, constant, getter/setter, config serialize/deserialize, API/MCP fields, 5 tests, stub). Removed redundant `WiFi.setScanMethod()`/`WiFi.setSortMethod()` calls. Removed no-op 11k/v struct assignments.
- **Kept**: `WIFI_ALL_CHANNEL_SCAN` + `WIFI_CONNECT_AP_BY_SIGNAL` via `esp_wifi_set_config` (boot-time AP selection). Two-stage connect pattern. `connectStation()` extraction.
- **Added**: `CONFIG_ESP_WIFI_11KV_SUPPORT=y` in `sdkconfig.defaults`.
- **Verified**: 844/844 native tests, all 4 framework envs compile clean.

### 2026-08-29T22:52:00Z — WO4 Dropped (Redundant)
PR #198 contained only a workspace WORKORDER file with no code changes. Closed. Finding tracked in Issue #202.

### 2026-08-29T23:10:00Z — Stack Merged
PRs #195, #196, #199 merged to main bottom-up. Issues #175, #134, #192, #102 closed.

### 2026-08-30T08:12:00Z — Build Verification (All Projects)
Full build matrix across framework, sample-app, and template-app revealed sample-app and template-app failures: `PsychicHttp@3.0.0` (exact pin) is incompatible with current ArduinoJson v7 headers. Framework used `@^3.0.0` (caret range → 3.1.1), which works. Root cause: apps drifted from the framework's semver convention.
- **Fix**: Updated both apps to use caret ranges (`@^3.0.0`, `@^7.4.3`). Committed directly to main.
- **Final matrix**: 12/12 hardware builds green, 844/844 native tests green.

**Cross-cutting decisions:**
- The project does not use GitHub Actions CI. Embedded PIO toolchains with OTA signing make cloud CI impractical.
- WiFi roaming policy should be delegated to Kconfig/IDF supplicant wherever possible, not hand-coded. Application-level RSSI thresholds are association filters, not roaming triggers.
- App dependency versions must use caret ranges matching the framework to avoid silent drift.

**Dead ends (project-wide):**
- Hand-coded 802.11k/v roaming (btm_enabled, rm_enabled struct assignments) is inert without `CONFIG_ESP_WIFI_11KV_SUPPORT` in sdkconfig. The IDF supplicant owns the protocol negotiation. The struct fields are informational within the wifi_config_t but only active when the Kconfig flag compiles in the 11k/v code paths.
- `esp_wifi_set_rssi_threshold()` is a one-shot API that requires a `WIFI_EVENT_STA_BSS_RSSI_LOW` event handler to re-arm. Without the handler, it fires once and is never re-evaluated.

## Project Wrap-Up

### 2026-08-30T09:11:00Z — Project Wrap-Up
**Architectural outcome:** Three code PRs merged: a critical EventBus UAF fix in DataManager, native test suite restored to 100% green, and WiFi mesh roaming via Kconfig (`CONFIG_ESP_WIFI_11KV_SUPPORT`) plus boot-time AP selection improvements. Two PRs dropped after review (CI pipeline: not applicable; OTA investigation: no code changes). A latent app dependency drift (exact pins vs caret ranges for PsychicHttp/ArduinoJson) was discovered and fixed during build verification.

**Lessons for future projects:**
1. **Review-driven iteration is high-value.** The reviewer caught that WO5's entire RSSI threshold approach was architecturally wrong — the fix was one Kconfig line, not 200 lines of state machine. Workers lack the domain context to make that call.
2. **Build the full matrix before merging.** The sample/template-app dependency drift would have gone unnoticed without the comprehensive build pass.
3. **Unpinned platforms are a risk.** `platform = espressif32` with no `@version` means the IDF version resolved at build time is unknowable from the repo alone.

**Unresolved items:**
- Issue #200: True 11k/v roaming verification on hardware against the actual mesh.
- Issue #201: `test_otaUpdateManager` is in `test_ignore` — blind spot now that native tests are a quality gate.
- Issue #202: Document Issue #102 findings in a discoverable location.
- Issue #144: CI pipeline remains open for future consideration.
- `platform = espressif32` is unpinned across all projects.

## Modules & Components
- DataManager
- Tests (native suite, PngWriter, MCPServer stubs)
- WifiManager
- OTAUpdateManager
- sdkconfig.defaults
- sample-app/platformio.ini, template-app/platformio.ini
