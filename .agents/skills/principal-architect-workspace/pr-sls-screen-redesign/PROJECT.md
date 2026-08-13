# PR — SLS Screen Redesign (CrowPanel 1.28")

> Adopt the reviewed SquareLine Studio redesign into the repo: refresh the SLS
> project source, re-export the LVGL UI C, and re-wire the framework bridge to the
> renamed/added widget groups — at **behaviour parity** with the current build.

**Base branch:** `sls-templates/crowpanel-128` (the WO13 wiring lives here, not on `main`)
**Design authority:** the reviewed SLS export at `/Users/mcneillm/Downloads/sls-project`
(delivered by Claude Design after an interactive review pass)
**Predecessor:** [pr-47-lvgl-display](../pr-47-lvgl-display/PROJECT.md) WO13 (SLSSystemScreens wiring)
**Related issue:** [#82](https://github.com/matthewmcneill/esp32-app-foundation/issues/82) — LVGL CONFIGURING orchestration + `ILVGLSystemScreens` extension. **Owned and implemented in this stack** (we are the framework architect); closed by this PR via `Fixes #82`. Design decisions D1–D3 locked on the issue.
**Status:** ✅ IMPLEMENTED — 6-WO stack landed on `sls-screen-redesign/wo6-native-lvgl-tests` (tip `4d16901`). **Native 482/482** + crowpanel-128-rotary & esp32s3nano [SUCCESS]. Not yet submitted as a PR. #82 (configuring) + #83 (dormant tests, now enabled) both resolved on the branch.

---

## 1. Context

PR-47 WO13 wired three SquareLine-exported screens (Boot / System / Diagnostics)
to the framework via `SLSSystemScreens : ILVGLSystemScreens`. Since then the design
was reviewed interactively in Claude Design and re-exported as an **SLS project**
(`.spj`/`.sll`/`.slt` + assets) — **not** the generated C. The project source in the
repo and the exported UI C in `modules/LVGLDisplay/ui/` are still the *old* design.

Adopting the redesign is therefore a three-part job: ingest the new project source,
regenerate the UI C from SquareLine Studio, and update the C++ bridge that references
the (now renamed) widget globals so the firmware compiles and behaves as before.

## 2. What changed in the redesign (verified diff vs. repo)

Boot screen — **unchanged** (6 widgets, identical names). Error and OTA groups — **unchanged**.
Other changes, all firmware-visible:

| Area | Old | New | Kind |
|------|-----|-----|------|
| System WiFi group | `uiSystemWizardGroup` | `uiSystemSetupGroup` | rename |
| " | `ui_SystemWizardTitle` | `ui_SystemSetupTitle` | rename |
| " | `ui_SystemWizardSSIDLbl` | `ui_SystemSetupSSIDCaption` | rename |
| " | `ui_SystemWizardSSID` | `ui_SystemSetupSSID` | rename |
| " | `ui_SystemWiFiIcon` | `ui_SystemSetupIcon` | rename |
| " | `ui_SystemWizardStatus` | `ui_SystemSetupURLCaption` | rename + **semantic** (status→URL) |
| " | `ui_SystemWizardIP` | `ui_SystemSetupURL` | rename + **semantic** (IP→URL) |
| System "please wait" group | `uiSystemInterstitialGroup` | `uiSystemBusyGroup` | rename |
| " | `ui_SystemInterstitialMessage` | `ui_SystemBusyMessage` | rename |
| " | `ui_SystemSpinner` | `ui_SystemSpinner` | unchanged |
| System **new** group | — | `uiSystemMessageGroup` (`ui_SystemMsgIcon` / `MsgTitle` / `MsgBody`) | **added** |
| Diag | `ui_DiagTitle` | `ui_DiagCategory` | rename |
| Diag | `ui_DiagUnit` | — | **removed** |
| Diag | `ui_DiagInfoRoller` | — | **removed** |
| Diag | `ui_DiagTouchBtnLbl` / `ui_DiagTouchButton` | — | **removed** |
| Diag | — | `ui_DiagCounter` | **added** |
| Logo asset | `cblabs_320x100.png` | `cblabs_180x58.png` | swap |
| Fonts used | 10,14,16,20,28 enabled | needs 12,14,16,18,20,22,40 | enable 12/18/22/40 |

> As exported to C globals, widget names are `ui`-prefixed (e.g. `uiSystemSetupGroup`
> → `ui_uiSystemSetupGroup`; `ui_SystemSetupTitle` → `ui_ui_SystemSetupTitle`).

## 3. Scope

**In scope (this PR — parity adoption):** refresh SLS project source; regenerate and
integrate UI C; enable required fonts; swap logo asset; re-wire `SLSSystemScreens`,
native `ui_stubs.c`, and `test_sls_screens.cpp` to the new symbol set. Error + OTA
behaviour unchanged. Build native + `crowpanel-128-rotary` green. **No flashing.**

**Explicitly out of scope (deferred — tracked by [#82](https://github.com/matthewmcneill/esp32-app-foundation/issues/82)):**
*driving* the new Setup(URL) / Busy / Message groups from the framework. The module's
`_onAppConfiguring` is a `// TODO` stub ([LVGLDisplayModule.cpp:558](../../../../modules/LVGLDisplay/LVGLDisplayModule.cpp)) —
LVGL has **no** CONFIGURING orchestration at all (U8G2 has the full three-tier dispatch).
Wiring the new groups requires implementing `configuringScreen()`/`wifiWizardScreen()`/
`gateHelperScreen()`, adding module call sites in `LVGLDisplayModule.cpp` core, **and an
additive `ILVGLSystemScreens` interface change** — a framework-API decision now with the
architect via #82. It becomes a **follow-on project** once #82 is resolved. In this PR the
new groups exist in the UI and are hidden by `hideAllGroups()` — dormant but present, ready
to be driven once #82 lands. See §9 for the mapping already worked out.

## 4. Work Order Ledger

Stacked on `sls-templates/crowpanel-128`, each WO based on its predecessor.

- **[x] [wo1-ingest-sls-project](wo1-ingest-sls-project/WORKORDER.md)** *(mechanical)* — ✅ `d496574`.
  Reviewed `.spj`/`.sll`/`.slt` + `cblabs_180x58.png` ingested; stale `cblabs_320x100.png` dropped;
  README refreshed; validator clean.
- **[x] [wo2-integrate-ui-export](wo2-integrate-ui-export/WORKORDER.md)** *(standard)* — ✅ (committed on wo2 branch).
  Human re-exported UI C into `modules/LVGLDisplay/ui/`; logo asset swapped; montserrat 12/18/22/40 enabled,
  10/28 pruned. Human tweaks renamed more widgets than the download diff → export is authoritative.
- **[x] [wo3-rewire-firmware](wo3-rewire-firmware/WORKORDER.md)** *(standard)* — 🚧 (native 417/417; crowpanel compile in verification).
  `SLSSystemScreens` + `ui_stubs.c` + `test_sls_screens.cpp` re-wired to the authoritative exported symbols
  (Setup/Busy/Message groups; renamed Error/OTA labels).
- **[x] [wo4-configuring-orchestration](wo4-configuring-orchestration/WORKORDER.md)** *(architect)* — ✅ `987f06f`.
  Added `ILVGLSystemScreens::onConfiguring(payload)`; `_onAppConfiguring` three-tier dispatch mirroring U8G2.
  Native 417/417, crowpanel [SUCCESS]. **Framework side of #82.**
- **[x] [wo5-sls-configuring-skin](wo5-sls-configuring-skin/WORKORDER.md)** *(standard)* — ✅ `f15ffd9`.
  `SLSSystemScreens` drives Setup/Busy/Message from the payload (`gateHelperScreen` defers WiFi to Tier-2);
  5 tests added. crowpanel [SUCCESS]. Closes #82 on merge (`Fixes #82`).
- **[x] [wo6-native-lvgl-tests](wo6-native-lvgl-tests/WORKORDER.md)** *(standard)* — ✅ `4d16901`.
  Enabled the dormant `test_lvglDisplay` suite on native (stateful lvgl stub, `IBootReporter.hpp`→`include/`,
  env-conditional `ui/` exclusion via library extraScript, `ui_stubs.cpp`). **Native 417→482**; crowpanel &
  esp32s3nano [SUCCESS]. Closes #83 on merge (`Fixes #83`).
- **[ ] [wo7-hardware-test-debug](wo7-hardware-test-debug/WORKORDER.md)** *(architect)* — 📋 authored, **awaiting hardware**.
  Interactive on-device run of [HARDWARE-TEST-PLAN.md](HARDWARE-TEST-PLAN.md) (T1–T13) + defect debug loop; the
  human drives via [wo7-hardware-test-debug/BRIDGE-PROMPT.md](wo7-hardware-test-debug/BRIDGE-PROMPT.md). Gates the PR.

## 5. Open Questions — RESOLVED (human, 2026-08-09)

- **OQ-1 — UI C re-export (blocks WO2). ✅ Resolved:** human runs *Export → Export UI Files*
  in SquareLine Studio 1.6.1 into `modules/LVGLDisplay/ui/`; WO2 integrates it. (Option A.)
- **OQ-2 — Drive the new groups now, or defer? ✅ Resolved → routed to framework architect.**
  The human confirmed the Setup/Busy/Message groups are essential (WiFi wizard + gate helpers)
  and asked for the full system-screen contract (boot / configuring / error / OTA / config
  progress / gate helpers) at U8G2 parity. Because that needs an **additive framework-API
  change** to `ILVGLSystemScreens`, it was filed as **[#82](https://github.com/matthewmcneill/esp32-app-foundation/issues/82)**
  for the architect. **This project stays parity-only (WO1–WO3); the configuring work is a
  follow-on gated on #82.** Mapping preserved in §9.
- **OQ-3 — Prune now-unused fonts? ✅ Resolved:** WO2 enables montserrat 12/18/22/40 and
  **prunes 10/28 if a build-wide check confirms they're unreferenced** (they cost flash).
- **OQ-4 — Branch base / dirty tree + PR grouping. ✅ Resolved:** commit the pending PA
  update (planning artifacts + the ` M .agents/skills/principal-architect` submodule pointer)
  to clean the tree **before** cutting any branch. Base the stack on `sls-templates/crowpanel-128`;
  PR grouping decided at stack-submission time.

## 6. Branch Strategy

- **Base:** `sls-templates/crowpanel-128`. WO1 branches from it; WO2 from WO1; WO3 from WO2.
  No standalone feature branch — stack tip is the PR head (STACKING_REFERENCE §1).
- **No branches created in this planning phase.** Creation begins only after human
  approval of this decomposition.
- ⚠️ **Branch-safety flag:** the working tree currently shows ` M .agents/skills/principal-architect`
  (a git-submodule pointer left by commit `9338fff` "Migrate all skills to git submodules").
  It is unrelated to this work; it must be committed, stashed, or reverted **before** any
  branch is cut, per rule `01-plan-before-execute.md`. Decision needed from the human.

## 7. Verification Gates

- WO1: `git status` clean of stray files; README renders; no firmware delta.
- WO2: `crowpanel-128-rotary` compiles with the new UI + fonts (via `platformio` MCP; no flash).
- WO3: `native` tests pass **and** `crowpanel-128-rotary` compiles. No hardware flash in any WO.

## 8. Status Log

| Date | Event |
|------|-------|
| 2026-08-09 | Reviewed the Claude Design export vs. repo. Confirmed it is the SLS **project** (no C), byte-diffed widget tree: Boot/Error/OTA unchanged; Wizard→Setup (+URL semantics), Interstitial→Busy, new Message group, Diag simplified; logo → `cblabs_180x58`; fonts add 12/18/22/40. Firmware bridge references old globals → would break on re-export. |
| 2026-08-09 | Confirmed module invokes only `errorScreen`/`otaScreen`/`onError`/`onOtaProgress`; the new groups are not reachable without module-core changes → scoped this PR to **parity adoption**, deferred group-driving (OQ-2). |
| 2026-08-09 | Decomposition drafted (3-WO stack) + WORKORDERs authored. **No branches, no workers.** Plan gate: awaiting human approval of the decomposition and resolution of OQ-1…OQ-4. |
| 2026-08-09 | Human resolved OQ-1/3/4 and **expanded OQ-2** — wants the full system-screen contract (boot/configuring/error/OTA/config-progress/gate-helpers) at U8G2 parity. Research: LVGL `_onAppConfiguring` is a `TODO` stub (LVGLDisplayModule.cpp:558); LVGL has no built-in configuring screens; U8G2 reference = three-tier modal dispatch (U8G2DisplayModule.cpp:1179). Worked out the SLS group ↔ contract mapping (§9). Clarified the "2 error screens" (built-in `ErrorScreen` fallback vs SLS Error group; + the new Message group is the *generic gate helper*, not a second error). |
| 2026-08-09 | Configuring work needs an additive `ILVGLSystemScreens` change → **filed [#82](https://github.com/matthewmcneill/esp32-app-foundation/issues/82)** (P2-correctness) for the framework architect. Configuring orchestration **deferred to a follow-on gated on #82**; this project re-settled to the parity-only 3-WO stack. |
| 2026-08-09 | Human: "we are the framework architect, so design it and proceed." Took **#82 directly** (D1 `onConfiguring`, D2 no built-in LVGL fallback, D3 spinner progress; commented on #82). Configuring folded back in → 5-WO stack. Human granted **autonomous execution**. |
| 2026-08-09 | **WO1–WO5 executed inline & committed** on the stack (base `sls-templates/crowpanel-128` → wo1…wo5). Human ran the SLS UI export (OQ-1). Verified each WO: native **417/417**, crowpanel-128-rotary **[SUCCESS]** (Flash 43.2%, 1.81 MB). Transient `littlefs` component-manager race on one build (self-resolved). SLS project source synced to the human's in-editor tweaks (`64b3c26`). |
| 2026-08-09 | **Finding → filed [#83](https://github.com/matthewmcneill/esp32-app-foundation/issues/83)** (P2-correctness): `test_unit/test_lvglDisplay` is `lib_ignore`/`test_ignore`d on native (since `f5d459a`), so the LVGL display suite — incl. the SLS bridge/configuring tests (WO13's + this PR's) — has **never executed**. Production verified via crowpanel compile; tests dormant/correct-by-construction. |
| 2026-08-09 | Human asked to evaluate reviving native LVGL tests. Investigated (empirical trial): stub infra ~95% present; blockers small. Posted feasibility to #83 → human said "proceed". |
| 2026-08-09 | **WO6 executed & committed (`4d16901`) — enabled the native `test_lvglDisplay` suite (Fixes #83).** Moved `IBootReporter.hpp`→`include/interfaces/`; made `lv_label` stubs stateful (+`lv_obj_remove_flag`,`LV_IMG_DECLARE`); env-conditional `ui/` exclusion via a LVGLDisplay library `extraScript` (hardware compiles generated `ui/`, native uses `ui_stubs`); moved `ui_stubs.c`→ suite as `ui_stubs.cpp` (`extern "C"`). Native **417→482**; crowpanel & esp32s3nano [SUCCESS]. Generated `ui/` stays hardware/visual-verified by design; native covers our display logic. |
| 2026-08-09 | **Stack complete (6 WOs), not yet submitted.** Remaining: open the stacked PR(s) (tip `4d16901`, `Fixes #82`, `Fixes #83`), decide PR grouping (OQ-4), and a hardware smoke-test on a physical CrowPanel (no flashing done). |
| 2026-08-09 | Filed **[#84](https://github.com/matthewmcneill/esp32-app-foundation/issues/84)** (dormant `test_u8g2Display`/`test_application`, per-suite recommended fixes). Authored **HARDWARE-TEST-PLAN.md** (T1–T13, flash-gate, capture_display verification). |
| 2026-08-09 | **Human holds the PR pending a hardware run.** Authored **WO7** (interactive hardware test + debug) + a **BRIDGE-PROMPT.md** for a fresh interactive worker session the human drives at the bench. No branch cut / no worker spawned — awaits the physical CrowPanel. |

---

## 9. Deferred: LVGL CONFIGURING orchestration (tracked by #82)

Not in this project's scope, but the design was worked out and is preserved here so
the follow-on (once #82 is resolved) starts warm. The redesigned SLS groups map 1:1
onto the framework's system-screen contract and U8G2's three-tier dispatch:

| Contract screen | Framework trigger | U8G2 reference board | LVGL SLS group |
|-----------------|-------------------|----------------------|----------------|
| Boot | boot | built-in boot | Boot group (exists) |
| Configuring / config progress | `APP_CONFIGURING`, `InProgress` | `ConfiguringStatusBoard` | **Busy** group (spinner + status) |
| WiFi wizard | `NeedsUserAction` + `GateNames::WIFI_STATION` | `WiFiWizardBoard` | **Setup** group (SSID + portal URL) |
| Gate helper (generic) | `NeedsUserAction` + other gate | `GenericGateHelperBoard` | **Message** group (icon/title/body) |
| Error | `showError` | `ErrorBoard` | Error group (wired) |
| OTA | `OTA_PROGRESS` | `OTAProgressBoard` | OTA group (wired) |

Reference dispatch: `U8G2DisplayModule::_onAppConfiguring` (modules/U8G2Display/U8G2DisplayModule.cpp:1179)
— suspend base, show configuring status board, update connection info (`hostname`→SSID,
`deviceIP`→`http://…` URL); modal only when `actionStatus == NeedsUserAction`; Tier 1
`gateHelperBoard(p)` → Tier 2 WiFi wizard for `WIFI_STATION` → Tier 3 generic helper.

Follow-on shape once #82 lands (indicative, ~2 WOs): (a) `LVGLDisplayModule::_onAppConfiguring`
three-tier dispatch selecting Setup/Busy/Message via visibility groups on `ui_uiSystemScreen`;
(b) `SLSSystemScreens` implements `onConfiguring(payload)` + `configuringScreen()`/
`wifiWizardScreen()`/`gateHelperScreen(payload)` populating those groups from the payload;
(c) native tests. LVGL uses a single System screen with visibility groups, so the U8G2
"base + modal layering" collapses to "which group is visible" — simpler than U8G2.

**Note on "2 error screens":** the built-in `ErrorScreen` (modules/LVGLDisplay/screens/ErrorScreen.cpp)
is only a fallback, bypassed when a skin provider is present (LVGLDisplayModule.cpp:298). The
two error-*looking* SLS panels are the **Error** group (fatal error) and the **Message** group
(generic gate helper) — distinct lifecycle roles, not a duplicate.
