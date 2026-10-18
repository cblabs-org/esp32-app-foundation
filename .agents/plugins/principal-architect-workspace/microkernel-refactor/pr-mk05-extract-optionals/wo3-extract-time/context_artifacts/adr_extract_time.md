# ADR: Extraction of TimeManager and TimeConfigGate from Application Core (`pr-mk05-wo3`)

## Context & Rationale

Per microkernel design principles (MK-1 and MK-6), `Application` (Ring-0/1 composition hub) should not own optional Ring-2 services by value. `TimeManager` manages NTP synchronization, POSIX timezone rules, and REST API routes for time settings. While lightweight, it is an optional domain service that belongs in Ring-2 alongside `MCPServer`, `OTAUpdateManager`, and `HomeAssistantModule`.

## Structural Decisions

1. **Removed Ownership**: `Application` no longer owns `TimeManager _timeManager;` or `TimeConfigGate _timeGate;` by value and no longer includes `TimeManager.hpp` or `TimeConfigGate.hpp`.
2. **Removed Getters**: `Application::getTimeManager()` and `Application::getTimeGate()` accessors were removed to enforce explicit composition.
3. **Explicit Composition in `main.cpp`**: `main.cpp` (along with `template-app/src/main.cpp` and `sample-app/src/main.cpp`) instantiates file-scope `TimeManager timeManager` and `TimeConfigGate timeGate{timeManager}` inside `#ifdef WITH_TIME`. It registers `&timeManager` with `app.registerModule(&timeManager)` pre-lock, and calls `timeManager.begin()` post-`app.begin()`.
4. **Flag Management**: `WITH_TIME` is configured as **default ON** in `platformio.ini` (`build_flags` for `env:base_esp32` and `env:native`).
5. **Flash Payoff**: Disabling `WITH_TIME` saves **~12.8 KiB flash** on `esp32dev` (1,124,507 bytes vs 1,137,379 bytes).
