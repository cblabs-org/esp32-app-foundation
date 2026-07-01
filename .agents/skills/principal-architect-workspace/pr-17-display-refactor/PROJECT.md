---
id: pr-17
title: "U8G2 Display Module OO Refactoring"
status: "✅ COMPLETE"
branch: "pr-17-display-refactor"
depends_on: []
design_spec: "docs/design_specification.md#78-display-module-object-model-target-architecture"
decisions: "D34, D35, D36, D37, D38"
---

# PR-17: U8G2 Display Module OO Refactoring

## Goal

Refactor `modules/U8G2Display/U8G2DisplayModule` from a God Object / Orchestrator
anti-pattern into a pure service module with a hierarchical COM-style object model.
Full target architecture specified in `docs/design_specification.md §7.8`.

## Key Architectural Contracts (Non-Negotiable)

Before writing any code, the worker MUST read `docs/design_specification.md §7.8` in full.
The following are invariants — do not deviate without user approval:

1. `Application*` is removed from `U8G2DisplayModule` member variables
2. `AppState` polling is removed from `tick()` entirely
3. `display.carousel`, `display.boards`, `display.screensaver` are exposed as public sub-objects
4. `IDisplayBoard*` appears only at engine boundaries (`showModal`, `carousel.add`) — never returned to callers
5. `ScreensaverBoard` is NOT pushed via any modal mechanism; it is managed by `ScreensaverController`
6. All `Application::onEnter*` / `onWifi*` call sites drive the display; the display does not poll

## Work Orders

### WO1 — Extract `CarouselEngine`
**Files**: New `modules/U8G2Display/CarouselEngine.hpp/.cpp`
**Scope**:
- Extract carousel pool (`std::vector<IDisplayBoard*>`), rotation timing, suspend/resume, and `setInterval()` from `U8G2DisplayModule`
- Public API: `add(IDisplayBoard*)`, `show(IDisplayBoard*)`, `suspend()`, `resume()`, `setInterval(ms)`, `tick(ms)`, `currentBoard()`
- `U8G2DisplayModule` gains a `CarouselEngine _carousel` member; `display.carousel` is its public reference
- All existing carousel logic in `U8G2DisplayModule.cpp` delegated to `_carousel`
- No API changes visible to callers yet (WO4 handles that)

### WO2 — Create `BoardCatalog`
**Files**: New `modules/U8G2Display/BoardCatalog.hpp`
**Scope**:
- Thin struct/class holding concrete typed references to user-surfaced boards:
  `BootBoard& boot`, `WiFiWizardBoard& wifiWizard`, `OTAProgressBoard& otaProgress`, `ErrorBoard& error`
- Boards remain owned by `U8G2DisplayModule` as member variables (no heap allocation)
- `BoardCatalog` is initialised in `U8G2DisplayModule` constructor via reference binding
- `display.boards` becomes the public accessor: `U8G2DisplayModule::boards` is a `BoardCatalog` member

### WO3 — Create `ScreensaverController`
**Files**: New `modules/U8G2Display/ScreensaverController.hpp/.cpp`
**Scope**:
- Owns inactivity timer, dim/blank phase state machine, brightness management
- Public API: `setDimTimeout(ms)`, `setBlankTimeout(ms)`, `setDimLevel(uint8_t)`, `setPolicy(ScreensaverPolicy)`, `setBoard(IDisplayBoard*)`, `resetActivity()`, `isActive()`, `isDimmed()`, `isBlanked()`, `tick(ms)`
- `ScreensaverPolicy` enum: `DimOnly`, `FullSleep`
- Takes `U8G2&` reference for brightness/power-save calls
- Replaces `_checkSleep()`, `_checkDim()`, `_sleeping`, `_sleepDiagLogged`, `_modalDimmed` in `U8G2DisplayModule`
- `display.screensaver` is the public accessor

### WO4 — Refactor `U8G2DisplayModule` Root
**Files**: `modules/U8G2Display/U8G2DisplayModule.hpp`, `modules/U8G2Display/U8G2DisplayModule.cpp`
**Scope** (after WO1–3 complete):
- Remove `Application* _app` member and all usages
- Remove `AppState _lastKnownState` member
- Remove `_checkSleep()`, `_checkDim()`, `_modalDimmed`, `_sleeping`, `_sleepDiagLogged` (now in `ScreensaverController`)
- Add `CarouselEngine _carousel`, `BoardCatalog _boards`, `ScreensaverController _screensaver` members
- Add public presentation API: `showModal(IDisplayBoard*)`, `clearModal()`, `showBase(IDisplayBoard*)`, `resetActivity()`
- Simplify `tick()` to: `_screensaver.tick(ms)` → `_carousel.tick(ms)` → `_renderCurrentBoard()`
- Remove all flat getter methods: `getBootBoard()`, `getWifiWizard()`, `getOtaProgress()`, `getScreensaver()`, `getError()`, `getConfiguringStatus()`, `pushModal()`, `clearModal()` (replaced by new API)
- Publish `display.sleep`, `display.wake` events via EventBus on screensaver transitions

### WO5 — Update All Call Sites
**Files**: `modules/application/Application.cpp`, any consuming app `src/main.cpp`
**Scope**:
- `Application.cpp`: Add `onEnterConfiguring()`, `onWifiAPStarted()`, `onWifiConnected()`, `onEnterRunning()` helpers that call the new display API
- Wire these into the `AppState` state machine transitions in `Application::tick()`
- Replace any surviving `display.getBootBoard()` / `display.pushModal()` usages with the new API
- Update `IStatusReporter` implementation in `U8G2DisplayModule` to use `showModal` / `clearModal` internally
- Grep command: `git grep -n "getBootBoard\|getWifiWizard\|pushModal\|getOtaProgress\|getScreensaver\|getConfiguringStatus\|getError"`

## Dependency Order

```
WO1 (CarouselEngine) ──┐
WO2 (BoardCatalog)   ──┼──→ WO4 (Module root refactor) ──→ WO5 (Call sites)
WO3 (ScreensaverCtrl)──┘
```

WO1–3 can be executed concurrently. WO4 requires WO1–3 merged. WO5 requires WO4 merged.

## Verification Plan

- Native build must pass: `pio run -e native`
- Hardware build must pass: `pio run -e esp32dev` (or target board)
- `git grep "getBootBoard\|getWifiWizard\|_app->get"` must return zero hits in `U8G2Display/`
- Manually verify carousel rotation, WiFi wizard modal, OTA progress modal, screensaver dim/blank cycle
- Verify EventBus `display.sleep` / `display.wake` events fire correctly

## Notes for Worker Agent

- House style: read `.agents/skills/house-style-docs/references/cpp-embedded.md` before writing headers
- Do NOT introduce heap allocation for the new sub-objects — they are member variables
- `ScreensaverController` takes `U8G2&` by reference in `tick()`, not stored — avoids circular dependency
- The `_overrideBoard` pointer pattern (§7.2) is replaced by the explicit Modal layer in WO4
