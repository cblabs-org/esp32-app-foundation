---
name: sls-screen-redesign-wo4
description: Handoff instructions for Work Order 4 — Extend ILVGLSystemScreens + wire LVGL CONFIGURING three-tier dispatch (Fixes #82, framework side)
complexity: architect
---

# WO4 — LVGL CONFIGURING Orchestration (framework capability)

### Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Give the LVGL display path CONFIGURING support at U8G2 parity: add one callback to the
`ILVGLSystemScreens` interface and implement `LVGLDisplayModule::_onAppConfiguring` as a
three-tier modal dispatch that drives a system-screen provider. This is the **framework**
side (interface + module); the SLS skin implementation is WO5. Closes the `_onAppConfiguring`
TODO left by #66/#60 for LVGL. **Fixes #82.**

## Design decisions (locked by architect on #82 — do not revisit)
- **D1:** add `virtual void onConfiguring(const AppConfiguringPayload*)` to `ILVGLSystemScreens`, default no-op, mirroring `onError`/`onOtaProgress`. Screen-factory signatures unchanged.
- **D2:** no built-in LVGL configuring screens. When `_screens == nullptr`, the module logs and returns (SLS/skin is required for configuring visuals on LVGL).
- **D3:** config progress is an indeterminate spinner (skin's Busy group), not a determinate bar.

## Context
- **Branch:** `sls-screen-redesign/wo4-configuring-orchestration` (based on WO3).
- Reference implementation: `U8G2DisplayModule::_onAppConfiguring` (modules/U8G2Display/U8G2DisplayModule.cpp:1179) — base board + Tier 1 `gateHelperBoard(p)` → Tier 2 WiFi wizard for `GateNames::WIFI_STATION` → Tier 3 generic; modal only when `actionStatus == NeedsUserAction`.
- `include/interfaces/ILVGLSystemScreens.hpp` already declares `configuringScreen()`, `wifiWizardScreen()`, `gateHelperScreen(const AppConfiguringPayload*)` (all default nullptr) — the module just never called them.
- Module currently: `_onAppConfiguring` (LVGLDisplayModule.cpp:551) logs only; `_screens` provider member; `_inSystemScreen`/`_priorScreen` screen-swap bookkeeping (see `showError`/`restorePriorScreen`).
- `GateNames::WIFI_STATION` + `GateActionStatus` live in `include/interfaces/IConfigGate.hpp`.

## Strict Boundary Box
- Do **NOT** modify `modules/LVGLDisplay/screens/SLSSystemScreens.*` or the SLS `ui/` files — the skin overrides are WO5.
- Do **NOT** change `onError`/`onOtaProgress`/`errorScreen`/`otaScreen` behaviour.
- Do **NOT** add built-in LVGL configuring/wizard screens (D2).
- Do **NOT** flash. Compile + native tests only.

## Tooling Constraints
- Read `pio-manager`; build via `platformio` MCP (`build_project` env `crowpanel-128-rotary`, `run_tests` env `native`). Rule `04-mcp-tooling.md`.

## Line-Item Instructions
- [ ] `include/interfaces/ILVGLSystemScreens.hpp`: add `virtual void onConfiguring(const AppConfiguringPayload* payload) { (void)payload; }` with doc comment; update the header's method-summary block. (Apache-2.0 header stays.)
- [ ] `modules/LVGLDisplay/LVGLDisplayModule.cpp`: add `#include <cstring>` and `#include "interfaces/IConfigGate.hpp"` (for `strcmp` + `GateNames`/`GateActionStatus`) if not already visible.
- [ ] Replace the `_onAppConfiguring` TODO body with three-tier dispatch:
  - If `_screens == nullptr` → `ESP_LOGI(...)` and return (D2).
  - Call `_screens->onConfiguring(p)` (skin populates widgets from payload).
  - If `p->actionStatus == GateActionStatus::NeedsUserAction`: `scr = _screens->gateHelperScreen(p)`; if null and `p->gateName` == `GateNames::WIFI_STATION` → `scr = _screens->wifiWizardScreen()`; if null → `scr = _screens->configuringScreen()`.
  - Else (`InProgress`) → `scr = _screens->configuringScreen()`.
  - If `scr != nullptr` and `scr != lv_screen_active()`: set `_priorScreen`/`_inSystemScreen` (as `showError` does) and `lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false)`. **Guard against re-animating the same screen** on each configuring re-publish.
- [ ] Confirm `_onAppRunning` still transitions out (loads Diag screen); reset `_inSystemScreen=false` there for consistency.
- [ ] `run_tests` native (existing suite must stay green; the module isn't native-built but the interface change must not break native).
- [ ] `build_project` crowpanel-128-rotary — compiles clean (module no-ops without a provider that overrides the configuring methods; that's WO5).

## Verification Requirement
Paste native + crowpanel results. **Before committing, ask the user if they are happy** (waived under standing autonomous approval). Commit: `feat(lvgl): add onConfiguring + wire _onAppConfiguring three-tier dispatch (#82)`.

### Issue Surfacing
Surface out-of-boundary findings in the Completion Report; do not file issues directly.

## Provenance
Part of **PR: SLS Screen Redesign** ([PROJECT.md](../PROJECT.md)); decomposition + scope-expansion approved by the human (PROJECT.md §8). Implements framework issue #82.

---
## Completion Report
<!-- Executed inline by the PA acting as architect, autonomous approval 2026-08-09 -->

### Status
complete

### Changes Made
- `include/interfaces/ILVGLSystemScreens.hpp` — added `virtual void onConfiguring(const AppConfiguringPayload*)` (default no-op) + header method-summary entry (D1). Additive, non-breaking, Apache-2.0 header intact.
- `modules/LVGLDisplay/LVGLDisplayModule.cpp` — added `#include <cstring>` + `#include "interfaces/IConfigGate.hpp"`; replaced the `_onAppConfiguring` TODO with the three-tier dispatch (D2 no-provider no-op → `onConfiguring(p)` → NeedsUserAction: gateHelper→wizard(WIFI_STATION)→configuring; else configuring base); load-guard so re-publishes on the same System screen don't re-animate. `_onAppRunning` now clears `_inSystemScreen`/`_priorScreen`.

### Decisions Made
- Screen-load guard `scr != lv_screen_active()`: the provider's factory methods toggle visibility groups in place on the single System screen, so the fade animation runs only on first entry; group/text still update every re-publish (factory + `onConfiguring` run unconditionally).

### Verification
- `run_tests` native → **417/417** (interface change additive; native unaffected).
- `build_project` crowpanel-128-rotary → **[SUCCESS]** 63.67s, Flash 43.2% (1,812,775 B, +172 B vs WO3). Module no-ops until WO5 provides the overrides.

### Out-of-Scope Findings
- (none)

### Blockers
(none)
