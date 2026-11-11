# modules/PreconditionEngine

## Invariants & contracts

### [docseed-01328750] **A11y:** Set `aria-disabled="true"` on the element to communicate the disabled state to screen readers.
`invariant · kind: semantic · anchors: include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-2eac23e7] **All 7 app-facing interfaces demonstrated** — `WeatherModule` now implements every framework interface including `IMCPContributor` and `IPreconditionContributor`
`contract · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-39433a64] The `PreconditionEngine` evaluates all preconditions. Once all startup preconditions are met, the framework transitions to `RUNNING`.
`invariant · kind: semantic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-3b85990d] **Centralized Gateways:** `ConfigManager` for persistence, `DataManager` for background data fetching, `PreconditionEngine` for lifecycle gating.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp, modules/DataManager/DataManager.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-47972415] `IMCPContributor`, `IPreconditionContributor`, `IPreconditionRegistry`
`contract · kind: semantic · anchors: include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, include/interfaces/IPreconditionRegistry.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-48ddb8a5] Regression is managed by the `PreconditionEngine`'s `DampingPolicy` — if a previously-met precondition regresses, damping prevents UI flapping.
`invariant · kind: semantic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-57297d5c] Enforces exact signature parity for all framework interfaces (`ITickable`, `IConfigurable`, `IDataSource`, `IStatusReporter`, `IBootReporter`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`, `ISanitizer`) using compile-time static assertions.
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IStatusReporter.hpp, include/interfaces/IBootReporter.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, include/interfaces/ISanitizer.hpp · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-5e06c783] **Interface Contracts**: Implements appropriate framework interfaces (`ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`, `IDataSource`, `ISanitizer`, `IBootReporter`).
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/ISanitizer.hpp, include/interfaces/IBootReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-90a3a527] `IPreconditionContributor` → `module->registerPreconditions(_preconditionEngine)`
`invariant · kind: semantic · anchors: include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-96395b9c] **`EvalFn evaluate`** (Hot Path): Non-blocking fast-path function called every tick. Checks a cached flag or state variable and returns `PreconditionStatus{met, cause}`.
`invariant · kind: semantic · anchors: include/interfaces/IPreconditionRegistry.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9cbb9f21] Implements one or more framework interfaces (`ITickable`, `IConfigurable`, `IDataSource`, `IStatusReporter`, `IBootReporter`, `IDisplayModule`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`)
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IStatusReporter.hpp, include/interfaces/IBootReporter.hpp, include/interfaces/IDisplayModule.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-a123752e] **`BLE Advertising`**: Monitors advertising packet composition. Evaluates to `Fault` (`Cause::Implausible`) if required advertising elements exceed the 31-byte PDU limit.
`invariant · kind: semantic · anchors: include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-a5369772] All 14 framework interfaces: `ITickable`, `IConfigurable`, `IDataSource`, `IStatusReporter`, `ITabRegistry`, `IWebContributor`, `IMCPContributor`, `IPrecondition`, `IPreconditionRegistry`, `IPreconditionContributor`, `IOTAVerifier`, `ISanitizer`, `IBootReporter`, `IDisplayModule`
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IStatusReporter.hpp, include/interfaces/ITabRegistry.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IWebContributor.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPrecondition.hpp, include/interfaces/IPreconditionRegistry.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, include/interfaces/IPreconditionContributor.hpp, include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, include/interfaces/ISanitizer.hpp, include/interfaces/IBootReporter.hpp, include/interfaces/IDisplayModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-a5a7e321] **Heap & Gate Evaluation (Steps 18–20):** Publish `system.heap`, clear status overlay, evaluate `PreconditionEngine::allStartupMet()`, and transition to either `AppState::Running` or `AppState::Configuring`.
`invariant · kind: semantic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp, include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b6c108f0] **`edges` / `edgeCount`**: Prerequisite dependencies on other preconditions.
`invariant · kind: semantic · anchors: include/interfaces/IPreconditionRegistry.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c378cf34] **`Configuring → Running`:** During `tick()`, `PreconditionEngine::allStartupMet()` becomes true. The framework publishes `ApplicationEvents::APP_RUNNING` (`"app.running"`).
`invariant · kind: semantic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-d8890eb3] **Extensible Module Model**: Defined extension points (`IConfigurable`, `ITickable`, `IDataSource`, `IWebContributor`, `IMCPContributor`, `IPreconditionContributor`) allowing domain modules to self-register via `registerModule()` without touching the kernel.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/ITickable.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp, include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-dd291328] **`DescribeFn describe`** (Cold Path): Formats context-sensitive user guidance into a caller-provided buffer based on the current `Cause`. Called on-demand by displays and portal UI.
`invariant · kind: semantic · anchors: include/interfaces/IPreconditionRegistry.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-fcc79c9c] **Deterministic Lifecycle & Boot Gating**: A structured 3-state application lifecycle (`Booting` → `Configuring` ↔ `Running`) governed by **`PreconditionEngine`**, which evaluates system prerequisites, gates normal execution, continuously monitors runtime health, and attributes failures with Cause and Severity to prevent wedged device states.
`invariant · kind: semantic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

## Ruled out, failure signatures and gotchas

### [docseed-fd25e553] **`BLE Stack`**: Monitors controller and NimBLE host initialization. Evaluates to `Fault` (`Cause::InternalError`) if hardware init fails or provider service creation fails.
`failure-signature · kind: procedural · anchors: include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-8583ff93] **Boot Preconditions**: Modules that gate system readiness implement `IPreconditionContributor` and register preconditions with appropriate severity (`Info`, `Warning`, `Critical`).
`gotcha · kind: semantic · anchors: include/interfaces/IPreconditionContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`
