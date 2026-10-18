# Architecture Decision Record: Display Extraction

## Context
The ESP32 App Foundation framework previously owned the display subsystem directly within the `Application` microkernel, hard-coding `U8G2DisplayModule` initialization, configuration, and dependencies via `#ifdef HAS_DISPLAY`. This violated microkernel principles and prevented clean integration of alternate displays like LVGL.

## Decision
Extracted display concerns out of the `Application` class.
- Display modules (`U8G2DisplayModule`, `LvglDisplayModule`) now inherit from `IBootReporter` (introduced in WO1).
- `beginEarly()` overrides handle early hardware bootstrapping, satisfying phase 1 initialization needs (like rendering boot splashes) before framework dependencies are available.
- Display module instantiation and framework registration (via `app.registerModule`) have been moved downstream to the consumer applications (e.g., `template-app/src/main.cpp`).
- Internal references to `HAS_DISPLAY` and display-specific components (e.g., `_diagnosticsBoard`) were removed from `Application`.

## Consequences
- The `Application` kernel is now fully decoupled from display hardware.
- Downstream applications have complete control over which display module is instantiated and registered.
- Support for LVGL displays is fully symmetrical to U8G2 displays.
