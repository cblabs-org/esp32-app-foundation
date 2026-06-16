# Framework Architecture Reference

## The 3-Tier Model

The framework decouples code into three distinct layers to ensure hardware/software independence and enable host-based testing.

*   **Framework Kernel (`modules/`):** Central reusable infrastructure (e.g., `Application`, `ConfigManager`, `EventBus`, `WifiManager`, `WebServerManager`). It has zero dependencies on application-specific code.
*   **Interfaces (`include/interfaces/`):** Pure virtual C++ class contracts (e.g., `ITickable`). This is the only coupling point between Kernel and Application.
*   **Your App (`template/src/`):** Implements interfaces to inject custom behavior.

## Single Include Façade
All downstream code should use a single header include to access the framework:
```cpp
#include "AppFoundation.h"
```

## Universal Module Registration
The framework uses C++ compile-time type introspection (`if constexpr` and `std::is_base_of_v`) to detect which interfaces a class implements. Calling `app.registerModule(&module)` automatically registers the module with all matching subsystems.

```cpp
// Execute in setup()
app.registerModule(&myAppModule);
```

## Extension Interfaces

| Interface | Purpose | Key Callback |
| :--- | :--- | :--- |
| `ITickable` | Periodic, non-blocking tick-loop logic | `tick(uint32_t currentMillis)` |
| `IConfigurable` | Central config persistence (JSON) | `applyConfig(const JsonObject&)` |
| `IWebContributor` | Web routes & bottom-nav UI tab addition | `registerRoutes(PsychicHttpServer&)` |
| `IMCPContributor` | Local Model Context Protocol JSON-RPC tools | `registerMCPTools(MCPToolRegistry&)` |
| `IConfigGate` | Blocks boot-to-RUNNING transition | `isSatisfied()` |
| `IDataSource` | Background data fetch task | `fetchUpdate()` |
| `IStatusReporter` | Progress status reporting during boot | `reportStatus(const char*, float)` |
| `IDisplayBoard` | Telemetry pane in monochrome display | `render(U8G2&)` |

## Application Lifecycle & Boot Phases

The system transitions: `BOOT` $\rightarrow$ `CONFIGURING` $\leftrightarrow$ `RUNNING`.

### 1. BOOT Phase
Executed synchronously inside `app.begin(config)`:
1.  **EventBus** initialized.
2.  **LittleFS** mounted.
3.  **DeviceCrypto** & **KeyManager** initialize keys.
4.  **ConfigManager** loads and applies configs.
5.  **WifiManager**, **TimeManager**, **DataManager**, and **OTAUpdateManager** start.
6.  REST routes and MCP tools registered.
7.  Registry **compacted and locked**.
8.  Firmware self-test checks boot partition.
9.  `IConfigGate` gates evaluated.

### 2. CONFIGURING Phase
Entered if any `IConfigGate::isSatisfied()` returns `false` (e.g., station credentials missing).
*   Ticks are still dispatched; WiFi/local captive portal remain active.
*   Framework polls gates; transitions to `RUNNING` when all are satisfied.

### 3. RUNNING Phase
Entered once all configuration gates are satisfied.
*   Ticks dispatched to all `ITickable` modules.
*   Gates are checked every 5 seconds. If a gate remains unsatisfied beyond its `getRegressionToleranceMs()`, the system regresses to `CONFIGURING`.

## LittleFS Directory Layout
*   `/mod/<namespace>.json`: Central config file for `IConfigurable` namespace.
*   `/www/`: Root directory for the Single Page Application (SPA) web portal.
*   `/www/<namespace>/tab.html` & `/www/<namespace>/tab.js`: Dynamic UI fragments loaded by the SPA shell.
