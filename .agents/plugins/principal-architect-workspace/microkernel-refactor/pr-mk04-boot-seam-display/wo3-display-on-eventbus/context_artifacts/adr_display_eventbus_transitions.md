# ADR: Decoupling Display Lifecycle via EventBus (`APP_CONFIGURING` and `APP_RUNNING`)

## Context & Motivation
Prior to this refactor, `Application.cpp` directly drove display state transitions by invoking display methods in `_onEnterConfiguring()` and `_onEnterRunning()`. In WO2, those direct calls were commented out as part of extracting the display into a standalone module. 

WO3 completes the decoupling by subscribing display modules directly to `ApplicationEvents::APP_CONFIGURING` and `ApplicationEvents::APP_RUNNING` on the `EventBus`.

## Key Architectural Decisions

1. **`U8G2DisplayModule` Subscriptions**:
   - `APP_CONFIGURING`: Suspends the board carousel (`carousel.suspend()`) and displays the configuring status board (`showBase(&boards.configuringStatus)`).
   - `APP_RUNNING`: Clears modal overlays (`clearModal()`), clears base layer override (`showBase(nullptr)`), and resumes carousel rotation (`carousel.resume()`).
   - Restoring WiFi Wizard modal remains marked with `// TODO: Restore WiFi Wizard modal logic` to avoid tight coupling with `WifiManager`.

2. **`LVGLDisplayModule` Subscriptions**:
   - Subscribes to `APP_CONFIGURING` and `APP_RUNNING` with structured log entries (`ESP_LOGI`) matching the lifecycle pattern.

3. **Complete Elimination of Kernel Coupling (`Application.cpp`)**:
   - Removed commented-out `#ifdef HAS_DISPLAY` and `_display.*` code from `_onEnterConfiguring()` and `_onEnterRunning()`.
   - `Application` has zero concrete references to `_display` or `HAS_DISPLAY`.

## Consequences & Tradeoffs
- Post-boot display behavior is fully event-driven, decoupled from kernel state loops.
- `Application` acts purely as an event publisher; display modules manage their own layer states based on lifecycle events.
