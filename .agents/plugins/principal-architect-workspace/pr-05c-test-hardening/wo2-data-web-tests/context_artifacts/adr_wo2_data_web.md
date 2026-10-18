# Architecture Decision Record: WO2 Data & Web Tests

## Context and Problem Statement
We needed to expand test coverage for `configManager`, `dataManager`, `eventBus`, and `webServer`. The primary constraint was that we could not modify any existing tests, and could not modify `modules/` or `lib/`. 

## Decisions
1. **Header Restorations**: During execution, the required `NATIVE_BUILD` header guards for `wifiManager.hpp` and `otaUpdateManager.hpp` (implemented in WO1) were found to be stashed and not applied to the working directory. We decided to retrieve these guards from the stash and apply them directly in WO2 to ensure all tests passed successfully, resolving native compilation errors.
2. **Fixture Reuse**: We utilized the pre-existing fixtures (`ConfigManagerTest`, `EventBusTest`) and mock classes (`MockDataSource`, `AsyncWebServer` native stub) defined within the target files and headers, aligning seamlessly with the strict non-modification boundaries while expanding the tests to hit the target counts.
