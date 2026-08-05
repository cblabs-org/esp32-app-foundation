# ADR: Config Gate Dependency Ordering

## Context

After fixing the EventBus subscription, the configuring screen appeared — but it showed "Weather Setup" immediately at boot, even before WiFi connected. The URL displayed `http://0.0.0.0` because no IP was assigned yet. The user couldn't act on the Weather gate without network access.

## Root Cause

`ConfigGateResolver::findActionableGate()` iterates registered gates in order and returns the first unsatisfied gate whose dependencies are all satisfied. Both `WeatherModule` and `WifiConfigGate` had **no declared dependencies**, so the resolver picked whichever came first in the registration list.

Registration order:
1. `WeatherModule` — registered in `main.cpp` via `app.registerModule()` **before** `app.begin()`
2. `WifiConfigGate` — registered inside `app.begin()` at step 7

Since Weather was registered first and had no deps, it was returned as actionable even though WiFi hadn't connected.

## Decision

Added `getDependencies()` override to `WeatherModule` declaring `GateNames::WIFI_STATION` as a dependency:

```cpp
const char* const* getDependencies(uint8_t& count) const override {
    static const char* deps[] = { GateNames::WIFI_STATION };
    count = 1;
    return deps;
}
```

This ensures the resolver skips Weather until WiFi Station is satisfied (connected in STA mode).

## Design Principle

Any gate that requires network access to be actionable (web portal configuration, API setup, etc.) should declare `WIFI_STATION` as a dependency. This is a consumer responsibility — the framework doesn't auto-infer it because some gates might work over AP mode or be locally actionable.

## Verified Boot Sequence After Fix

```
I (7859) Application: State → CONFIGURING
I (7869) Config gate unsatisfied: WiFi Station (status=1)   ← InProgress, shown first
I (9880) sta ip: 192.168.20.186                              ← WiFi connects
I (9906) Application: State → RUNNING                        ← All gates satisfied
```

## Files Changed

- `sample-app/src/WeatherModule.hpp` — Added `getDependencies()` override
