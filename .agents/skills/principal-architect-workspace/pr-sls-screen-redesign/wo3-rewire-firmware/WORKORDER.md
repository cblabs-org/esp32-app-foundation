---
name: sls-screen-redesign-wo3
description: Handoff instructions for Work Order 3 — Re-wire SLSSystemScreens + native stubs + tests to the new widget symbol set
complexity: standard
---

# WO3 — Re-wire Firmware Bridge to New Widget Names

### Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Update the framework bridge and its native scaffolding to the redesigned widget symbol set
so both `native` tests and the `crowpanel-128-rotary` firmware build are green again — at
**behaviour parity** (Error + OTA wired exactly as before; the new Setup/Busy/Message groups
exist and are hidden, but are NOT newly driven).

## Context
- **Branch:** `sls-screen-redesign/wo3-rewire-firmware` (based on WO2). The new UI C from WO2 is already in place.
- The module invokes only `errorScreen()`, `otaScreen()`, `onError()`, `onOtaProgress()` on the provider.
  Do not attempt to make the new groups reachable — that is a deferred, separate PR (PROJECT.md OQ-2).

### Old → New global-symbol mapping (authoritative)
Unchanged (leave as-is): all `ui_uiBoot*`, all `ui_ui_SystemError*` + `ui_uiSystemErrorGroup`,
all `ui_ui_SystemOTA*` + `ui_uiSystemOTAGroup`, `ui_ui_SystemSpinner`, `ui_uiSystemScreen`,
`ui_ui_DiagArc`, `ui_ui_DiagValue`, `ui_ui_DiagStatus`, `ui_ui_DiagScreen`.

| Old symbol | New symbol |
|------------|------------|
| `ui_uiSystemWizardGroup` | `ui_uiSystemSetupGroup` |
| `ui_ui_SystemWizardTitle` | `ui_ui_SystemSetupTitle` |
| `ui_ui_SystemWizardSSIDLbl` | `ui_ui_SystemSetupSSIDCaption` |
| `ui_ui_SystemWizardSSID` | `ui_ui_SystemSetupSSID` |
| `ui_ui_SystemWiFiIcon` | `ui_ui_SystemSetupIcon` |
| `ui_ui_SystemWizardStatus` | `ui_ui_SystemSetupURLCaption` |
| `ui_ui_SystemWizardIP` | `ui_ui_SystemSetupURL` |
| `ui_uiSystemInterstitialGroup` | `ui_uiSystemBusyGroup` |
| `ui_ui_SystemInterstitialMessage` | `ui_ui_SystemBusyMessage` |
| `ui_ui_DiagTitle` | `ui_ui_DiagCategory` |
| `ui_ui_DiagUnit` | *(removed)* |
| `ui_ui_DiagInfoRoller` | *(removed)* |
| `ui_ui_DiagTouchBtnLbl` | *(removed)* |
| `ui_ui_DiagTouchButton` | *(removed)* |
| — | `ui_ui_SystemMsgIcon` / `ui_ui_SystemMsgTitle` / `ui_ui_SystemMsgBody` *(new)* |
| — | `ui_uiSystemMessageGroup` *(new)* |
| — | `ui_ui_DiagCounter` *(new)* |

## Strict Boundary Box
- Do **NOT** modify anything under `modules/LVGLDisplay/ui/` (generated) — if a symbol name here differs from the mapping above, the export is the source of truth: **stop and report** the discrepancy rather than editing generated code.
- Do **NOT** modify `include/interfaces/ILVGLSystemScreens.hpp` — the interface is stable this PR.
- Do **NOT** modify `LVGLDisplayModule.cpp` core logic. (If it references a renamed global directly — it should not — stop and report.)
- Do **NOT** implement `configuringScreen()`/`wifiWizardScreen()`/`gateHelperScreen()` or add module call sites — deferred (OQ-2).
- Do **NOT** flash or deploy.

## Tooling Constraints
- Read `pio-manager` before PlatformIO ops; use the `platformio` MCP server (rule `04-mcp-tooling.md`).
- Builds/tests: `run_tests` on `environment: "native"`; `build_project` on `environment: "crowpanel-128-rotary"`.
- Consult the `framework-expert` skill if you need module/interface pattern guidance.

## Line-Item Instructions
- [ ] `modules/LVGLDisplay/screens/SLSSystemScreens.cpp`:
  - In `hideAllGroups()`: rename `ui_uiSystemWizardGroup`→`ui_uiSystemSetupGroup`, `ui_uiSystemInterstitialGroup`→`ui_uiSystemBusyGroup`, and **add** hiding of the new `ui_uiSystemMessageGroup`. Keep Error + OTA group hides.
  - `errorScreen()` / `otaScreen()` / `onError()` / `onOtaProgress()` reference only unchanged symbols — verify no edits needed.
- [ ] `modules/LVGLDisplay/screens/SLSSystemScreens.hpp`: update the class/doc comments that name the old groups (Wizard/Interstitial) to Setup/Busy and mention the Message group. No API change.
- [ ] `test/stubs/platform/ui_stubs.c`: rewrite the stub global set to match the new export **exactly** — add `ui_ui_SystemSetup*` (Title/SSIDCaption/SSID/Icon/URLCaption/URL), `ui_uiSystemSetupGroup`, `ui_ui_SystemBusyMessage`, `ui_uiSystemBusyGroup`, `ui_ui_SystemMsg*` (Icon/Title/Body), `ui_uiSystemMessageGroup`, `ui_ui_DiagCategory`, `ui_ui_DiagCounter`; **remove** `ui_ui_SystemWizard*`, `ui_ui_SystemWiFiIcon`, `ui_ui_SystemInterstitialMessage`, `ui_uiSystemInterstitialGroup`, `ui_ui_DiagTitle`, `ui_ui_DiagUnit`, `ui_ui_DiagInfoRoller`, `ui_ui_DiagTouchBtnLbl`, `ui_ui_DiagTouchButton`. (Cross-check against the actual WO2 export symbol list, not just this table.)
- [ ] `test/test_unit/test_lvglDisplay/test_sls_screens.cpp`: update any assertions/fixtures referencing the renamed/removed globals; add coverage that `hideAllGroups()` now also hides the Message group. Keep the Error/OTA behaviour tests.
- [ ] `run_tests` on `native` — all pass.
- [ ] `build_project` on `crowpanel-128-rotary` — compiles clean (no flash).
- [ ] (Optional sanity) `build_project` on `esp32s3nano` if it built pre-change — confirm no regression; note if it has a pre-existing toolchain failure unrelated to this WO.

## Verification Requirement
- Paste `native` test results and the `crowpanel-128-rotary` build result into the Completion Report.
- **Before committing, ask the user if they are happy with the changes.**
- Suggested commit message: `feat(lvgl): re-wire SLSSystemScreens to redesigned Setup/Busy/Message groups`

### Issue Surfacing
If you discover a defect or concern *outside your boundary box* (e.g. the module core does reference a renamed global, or OQ-2 work looks necessary sooner), surface it in your Completion Report — do not file issues directly. The PA will triage.

## Provenance
Part of **PR: SLS Screen Redesign** ([PROJECT.md](../PROJECT.md)); decomposition approved by the human (see PROJECT.md §8 Status Log). Authorized to execute WO3 autonomously within this Strict Boundary Box.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
-

### Decisions Made
-

### Dead Ends & Warnings
-

### Out-of-Scope Findings
-

### Blockers
(none)
