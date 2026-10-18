# ADR: System Board Ownership & Design Patterns

## Decision 1: Boards as Owned Members (not Heap Allocated)

The three system boards are declared as direct member variables of `U8g2DisplayModule` rather than heap-allocated via `new`/`unique_ptr`. This avoids dynamic allocation on a constrained ESP32 (no fragmentation risk) and guarantees lifetime management — the boards exist for the entire lifetime of the display module.

## Decision 2: Board Override vs Pool Membership

System boards are NOT added to the carousel pool via `addBoard()`. Instead, they are shown via `showBoard(&_bootSplash)` which sets them as an override board (`_overrideBoard`). This allows them to bypass the carousel rotation timer and be displayed independently of the pool lifecycle.

## Decision 3: NATIVE_BUILD Guard Strategy

All U8G2 rendering and `millis()` calls are guarded with `#ifndef NATIVE_BUILD`. In native test builds:
- `render()` is a no-op (suppresses unused param warning with `(void)u8g2`)
- `isExpired()` on `BootSplashBoard` always returns `false` (no millis available)
- `LoadingBoard::isExpired()` and `WiFiWizardBoard::isExpired()` work normally (they're pure state checks, no hardware dependency)

This means render correctness can only be verified on hardware. The native tests focus on state transitions and API contracts.

## Decision 4: U8G2 Font Selection

Used `u8g2_font_helvB10_tr` for bold headings and `u8g2_font_helvR08_tr` for regular text. These are compact Helvetica-style fonts that work well on both 128x64 (SSD1306) and 256x64 (SSD1322) displays. The render logic dynamically centres text using `getDisplayWidth()` so it adapts to any display size.

## Decision 5: Test Architecture — No U8G2 Instantiation

The `U8G2` class is only forward-declared in `IDisplayBoard.hpp` for native builds. Attempting to instantiate `U8G2 dummy;` in tests fails because the complete type definition is behind `#ifndef NATIVE_BUILD` guards in U8g2lib.h. Therefore, render tests were excluded from the native suite — they require hardware integration tests (WO4+).
