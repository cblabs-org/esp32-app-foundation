# PROJECT.md — PR-05a: Widget Toolkit
# ESP32 App Foundation — Composable UI Widget System

**Status:** IN PROGRESS  
**Branch:** `pr-05a-widget-toolkit`  
**Principal Architect Session:** 21161294-94bd-4e89-910b-45d3edb870e6  
**Chief Architect Session:** f4e779d9-4aa1-46e2-8115-e1e8edc4fa42  
**Depends on:** PR-05 (U8G2 Display Module) — MERGED  
**Last Updated:** 2026-05-21

---

## Architectural Overview

PR-05 established the U8G2 display module with a carousel engine and 6 system
boards. However, system boards currently render directly to U8G2 via hand-coded
`u8g2.setFont()`, `u8g2.drawStr()`, and `u8g2.getStrWidth()` calls with manual
pixel math. This is fragile, repetitive, and prevents reuse.

PR-05a introduces a **composable widget system** — a layered toolkit that
allows boards to compose their layouts from reusable, independently testable
graphical components.

### Three-Tier Architecture

```
┌──────────────────────────────────────────────────────────┐
│  Tier 3: Module-Aware Widgets                            │
│  (wifiStatusWidget, systemMessageWidget,                 │
│   scrollingMessagePoolWidget)                            │
│  Depend on framework services (EventBus, WiFi state)     │
├──────────────────────────────────────────────────────────┤
│  Tier 2: Generic Widgets                                 │
│  (labelWidget, scrollingTextWidget, clockWidget,         │
│   progressBarWidget, imageWidget)                        │
│  Pure graphical, reusable by any app                     │
├──────────────────────────────────────────────────────────┤
│  Tier 1: Primitives                                      │
│  (iGfxWidget, drawingPrimitives, U8g2StateSaver,         │
│   iBoardLayout, font management)                         │
│  Zero dependencies beyond U8G2                           │
└──────────────────────────────────────────────────────────┘
```

### File Layout

All widget toolkit files live within the existing display module directory:

```
modules/u8g2Display/
  widgets/
    iGfxWidget.hpp               ← Abstract base class
    drawingPrimitives.hpp/.cpp   ← Stateless U8G2 utility wrappers
    labelWidget.hpp/.cpp         ← Static text with alignment
    scrollingTextWidget.hpp/.cpp ← Horizontal marquee scroller
    clockWidget.hpp/.cpp         ← Real-time clock display
    progressBarWidget.hpp/.cpp   ← Animated progress indicator
    imageWidget.hpp/.cpp         ← XBM bitmap renderer
    wifiStatusWidget.hpp/.cpp    ← WiFi signal/disconnect icon
    systemMessageWidget.hpp/.cpp ← Multi-line system alert overlay
    scrollingMessagePoolWidget.hpp/.cpp ← EventBus-driven message rotator
  boards/
    interfaces/
      iBoardLayout.hpp           ← Board view interface (decouples rendering)
  fonts/
    fonts.hpp/.cpp               ← Foundation font declarations + data
```

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| WD-01 | `iGfxWidget` uses concrete x/y/w/h members (not virtual getters) | Zero vtable overhead for geometry; matches departures-board proven pattern |
| WD-02 | `drawingPrimitives` are free functions (not a class) | Stateless utilities; no instance needed; matches existing pattern |
| WD-03 | `U8g2StateSaver` is RAII (constructor saves, destructor restores) | Exception-safe, prevents display state corruption |
| WD-04 | `iBoardLayout` injects `Application*` not `appContext*` | Foundation uses `Application` hub, not departures-board's `appContext` |
| WD-05 | Foundation fonts use standard U8G2 built-in fonts initially | No custom font byte arrays needed for a generic framework |
| WD-06 | `scrollingMessagePoolWidget` consumes EventBus, not `MessagePool` | PROGRAM-PLAN mandates no MessagePool; use EventBus.publish("display.message") |
| WD-07 | `__FlashStringHelper` overloads excluded from foundation | Foundation is framework-level; flash strings are app concern |
| WD-08 | `SCREEN_WIDTH` / `SCREEN_HEIGHT` replaced with runtime queries | Foundation supports multiple display sizes; no compile-time `#define` |
| WD-09 | Native build guards (`#ifndef NATIVE_BUILD`) on all U8G2 calls | Maintains testability on host CPU; matches PR-05 pattern |

### Discovered Standards (carried from PR-05)

| # | Discovery | Applied |
|---|---|---|
| D-01 | `CONFIG_AUTOSTART_ARDUINO=y` in sdkconfig.defaults | ✅ Already in repo |
| D-02 | `lib_extra_dirs = modules` in both `[base_esp32]` and `[env:native]` | ✅ Already in repo |
| D-03 | WO branch names must be FLAT (no slashes) | ✅ Enforced: `pr-05a-wo1-primitives` |
| D-04 | PlatformIO LDF ignores preprocessor guards — Application.hpp excluded from display module native builds | ✅ Already resolved in PR-05 |

---

## Work Order Ledger

| WO | Branch | Status | Topic |
|---|---|---|---|
| **[ ] [wo1-primitives](wo1-primitives/WORKORDER.md)** | `pr-05a-wo1-primitives` | READY | `iGfxWidget`, `drawingPrimitives`, `U8g2StateSaver`, `iBoardLayout`, font management |
| **[ ] [wo2-generic-widgets](wo2-generic-widgets/WORKORDER.md)** | `pr-05a-wo2-generic-widgets` | BLOCKED (WO1) | `labelWidget`, `scrollingTextWidget`, `clockWidget`, `progressBarWidget`, `imageWidget` |
| **[ ] [wo3-module-widgets](wo3-module-widgets/WORKORDER.md)** | `pr-05a-wo3-module-widgets` | BLOCKED (WO2) | `wifiStatusWidget`, `systemMessageWidget`, `scrollingMessagePoolWidget` + system board refactor |

### Concurrency Analysis

All three WOs are strictly serial — each tier builds on the previous:
- WO2 depends on `iGfxWidget` and `drawingPrimitives` from WO1
- WO3 depends on generic widgets from WO2 AND refactors boards that use them

**No parallel execution possible.** Must be serialized in order.

---

## Reference Mining Summary

Mined from `../departures-board/modules/displayManager/widgets/`:

| Source File | Patterns Extracted |
|---|---|
| `iGfxWidget.hpp` | Abstract base (x, y, w, h, visibility), `tick()` / `render()` / `renderAnimationUpdate()` lifecycle |
| `drawingPrimitives.hpp/.cpp` | `TextAlign` enum, `U8g2StateSaver` RAII, `blankArea()`, `getStringWidth()`, `drawText()` with alignment/truncation, `drawTriangle()` |
| `scrollingTextWidget.hpp/.cpp` | Marquee math: pixel-shift with `tick()`, 3s initial delay, ~25 FPS pacing, `renderAnimationUpdate()` partial refresh, `isScrollFinished()` loop detection |
| `clockWidget.hpp/.cpp` | `TimeManager*` DI, `ClockFormat` enum, blink state machine, `renderAnimationUpdate()` for colon blink |
| `progressBarWidget.hpp/.cpp` | Animated percent interpolation, `setMessage()` + `setPercent()` with animation duration, `showPercentText` toggle |
| `labelWidget.hpp/.cpp` | Static text with `TextAlign`, truncation toggle, font override |
| `imageWidget.hpp/.cpp` | XBM bitmap with PROGMEM pointer |
| `wifiStatusWidget.hpp/.cpp` | RSSI category bars, disconnection blink, connection state tracking |
| `systemMessageWidget.hpp/.cpp` | Multi-line overlay (title + 3 lines), full-screen modal |
| `scrollingMessagePoolWidget.hpp/.cpp` | Pool rotation, interleaved messages (adapt to EventBus instead of MessagePool) |
| `iBoardLayout.hpp` | Board view interface with `tick()` / `render()` / `renderAnimationUpdate()` — adapt context injection to `Application*` |
| `fonts.hpp` | Font extern declarations pattern (ODR compliance) |

**NOT carried over:**
- `trainFormationWidget` — app-specific
- `weatherWidget` — app-specific
- `locationAndFiltersWidget` — app-specific
- `serviceListWidget` — app-specific
- `MessagePool` dependency — replaced with EventBus

---

*Principal Architect — PR-05a Widget Toolkit — 2026-05-21*
