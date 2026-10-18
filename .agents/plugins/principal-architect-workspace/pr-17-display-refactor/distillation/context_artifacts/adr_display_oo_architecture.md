# ADR — Display Module OO Architecture (D34–D38)

**Session**: 0735b228-f5fb-4a7a-b7e7-4e2807bebae0
**Date**: 2026-06-03
**Status**: Resolved — written to `docs/design_specification.md §17`

---

## Problem Statement

`U8G2DisplayModule` was a God Object exhibiting the Orchestrator Anti-Pattern:
- Held `Application*` as a member variable
- Polled `AppState`, `WifiManager`, and `ConfigGates` inside `tick()`
- Exposed boards via flat concrete getter methods (`getBootBoard()`, `getWifiWizard()`, etc.)
- Required callers to three-step orchestrate: configure sub-object + take address + `pushModal()`
- Pushed `ScreensaverBoard` via `pushModal()` — assigning it MODAL priority (suppressed OTA overlay)

## Decisions

### D34 — Display module redesigned as pure service
`Application*` is removed from `U8G2DisplayModule`. Application layer owns all
"when to show what" decisions. Module owns only: render loop, carousel timing,
screensaver inactivity policy.

**Why**: A hardware presenter has no business reading business service state
(`WifiManager.getAPMode()`, `ConfigGates`). The comment in the header — "Polls
Application state in tick() eliminating EventBus race conditions" — was an application
timing problem solved in the wrong layer.

### D35 — Hierarchical COM-style sub-object API
Three navigable sub-objects replace flat getters:
- `display.carousel` — `CarouselEngine`
- `display.boards.*` — `BoardCatalog` with concrete typed references
- `display.screensaver` — `ScreensaverController`

**Why**: The COM/VB mental model (`myForm.dropDown.Items.Add()`) is the right
abstraction. Flat getters returned concrete types with no hierarchy and required
three-call orchestration to achieve one logical effect (the Two-Step Smell).

### D36 — Concrete types at caller layer; `IDisplayBoard*` only at engine boundaries
Callers access boards as concrete types (`OTAProgressBoard&`, `WiFiWizardBoard&`).
`IDisplayBoard*` appears only at `showModal()` and `carousel.add()`.

**Why**: Returning `IDisplayBoard&` to callers destroys the hierarchy — the caller
cannot call `setProgress()` on an interface. Polymorphism is needed at the render
loop; it is actively harmful at the configuration layer.

**Rejected approach**: Exposing widget sub-objects (e.g. `clockBoard.timeWidget.setManager()`)
was considered but rejected. `LabelWidget`, `ProgressBarWidget` are rendering primitives
(implementation topology), not domain concepts. They fail the Domain Meaning Test.

### D37 — Screensaver is a distinct display-intrinsic policy layer, not a modal
Three explicit priority layers: Modal → Base/Carousel → Screensaver.
Screensaver is the lowest layer, driven by inactivity timer, not application decision.

**Why**: Pushing `ScreensaverBoard` via `pushModal()` assigned it MODAL priority,
allowing it to suppress OTA progress overlays and WiFi wizard during inactivity.
The screensaver is display-intrinsic (hardware concern — dim, blank, power save);
it does not require application domain state to operate.

**Distinction confirmed**: Display-intrinsic policy (screensaver inactivity timer,
brightness management) is acceptable autonomy in the module. App-intrinsic decisions
("show WiFi wizard when AP starts") must be pushed upstream. The test: "does this
require knowledge of application domain state?"

### D38 — Board taxonomy: system, user-surfaced, display-intrinsic
- **System boards** (`BootBoard`, `ConfiguringStatusBoard`): not in `display.boards`;
  Application drives via `showBase()` / `reportStatus()`
- **User-surfaced boards** (`OTAProgressBoard`, `WiFiWizardBoard`, `ErrorBoard`):
  in `display.boards.*` with concrete type API
- **Display-intrinsic** (`ScreensaverBoard`): in `display.screensaver.setBoard()`
- **App boards**: registered via `display.carousel.add()`

**Why**: Not all boards should be externally configurable. Exposing system boards via
getters invited callers to bypass the module's lifecycle management. App-specific
content boards belong in the carousel, not as named properties.

## What Was Explicitly Rejected

| Approach | Reason Rejected |
|---|---|
| Widget sub-object exposure (`clockBoard.timeWidget.setManager()`) | Widgets are rendering primitives, not domain concepts. Fails Domain Meaning Test |
| IDisplayBoard& return to callers | Destroys caller's ability to use board-specific APIs |
| Screensaver as a modal | Wrong priority tier; suppresses legitimate overlays |
| Module polling AppState/WifiManager | Orchestrator Anti-Pattern; application logic in wrong layer |
| "EventBus race condition" fix staying in display module | Race condition is an application timing problem; solving it in the display module is the wrong layer |
