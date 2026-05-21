# ADR: WO2 Generic Widget Architecture Decisions

## 1. iGfxWidget Inheritance Pattern

All five generic widgets extend `iGfxWidget` directly:
- `labelWidget` — static text
- `scrollingTextWidget` — horizontal marquee
- `clockWidget` — real-time HH:MM / HH:MM:SS
- `progressBarWidget` — animated bar with interpolation
- `imageWidget` — static XBM bitmap

Each widget follows the contract: `tick(uint32_t)` for logic, `render(U8G2&)` for full-frame, and optionally `renderAnimationUpdate(U8G2&, uint32_t)` for partial-screen refresh.

## 2. NATIVE_BUILD Guard Strategy (Extended from WO1)

All `.cpp` files use a split guard approach:
- **Constructor, setters, and tick() logic**: Always compiled (no U8G2 dependency)
- **render() and renderAnimationUpdate()**: Wrapped in `#ifndef NATIVE_BUILD ... #endif`
- **Native stubs**: Empty `render(U8G2&)` bodies provided in the `#else` branch

This allows GoogleTest to validate all state management, animation math, and text handling without U8G2.

## 3. TimeManager Dependency Injection (clockWidget)

**Decision**: clockWidget receives `TimeManager*` via constructor, NOT a global reference.

**Rationale**: The `esp32-app-foundation` is a generic framework. Unlike the departures-board which had a global `updateCurrentTime()`/`getCurrentTime()` pair, this project uses the `TimeManager` kernel service which exposes `getLocalTime(struct tm*)` and `isTimeSet()`. Constructor injection keeps the widget testable with nullptr.

**Impact**: Tests pass nullptr for the TimeManager pointer; the widget guards all `timeMgr->` calls with null checks.

## 4. Runtime Display Dimension Queries

**Decision**: NO `SCREEN_WIDTH` or `SCREEN_HEIGHT` constants anywhere.

**Rationale**: The framework must support arbitrary display sizes. All widgets query `display.getDisplayWidth()` and `display.getDisplayHeight()` at render time.

**Fallback pattern**: When `width == -1` (auto), widgets use `display.getDisplayWidth()` as the fallback. In tick() (which has no display reference), a conservative fallback of 256px is used.

## 5. scrollingTextWidget Protected Members

**Decision**: Key state variables (`currentText`, `scrollX`, `messageWidth`, `needsScroll`, `needsLayout`, `font`, `scrollFinished`) are `protected` not `private`.

**Rationale**: This widget is designed as a base class for domain-specific scrolling widgets (e.g., a departures-board `destinationWidget` could extend it to add data loading on scroll completion). Private members would force derived classes to duplicate state management.

## 6. Integer-Math Interpolation (progressBarWidget)

**Decision**: Animation interpolation uses `(long)range * (long)elapsed / (long)animDurationMs` — no floating-point math.

**Rationale**: ESP32 has no FPU for double precision. Using integer math avoids soft-float overhead and keeps animation predictable across clock speeds. The cast to `long` prevents overflow on ranges up to ~2 billion ms.

## 7. Checkered Stipple Pattern (progressBarWidget)

**Decision**: The progress bar fill uses alternating pixels in a checkered pattern rather than a solid fill.

**Rationale**: On 1-bit monochrome OLED displays, a solid bar is visually harsh. The checkered stipple gives a "shaded" LCD/DMD appearance without requiring alpha blending or grayscale. This matches the aesthetic of the departures-board reference implementation.

## 8. drawText() Delegation

All text-rendering widgets delegate to `drawText()` from `drawingPrimitives.hpp` rather than calling `display.drawStr()` directly. This ensures consistent alignment, truncation, and vertical centering behavior across all widgets.
