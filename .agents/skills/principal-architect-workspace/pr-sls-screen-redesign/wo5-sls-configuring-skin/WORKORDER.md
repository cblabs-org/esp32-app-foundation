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
<!-- Executed inline by the PA acting as architect, autonomous approval 2026-08-09 -->

### Status
complete

### Changes Made
- `modules/LVGLDisplay/screens/SLSSystemScreens.hpp` — declared `configuringScreen`/`wifiWizardScreen`/`gateHelperScreen`/`onConfiguring` overrides + private `showGroup(lv_obj_t*)` helper.
- `modules/LVGLDisplay/screens/SLSSystemScreens.cpp` — added `#include "interfaces/IConfigGate.hpp"` + `<cstring>`; `showGroup()` (hide-all + reveal-one → System root); `configuringScreen()`→Busy, `wifiWizardScreen()`→Setup, `gateHelperScreen(p)`→Message (nullptr for `WIFI_STATION` to defer to Tier-2 wizard); `onConfiguring(p)` populates Setup SSID/URL (`http://<deviceIP>`), Busy message, Message title/body. Refactored `errorScreen`/`otaScreen` onto `showGroup`.
- `test/test_unit/test_lvglDisplay/test_sls_screens.cpp` — 5 new tests (Busy/Setup/Message visibility, WiFi-gate deferral, `onConfiguring` population) + extended null-defensive case.

### Decisions Made
- `gateHelperScreen` returns nullptr for `GateNames::WIFI_STATION` so the module's Tier-2 (`wifiWizardScreen`, Setup group) owns WiFi; generic gates get the Message group.

### Verification
- `build_project` crowpanel-128-rotary → **[SUCCESS]** 65.22s, Flash 43.2% (1,812,871 B). Production `SLSSystemScreens.cpp` compiles with the new methods.
- `run_tests` native → 417/417 — **but see finding below: `test_sls_screens.cpp` is NOT among them.**

### Out-of-Scope Findings
- ⚠️ **The entire `test_unit/test_lvglDisplay` suite (incl. `test_sls_screens.cpp`) is dormant.** `platformio.ini` `lib_ignore`s `LVGLDisplay` on `native` (line 177), so the module can't link natively, and the suite is `test_ignore`d (line 178) — present since `f5d459a` (2026-07-30, hardware-board scaffold). Consequence: the SLS bridge tests (WO13's and mine) have **never actually executed**; native "417/417" excludes them. Production code is verified only via the crowpanel compile. My test edits are correct-by-construction against the crowpanel-validated API but unrun. **Recommend a tracked issue** to restore native LVGL-display test coverage (e.g. compile `SLSSystemScreens.cpp` into the native suite against the existing lvgl stubs, independent of the ESP-only `LVGLDisplayModule.cpp`). → PA to file.

### Blockers
(none)
