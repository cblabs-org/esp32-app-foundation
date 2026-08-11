# IConfigGate

Abstract interface for application configuration prerequisites.

## Overview

A gate represents a condition that must be satisfied before the application transitions from `CONFIGURING` to `RUNNING`. During boot, `Application::tick()` polls all registered gates. Once every gate's `isSatisfied()` returns true, the state advances.

```cpp
enum class GateActionStatus : uint8_t {
    Satisfied       = 0,
    InProgress      = 1,
    NeedsUserAction = 2
};

namespace GateNames {
    static constexpr const char* WIFI_STATION = "WiFi Station";
    static constexpr const char* TIMEZONE     = "Timezone";
}

class IConfigGate {
public:
    virtual ~IConfigGate() = default;
    virtual bool isSatisfied() const = 0;
    virtual const char* getGateName() const = 0;
    virtual const char* getHelpText() const { return ""; }
    virtual const char* const* getDependencies(uint8_t& count) const { count = 0; return nullptr; }
    virtual uint32_t getRegressionToleranceMs() const { return 30000; }
    virtual GateActionStatus getActionStatus() const {
        return isSatisfied() ? GateActionStatus::Satisfied : GateActionStatus::NeedsUserAction;
    }
};
```

## Methods

- **`isSatisfied()`**: Returns `true` if the gate is satisfied. Polled every tick during `CONFIGURING`. Must be a fast, non-blocking check (e.g., returning a cached boolean).
- **`getGateName()`**: Human-readable name for logging and the UI (e.g., `"WiFi Station"`). *Lifetime contract:* must return a static/application-lifetime pointer.
- **`getHelpText()`**: User-facing guidance displayed on the config portal (e.g., `"Configure WiFi to continue"`).
- **`getDependencies()`**: An array of names of other gates that must be satisfied before this gate becomes "actionable". Allows ordered configuration (e.g., must connect to WiFi before configuring an API key).
- **`getRegressionToleranceMs()`**: How long (in ms) a gate can remain unsatisfied during `RUNNING` before forcing the system back to `CONFIGURING`.
- **`getActionStatus()`**: Returns the current `GateActionStatus` (`Satisfied`, `InProgress`, `NeedsUserAction`). Used by display modules to determine whether to show a generic progress spinner or a user-intervention modal.

## Resolution

`ConfigGateResolver` serves as the single source of truth (SSOT) for hardware-free, dependency-aware gate resolution. It handles checking gate dependencies to determine which gate is currently actionable.

## Regression Tolerance

Network blips or temporary API failures shouldn't instantly kick the user back to the config screen. `getRegressionToleranceMs()` acts as a debounce.

- Return `0` for immediate regression.
- Return `UINT32_MAX` to never regress (if the app handles recovery internally).
- Default is `30000` (30 seconds).

## Registration

Gates are added via `app.registerConfigGate(&myGate)`. The framework auto-registers the `WifiConfigGate`; downstream apps register custom gates for things like API keys or sensor calibration.
