# IConfigGate

Abstract interface for application configuration prerequisites.

## Overview

A gate represents a condition that must be satisfied before the application transitions from `CONFIGURING` to `RUNNING`. During boot, `Application::tick()` polls all registered gates. Once every gate's `isSatisfied()` returns true, the state advances.

```cpp
class IConfigGate {
public:
    virtual ~IConfigGate() = default;
    virtual bool isSatisfied() const = 0;
    virtual const char* getGateName() const = 0;
    virtual const char* getHelpText() const { return ""; }
    virtual const char* const* getDependencies(uint8_t& count) const { count = 0; return nullptr; }
    virtual uint32_t getRegressionToleranceMs() const { return 30000; }
};
```

## Methods

- **`isSatisfied()`**: Returns `true` if the gate is satisfied. Polled every tick during `CONFIGURING`. Must be a fast, non-blocking check (e.g., returning a cached boolean).
- **`getGateName()`**: Human-readable name for logging and the UI (e.g., `"WiFi Station"`).
- **`getHelpText()`**: User-facing guidance displayed on the config portal (e.g., `"Configure WiFi to continue"`).
- **`getDependencies()`**: An array of names of other gates that must be satisfied before this gate becomes "actionable". Allows ordered configuration (e.g., must connect to WiFi before configuring an API key).
- **`getRegressionToleranceMs()`**: How long (in ms) a gate can remain unsatisfied during `RUNNING` before forcing the system back to `CONFIGURING`. 

## Regression Tolerance

Network blips or temporary API failures shouldn't instantly kick the user back to the config screen. `getRegressionToleranceMs()` acts as a debounce.

- Return `0` for immediate regression.
- Return `UINT32_MAX` to never regress (if the app handles recovery internally).
- Default is `30000` (30 seconds).

## Registration

Gates are added via `app.registerConfigGate(&myGate)`. The framework auto-registers the `WifiConfigGate`; downstream apps register custom gates for things like API keys or sensor calibration.
