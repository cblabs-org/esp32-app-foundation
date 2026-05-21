# PR-03: Data Infrastructure

## Scope
Implementation of the Data Infrastructure for the ESP32 App Foundation program.

This PR establishes:
1. `EventBus` - typed pub/sub using static function pointers.
2. `dataManager` - FreeRTOS task for background data fetching.
3. `NullStatusReporter` - inline mock, and application hub wiring for data layers.

## Work Orders

- [ ] WO1: `EventBus`
- [ ] WO2: `dataManager`
- [ ] WO3: `NullStatusReporter` + App Hub Wiring
