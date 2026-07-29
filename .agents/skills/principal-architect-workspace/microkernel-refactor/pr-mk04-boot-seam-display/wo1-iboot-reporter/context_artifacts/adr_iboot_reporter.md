# Architecture Decision Record: IBootReporter Seam

## Context
The ESP32 App Foundation `Application::begin()` sequence requires early hardware initialization (specifically the boot display splash) before the framework's EventBus, LittleFS, ConfigManager, and domain services initialize. Previously, this was hardcoded to `_display.bootstrapHardware()`. 

## Decision
We introduced an `IBootReporter` interface (inheriting from `IStatusReporter`) with a `beginEarly()` virtual method. This acts as a dependency-injected seam. The `Application` registry was updated to detect modules implementing `IBootReporter` via `registerBootReporter`.

In `Application::begin()`, if an `IBootReporter` is registered, its `beginEarly()` method is called and it is adopted as the `_statusReporter`. To maintain backward compatibility during the WO1 transition before the U8G2 display is fully extracted (which happens in WO2), a fallback mechanism was preserved that retains the hardcoded `_display.bootstrapHardware()` call if `_bootReporter` is null.

## Consequences
- The framework core is now decoupled from the display hardware for the boot progress sequence.
- Hardware displays can now be fully registered as normal framework modules in WO2.
- The `NullStatusReporter` correctly acts as a fallback for headless operations, simplifying state management.
