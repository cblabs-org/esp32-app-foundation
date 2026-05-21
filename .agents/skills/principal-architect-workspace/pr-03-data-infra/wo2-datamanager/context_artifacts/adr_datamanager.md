# Architecture Decision Record: dataManager

## Decision
Implemented a singleton-style `DataManager` with an explicit mutex-protected registry and a FreeRTOS worker task. It utilizes the standard ESP32 `millis()` (and `std::chrono` natively) to coordinate interval-based background scheduling of `iDataSource` implementations. 

## Rationale
- **FreeRTOS Task:** A dedicated task offloads periodic network fetches from the main loop, preventing blockages.
- **PriorityTier Enum:** Scheduling iterates through a simple sorted tier approach (HIGH = 0, LOW = 2). Lower numerical value translates to higher precedence if multiple sources are due.
- **Native Mocks:** For GoogleTest (`NATIVE_BUILD`), FreeRTOS `<freertos/FreeRTOS.h>` is stubbed out, and a `processNext()` method is explicitly exposed to manually iterate the loop and simulate time progression using `setSimulatedTimeMs`. This keeps testing pure and extremely fast.

## Context
Following the WO2 requirements, we established this scheduler for the ESP32 App Foundation `dataManager` module. It must decouple data polling from standard `appContext` event buses, ensuring data providers just yield updates when requested.
