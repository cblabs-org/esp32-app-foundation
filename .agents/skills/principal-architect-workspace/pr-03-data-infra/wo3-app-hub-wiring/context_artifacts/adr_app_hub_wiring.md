# ADR: App Hub Wiring & NullStatusReporter

## NullStatusReporter
- **Decision:** Implemented as a private class `NullStatusReporter` within the `Application` class, with an instance variable `_nullReporter`.
- **Reason:** Keeps the fallback behavior completely encapsulated within the Application hub, avoiding exposing an unnecessary global or external class for a simple no-op behavior.

## Core Services Wiring
- **Decision:** `EventBus` and `DataManager` are composed directly as member variables `_eventBus` and `_dataManager` inside the `Application` class, and accessed via reference getters.
- **Reason:** Centralizes lifecycle management in `Application`, ensuring `begin()` order is strictly controlled by the hub. `tick()` drives `_dataManager.processNext()` under `NATIVE_BUILD` to support synchronous GoogleTest execution.

## Macro Conflicts
- **Decision:** Explicitly `#undef LOW` and `#undef HIGH` in `dataManager.cpp`.
- **Reason:** The `PriorityTier::LOW` enum in `iDataSource.hpp` conflicted with `Arduino.h` macros (`#define LOW 0x0`). Undefining them isolates our modern C++ enums from legacy Arduino macros.
