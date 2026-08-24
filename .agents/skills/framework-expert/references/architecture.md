# Framework Architecture Reference

> **Scope:** Terse, agent-facing quick reference for the `framework-expert` skill.
> The canonical human-facing documentation is [`docs/developer/`](../../../../docs/developer/README.md)
> — start with [`architecture-overview.md`](../../../../docs/developer/architecture-overview.md).
> The single source of truth for the full design is
> [`docs/design_specification.md`](../../../../docs/design_specification.md).

## The 3-Tier Model

The framework decouples code into three distinct layers to ensure hardware/software independence and enable host-based testing.

*   **Framework Kernel (`modules/`):** Central reusable infrastructure (e.g., `Application`, `ConfigManager`, `EventBus`, `WifiManager`). Zero dependencies on application code.
*   **Interfaces (`include/interfaces/`):** Pure virtual C++ class contracts. This is the only coupling point between Kernel and Application.
*   **Your App (`your-project/src/` — see `sample-app/` for reference):** Implements interfaces to inject custom behavior.

### ADR-04: The Hollywood Principle
The framework strictly enforces Inversion of Control. "Don't call us, we'll call you." Application modules should not orchestrate the boot sequence or poll the system state. Instead, they register via `app.registerModule()`, and the framework calls their lifecycle methods (`tick`, `applyConfig`, `render`).

## Static Scope Requirement (ADR-16)

**CRITICAL:** All module instances MUST be declared in static or global scope.

```cpp
// ✅ CORRECT: Global or static allocation
static MySensorModule sensorModule;

void setup() {
    app.registerModule(&sensorModule);
}

// ❌ WRONG: Stack allocation leads to dangling pointer
void setup() {
    MySensorModule sensorModule; // Destroyed when setup() returns
    app.registerModule(&sensorModule); 
} // Crash when framework calls sensorModule.tick()
```
The ESP32 lacks a Memory Management Unit (MMU) to trap dangling pointer access cleanly. Stack-allocated modules will silently corrupt memory or cause hard faults when the framework tries to invoke them.

## Application Lifecycle & Boot Phases

The system transitions: `BOOT` $\rightarrow$ `CONFIGURING` $\leftrightarrow$ `RUNNING`.

### 1. BOOT Phase
Executed synchronously inside `app.begin(config)`:
1.  **EventBus** & **LittleFS** initialized.
2.  **DeviceCrypto** & **KeyManager** initialize keys.
3.  **ConfigManager** loads configs from LittleFS.
4.  **WifiManager**, **TimeManager**, **DataManager**, and **OTAUpdateManager** start.
5.  REST routes and MCP tools registered.
6.  Registry **compacted and locked** (Registration closes).
7.  Firmware self-test checks boot partition.
8.  `PreconditionEngine` edges resolved, first tick.

### 2. CONFIGURING Phase
Entered if `PreconditionEngine::allStartupMet()` returns `false` (e.g., station credentials missing).
*   Ticks are dispatched; WiFi AP/Captive Portal active.
*   `PreconditionEngine::tick()` evaluates all preconditions each cycle, applying damping and breaker state machine logic, and deriving `Severity` from `Cause` and trust history via `deriveSeverity()`.
*   Transitions to `RUNNING` when `PreconditionEngine::allStartupMet()` returns `true`.
*   Application re-publishes via `AppConfiguringPayload` (preconditionName, cause, severity) when the actionable precondition or its status changes.
*   Display modules are pure event-driven reactors consuming `app.configuring` and `precondition.health` events.

### 3. RUNNING Phase
Entered once all Startup-role preconditions are met.
*   Ticks dispatched to all `ITickable` modules.
*   **Tick Budget (ADR-17):** Modules MUST complete their `tick()` within 10ms. The Task Watchdog Timer (TWDT) will panic and reboot the ESP32 if the main loop blocks for more than 5 seconds. Lengthy operations must be deferred to FreeRTOS tasks (e.g., `IDataSource`) or broken into state machines.
*   `PreconditionEngine` continues ticking. If a Startup/Both-role precondition becomes suppressed (damping threshold exceeded), the system reverts to `CONFIGURING`.

## LittleFS Directory Layout
*   `/mod/<namespace>.json`: Central config file for `IConfigurable` namespace.
*   `/www/`: Root directory for the Single Page Application (SPA) web portal.
*   `/www/<namespace>/tab.html` & `/www/<namespace>/tab.js`: Dynamic UI fragments.
