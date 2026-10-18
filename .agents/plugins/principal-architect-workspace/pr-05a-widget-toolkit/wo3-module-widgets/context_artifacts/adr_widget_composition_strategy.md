# ADR: Widget Composition Strategy for System Boards

## Decision

All 6 system boards were refactored to use internal widget composition while preserving their external `IDisplayBoard` interface and visual output.

## Context

The existing system boards (`BootSplashBoard`, `LoadingBoard`, `WiFiWizardBoard`, `OtaProgressBoard`, `ErrorBoard`, `ScreensaverBoard`) used raw `u8g2.setFont()` / `u8g2.drawStr()` / `u8g2.getStrWidth()` calls directly in their `render()` methods. WO3 replaces these internals with widget instances from the WO1/WO2 toolkit.

## Key Decisions

### 1. Widget Members as Value Types (Not Pointers)
All widget members are declared as direct member variables with in-class brace initialisers (e.g., `labelWidget _titleLabel{0, 12, -1, -1}`). This avoids heap allocation entirely and simplifies lifecycle management. Constructor arguments are positional coordinates matching the original manual centering math.

### 2. Font Configuration Deferred to render() / activate()
Because `AppFonts::defaultSmall()` etc. return U8G2 font pointers that require the Arduino framework at link time, fonts cannot be set in member initialiser lists. Instead:
- `labelWidget` fonts are set via `setFont()` in `activate()` or at the top of `render()`
- A new `setFont()` method was added to `progressBarWidget` (it previously only accepted font in constructor)

### 3. ScreensaverBoard Uses labelWidget with Dynamic setCoords()
The bouncing clock animation requires per-frame position updates. Rather than reimplementing text rendering manually, the refactored ScreensaverBoard uses `labelWidget._clockLabel` with `setCoords()` called each frame to move the label to the bounce position. The `render()` call on the widget handles all font/alignment work.

### 4. systemMessageWidget as Dedicated Multi-Line Renderer
`WiFiWizardBoard` and `ErrorBoard` both display centred multi-line messages. Rather than composing 4 separate `labelWidget` instances per board, a dedicated `systemMessageWidget` was created with `setMessage(title, l1, l2, l3)` API. This reduces boilerplate and ensures consistent vertical spacing logic.

### 5. scrollingMessagePoolWidget Uses EventBus (Not MessagePool)
The foundation framework has no `MessagePool` class. The widget subscribes to `EventBus` topic `"display.message"` via a static callback + `void* ctx` pattern. Messages are stored in a fixed circular buffer (`MessageEntry[8]`). The `begin(EventBus*)` method performs late binding to avoid constructor-time subscription issues.

### 6. WiFi.h Include Ordering Fix
`wifiStatusWidget.cpp` originally had `#include <WiFi.h>` inside the rendering `#ifndef NATIVE_BUILD` block (line ~85), but `tick()` also calls `WiFi.status()` inside its own guard block (line ~56). The include was moved to a top-level `#ifndef NATIVE_BUILD` block to ensure the symbol is available for both methods.

## Boundary Compliance
- `iGfxWidget.hpp` was NOT modified (WO1 deliverable)
- `drawingPrimitives.hpp/.cpp` were NOT modified (WO1 deliverable)
- Generic widgets from WO2 were NOT modified (except `progressBarWidget.hpp` — `setFont()` addition)
- `platformio.ini` and `library.json` were NOT modified
