---
name: sls-screen-redesign-wo5
description: Handoff instructions for Work Order 5 — SLSSystemScreens drives Setup/Busy/Message groups for CONFIGURING
complexity: standard
---

# WO5 — SLS Configuring Skin (drive Setup/Busy/Message)

### Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the configuring half of `SLSSystemScreens` so the module's three-tier dispatch
(WO4) drives the redesigned System-screen visibility groups, giving the CrowPanel a working
WiFi-setup / gate-helper / please-wait experience during CONFIGURING.

## Context
- **Branch:** `sls-screen-redesign/wo5-sls-configuring-skin` (based on WO4). The `onConfiguring` interface hook + module dispatch already exist.
- Group ↔ role mapping (from PROJECT.md §9):
  - **Setup** group (`ui_uiSystemSetupGroup`; `ui_ui_SystemSetupSSID`, `ui_ui_SystemSetupURL`) ← WiFi wizard (`wifiWizardScreen()`), for `GateNames::WIFI_STATION`.
  - **Busy** group (`ui_uiSystemBusyGroup`; `ui_uiSystemBusyMessage`, `ui_uiSystemSpinner`) ← base/InProgress (`configuringScreen()`).
  - **Message** group (`ui_uiSystemMessageGroup`; `ui_ui_SystemMsgTitle`, `ui_uiSystemMsgBody`) ← generic gate helper (`gateHelperScreen(p)`), for non-WiFi gates.
- `AppConfiguringPayload` fields: `gateName`, `helpText`, `actionStatus`, `hostname` (AP SSID), `deviceIP` (portal host). See `include/events/AppConfiguringPayload.hpp`.

## Strict Boundary Box
- Do **NOT** modify `LVGLDisplayModule.cpp` core or `ILVGLSystemScreens.hpp` (WO4 owns them).
- Do **NOT** modify the generated `ui/` files.
- Do **NOT** flash.

## Tooling Constraints
- `platformio` MCP: `run_tests` native, `build_project` crowpanel-128-rotary. Rule `04-mcp-tooling.md`.

## Line-Item Instructions
- [ ] `modules/LVGLDisplay/screens/SLSSystemScreens.hpp`: declare overrides `configuringScreen()`, `wifiWizardScreen()`, `gateHelperScreen(const AppConfiguringPayload*)`, `onConfiguring(const AppConfiguringPayload*)`. Add a private helper `showGroup(lv_obj_t* group)` = `hideAllGroups()` then un-hide `group`, returning `ui_uiSystemScreen`.
- [ ] `modules/LVGLDisplay/screens/SLSSystemScreens.cpp`:
  - `onConfiguring(p)` — populate widgets: `ui_ui_SystemSetupSSID` ← `p->hostname`; `ui_ui_SystemSetupURL` ← `http://<deviceIP>`; `ui_uiSystemBusyMessage` ← `p->helpText` (or a default e.g. "Please wait…"); `ui_ui_SystemMsgTitle` ← `p->gateName`; `ui_ui_SystemMsgBody` ← `p->helpText`. All null-guarded.
  - `configuringScreen()` → `showGroup(ui_uiSystemBusyGroup)`.
  - `wifiWizardScreen()` → `showGroup(ui_uiSystemSetupGroup)`.
  - `gateHelperScreen(p)` → **return nullptr when `p->gateName == GateNames::WIFI_STATION`** (let WO4's Tier-2 wizard handle WiFi); otherwise `showGroup(ui_uiSystemMessageGroup)`.
- [ ] `test/test_unit/test_lvglDisplay/test_sls_screens.cpp`: add fixtures for Setup/Busy/Message widgets + label children; tests that `wifiWizardScreen()` shows only Setup, `configuringScreen()` only Busy, `gateHelperScreen(non-wifi)` only Message, `gateHelperScreen(WIFI_STATION)` returns nullptr, and `onConfiguring` sets SSID/URL/title/body without crashing (incl. null-globals defensive case).
- [ ] `test/stubs/platform/ui_stubs.c` — already carries all Setup/Busy/Message symbols (WO3); no change expected. Confirm.
- [ ] `run_tests` native — all green. `build_project` crowpanel-128-rotary — clean.

## Verification Requirement
Paste native + crowpanel results. **Before committing, ask the user** (waived under standing autonomous approval). Commit: `feat(lvgl): SLSSystemScreens drives Setup/Busy/Message during CONFIGURING (#82)`. The WO5 commit (stack tip) closes #82 via `Fixes #82` in the PR body.

### Issue Surfacing
Surface out-of-boundary findings in the Completion Report; do not file issues directly.

## Provenance
Part of **PR: SLS Screen Redesign** ([PROJECT.md](../PROJECT.md)); approved by the human (PROJECT.md §8). Skin half of framework issue #82.

---
## Completion Report
<!-- Written at task completion -->

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
