# PR — SLS Screen Redesign (CrowPanel 1.28")

> Adopt the reviewed SquareLine Studio redesign into the repo: refresh the SLS
> project source, re-export the LVGL UI C, and re-wire the framework bridge to the
> renamed/added widget groups — at **behaviour parity** with the current build.

**Base branch:** `sls-templates/crowpanel-128` (the WO13 wiring lives here, not on `main`)
**Design authority:** the reviewed SLS export at `/Users/mcneillm/Downloads/sls-project`
(delivered by Claude Design after an interactive review pass)
**Predecessor:** [pr-47-lvgl-display](../pr-47-lvgl-display/PROJECT.md) WO13 (SLSSystemScreens wiring)
**Related issue:** [#82](https://github.com/matthewmcneill/esp32-app-foundation/issues/82) — LVGL CONFIGURING orchestration + `ILVGLSystemScreens` extension (with framework architect; **blocks the deferred configuring work**, see §5 OQ-2 / §9)
**Status:** 📋 PLANNED — decomposition drafted, **awaiting human approval**. No branches, no workers spawned.

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

- **[ ] [wo1-ingest-sls-project](wo1-ingest-sls-project/WORKORDER.md)** *(mechanical)* —
  Copy the reviewed `.spj`/`.sll`/`.slt` + `cblabs_180x58.png` into
  `squareline/crowpanel-128/`, drop the stale `cblabs_320x100.png`, and update the
  project `README.md`. No firmware touched. *Independent of the SLS export gate.*
- **[ ] [wo2-integrate-ui-export](wo2-integrate-ui-export/WORKORDER.md)** *(standard)* —
  **Precondition: OQ-1 resolved** (user supplies a fresh SquareLine "Export UI Files"
  of the revised project). Replace `modules/LVGLDisplay/ui/`, swap the compiled logo
  asset, enable montserrat 12/18/22/40 in `lv_conf.h`, reconcile `filelist.txt`/
  `CMakeLists.txt`. Compile-verify `crowpanel-128-rotary`.
- **[ ] [wo3-rewire-firmware](wo3-rewire-firmware/WORKORDER.md)** *(standard)* —
  Update `SLSSystemScreens.hpp/.cpp` (Wizard→Setup, Interstitial→Busy, add Message
  group to `hideAllGroups`), rewrite native `ui_stubs.c` to the new symbol set, update
  `test_sls_screens.cpp`. Native tests + `crowpanel-128-rotary` green.

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
