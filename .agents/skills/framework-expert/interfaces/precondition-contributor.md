# IPreconditionContributor

Abstract interface for modules that contribute preconditions to the `PreconditionEngine` via the visitor/registry pattern.

## Overview

A precondition represents a condition that the system monitors for operational readiness. During boot, the `PreconditionEngine` ticks all registered preconditions. Once every Startup-role precondition is met (`allStartupMet()` returns `true`), the application transitions from `CONFIGURING` to `RUNNING`. Operational-role preconditions continue to be evaluated at runtime but do not block boot.

Modules do not inherit a precondition object. Instead, they implement `IPreconditionContributor` and use a visitor pattern to register preconditions via `IPreconditionRegistry::add()`.

```cpp
class IPreconditionContributor {
public:
    virtual ~IPreconditionContributor() = default;
    virtual void registerPreconditions(IPreconditionRegistry& registry) = 0;
};
```

## IPreconditionRegistry::add()

```cpp
void add(const char* name,
         PreconditionRole role,
         EvalFn evaluate,
         DescribeFn describe = nullptr,
         const Edge* edges = nullptr,
         uint8_t edgeCount = 0);
```

| Parameter | Type | Description |
|:---|:---|:---|
| `name` | `const char*` | Human-readable unique identifier. Must have static/application lifetime. |
| `role` | `PreconditionRole` | Lifecycle role — `Startup`, `Operational`, or `Both`. |
| `evaluate` | `EvalFn` | Fast-path callback returning `PreconditionStatus`. Called every tick. Must be non-blocking. |
| `describe` | `DescribeFn` | Optional cold-path callback writing user-facing help text into a buffer. Called on-demand (e.g., display, portal). |
| `edges` | `const Edge*` | Optional pointer to array of dependency edges. |
| `edgeCount` | `uint8_t` | Number of entries in the `edges` array. |

### Callback Types

```cpp
using EvalFn     = std::function<PreconditionStatus()>;
using DescribeFn = std::function<void(Cause, char*, size_t)>;
```

- **`EvalFn`** (hot path): Returns `{bool met, Cause cause}`. Called every tick cycle. Must be cheap — read a cached boolean, check a flag. No I/O, no allocation.
- **`DescribeFn`** (cold path): Formats human-readable help text into a caller-supplied `char*` buffer. Called on-demand by display modules and the web portal. Receives the current `Cause` so the message can be context-specific.

## Cause Enum (12 values)

The cause vocabulary explains *why* a precondition is met or unmet.

| Value | Meaning |
|:---|:---|
| `Met` | Condition is satisfied. |
| `NotConfigured` | Nothing to work with. A person must supply something. |
| `Connecting` | In progress; expected to resolve unaided. |
| `Rejected` | Config present and refused — bad password, revoked token. |
| `Unreachable` | Config present and plausible; the peer is not answering. |
| `RateLimited` | The peer is answering and asking us to slow down. |
| `Implausible` | An answer arrived, well-formed, and a domain check rejects it. |
| `DependencyUnmet` | Blocked upstream. Not this precondition's problem. |
| `FallbackEngaged` | Degraded but serviceable, and still self-healing. |
| `Unavailable` | Not present in this deployment, by configuration. Not a fault. |
| `Indeterminate` | Cannot evaluate — a dependency was not wired. |
| `InternalError` | A defect, not a configuration problem. |

## Severity Enum (6 values)

Derived from `Cause` and trust history via `deriveSeverity()`. Represents the operational impact.

| Value | Meaning |
|:---|:---|
| `Transient` | Keep trying; self-healing in progress. |
| `NeedsUser` | Requires human intervention to resolve. |
| `Blocked` | Upstream dependency unmet; upstream owns the report. |
| `Degraded` | Serviceable in fallback mode, still self-healing. |
| `Inactive` | Deliberately absent by configuration; not a fault. |
| `Persistent` | Defect or un-wired dependency; requires attention. |

## deriveSeverity()

```cpp
inline Severity deriveSeverity(Cause cause, bool everMet = false);
```

Maps a `Cause` value and trust history (`everMet`) to a `Severity`. Notable trust-history sensitivity:
- `Unreachable` → `Transient` if previously met, `NeedsUser` if never met.
- `Implausible` → `Transient` if previously met, `NeedsUser` if never met.

## PreconditionRole Enum

| Value | Meaning |
|:---|:---|
| `Operational` | Features degrade while unmet. Does NOT block boot. (Default) |
| `Startup` | Must be met to leave boot. |
| `Both` | Required at boot AND continues to gate operational health. |

## EdgeKind Enum

Dependency relationship strength between preconditions.

| Value | Meaning |
|:---|:---|
| `Wants` | Presentation only: don't offer me while that is unmet. |
| `Requires` | I cannot be met without it. Evaluated every tick. |
| `BindsTo` | Requires, plus: if it becomes unmet at runtime, so do I. |

## BreakerState Enum

Circuit breaker states for self-healing resources.

| Value | Meaning |
|:---|:---|
| `Closed` | Normal operation. |
| `Open` | Retries exhausted; fallback engaged. |
| `HalfOpen` | Cooldown elapsed — trial probe in progress. |

## PreconditionStatus Struct

```cpp
struct PreconditionStatus {
    bool  met   = false;
    Cause cause = Cause::NotConfigured;
};
```

## PreconditionNames

Well-known precondition identifiers used by display modules for built-in modal dispatch.

```cpp
namespace PreconditionNames {
    inline constexpr const char* WIFI_STATION = "WiFi Station";
    inline constexpr const char* TIMEZONE     = "Timezone";
    inline constexpr const char* TIME_SYNC    = "Time Sync";
}
```

### Optional Module Preconditions

| Precondition | Module | Role | Build Flag | Description |
|:---|:---|:---|:---|:---|
| `audit_health` | `AuditModule` | `Operational` | `WITH_AUDIT` | Tracks audit subsystem health. Uses sticky `FallbackEngaged` degradation — once a drop or throttle occurs, stays degraded until `clearAll()`. See [Audit Module Guide](../references/audit-module-guide.md) §8. |

## Registration Flow

1. Module inherits `IPreconditionContributor`.
2. Implements `registerPreconditions(IPreconditionRegistry& reg)`.
3. Inside that method, calls `reg.add(...)` for each precondition it contributes.
4. At startup, `app.registerModule(&myModule)` auto-detects `IPreconditionContributor` via `is_base_of_v<IPreconditionContributor, T>` and calls `registerPreconditions()`.

No manual registration call is required — auto-detection handles it.

## Example: WifiPreconditionContributor (Framework Built-in)

```cpp
class WifiPreconditionContributor : public IPreconditionContributor {
public:
    explicit WifiPreconditionContributor(const WifiManager& wifi) : _wifi(wifi) {}

    void registerPreconditions(IPreconditionRegistry& reg) override {
        reg.add(PreconditionNames::WIFI_STATION, PreconditionRole::Startup,
            [this]() -> PreconditionStatus {
                if (_wifi.isReady() && !_wifi.getAPMode())
                    return {true, Cause::Met};
                if (_wifi.getAPMode())
                    return {false, Cause::FallbackEngaged};
                if (_wifi.hasCredentials())
                    return {false, Cause::Connecting};
                return {false, Cause::NotConfigured};
            },
            [this](Cause c, char* buf, size_t len) {
                switch (c) {
                    case Cause::NotConfigured:
                        snprintf(buf, len, "Connect to the device WiFi to configure");
                        break;
                    case Cause::Connecting:
                        snprintf(buf, len, "Connecting to %s...", _wifi.getSSID());
                        break;
                    case Cause::FallbackEngaged:
                        snprintf(buf, len, "Can't reach your network. Connect to %s",
                                 _wifi.getAPSSID());
                        break;
                    case Cause::Rejected:
                        snprintf(buf, len, "WiFi password was refused");
                        break;
                    default:
                        snprintf(buf, len, "WiFi unavailable");
                        break;
                }
            });
    }

private:
    const WifiManager& _wifi;
};
```

## Example: Minimal Consumer Pattern (MyApp)

```cpp
#include "AppFoundation.h"

class MyApp : public ITickable, public IPreconditionContributor {
public:
    void registerPreconditions(IPreconditionRegistry& reg) override {
        reg.add("API Key", PreconditionRole::Startup,
            [this]() -> PreconditionStatus {
                return _apiKeyConfigured
                    ? PreconditionStatus{true, Cause::Met}
                    : PreconditionStatus{false, Cause::NotConfigured};
            },
            [](Cause, char* buf, size_t len) {
                snprintf(buf, len, "Enter your API key in the Web Portal");
            });
    }

    void tick(uint32_t now) override { /* ... */ }

private:
    bool _apiKeyConfigured = false;
};
```

## Example: Consumer with Edges (WeatherModule Pattern)

```cpp
class WeatherModule : public ITickable, public IDataSource,
                      public IPreconditionContributor {
public:
    void registerPreconditions(IPreconditionRegistry& reg) override {
        static const Edge edges[] = {
            {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
        };

        reg.add("Weather API", PreconditionRole::Operational,
            [this]() -> PreconditionStatus {
                if (!_apiKeySet) return {false, Cause::NotConfigured};
                if (_lastFetchFailed) return {false, Cause::Unreachable};
                return {true, Cause::Met};
            },
            [](Cause c, char* buf, size_t len) {
                switch (c) {
                    case Cause::NotConfigured:
                        snprintf(buf, len, "Set API key in Weather settings");
                        break;
                    case Cause::Unreachable:
                        snprintf(buf, len, "Weather API not responding");
                        break;
                    default:
                        snprintf(buf, len, "Weather service unavailable");
                        break;
                }
            },
            edges, 1);
    }
    // ...
};
```

## PreconditionEngine Public API

The `PreconditionEngine` is the concrete engine that implements `IPreconditionRegistry` and manages all registered preconditions.

| Method | Description |
|:---|:---|
| `tick(uint32_t nowMs)` | Evaluate all preconditions, update damping/breaker, derive severity, publish `precondition.health` events on state change. |
| `allStartupMet()` | Returns `true` when every `Startup` and `Both` role precondition is met (undamped). Used for CONFIGURING → RUNNING transition. |
| `getStateAt(size_t i)` | Read-only access to the i-th `PreconditionState` (name, status, severity, breaker, etc.). |
| `getStateByName(name)` | Look up a precondition state by name. |
| `requestRetry(name)` | Reset the breaker for a specific precondition, forcing an immediate re-evaluation. |
| `bumpIdentityEpoch(name)` | Reset `everMet` trust history for a precondition (e.g., when the user changes WiFi credentials). |
| `resolveEdges()` | Resolve declared edge names to internal indices. Called once after all contributors have registered. |
| `getPreconditionCount()` | Number of registered preconditions. |

## Damping and Breaker Policies

The engine uses two stability mechanisms to prevent flapping and manage self-healing retries:

- **Damping**: Penalty score accumulates while a precondition is unmet, decays exponentially while met. If penalty exceeds the suppress threshold, the precondition is considered "suppressed" — triggering regression to CONFIGURING for Startup/Both roles. Hysteresis between suppress and reuse thresholds prevents oscillation.
- **Breaker**: Circuit breaker state machine (`Closed → Open → HalfOpen → Closed`) manages retry escalation. After exhausting retry attempts, the breaker opens and a fallback is engaged. After a cooldown, a trial probe is attempted (HalfOpen).

Both policies use configurable parameters (`DampingPolicy`, `RetryPolicy` structs) with sensible defaults.
