---
name: 146-config-gate-wo4
description: Handoff instructions for Work Order 4 — Framework Gate Migration
complexity: standard
---

# WO4 — Framework Precondition Migration

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Migrate the framework's two built-in config gates (`WifiConfigGate`, `TimeConfigGate`) to `IPreconditionContributor` implementations using the registry/visitor pattern. After this WO, the framework's own preconditions are registered through the new system.

## Context

Read:
- `docs/config-gate-design.md` — §9.1 (clock chain worked example), CG-1 (roles), CG-2b (registry pattern)
- `modules/WifiManager/WifiConfigGate.hpp` — the existing WiFi gate
- `modules/TimeManager/TimeConfigGate.hpp` — the existing time gate
- `modules/Application/Application.cpp` — L193 (built-in registration)
- WO1 types and WO2 engine for the target interfaces

**Key design decisions:**
- WiFi Station remains **Startup** role — without it there is no portal, so nothing else is fixable (§10).
- TimeConfigGate splits into **two** Operational preconditions: `Timezone` (user has chosen a zone) and `Time Sync` (NTP synced). Neither blocks boot (§9.1, §10).
- `Time Sync` has a `Requires` edge to `WiFi Station`.
- `Timezone` has **no edges** — it is settable from AP mode.

## Strict Boundary Box

- **DO NOT** modify `Application.hpp/cpp` beyond updating the built-in registration calls.
- **DO NOT** touch display modules, portal, sample-app, or template-app.
- **DO NOT** delete `IConfigGate.hpp` or the old gate classes — WO7 handles cleanup.
- The old `IConfigGate` implementations can remain as dead code temporarily.

## Tooling Constraints

- Use `platformio` MCP `build_project` (env: `native`).
- Use `platformio` MCP `run_tests` (env: `native`).

## Line-Item Instructions

- [ ] **`modules/WifiManager/WifiPreconditionContributor.hpp`** [NEW] — Implements `IPreconditionContributor`:
  ```cpp
  void registerPreconditions(PreconditionRegistry& reg) override {
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
              // Per-cause messages — replaces the fixed helpText
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
  ```
  Takes a `WifiManager&` reference in constructor. Mirror the existing `WifiConfigGate` constructor pattern.

- [ ] **`modules/TimeManager/TimePreconditionContributor.hpp`** [NEW] — Implements `IPreconditionContributor` with **two** preconditions:
  ```cpp
  void registerPreconditions(PreconditionRegistry& reg) override {
      // 1. Timezone — Operational, no edges (settable in AP mode)
      reg.add(PreconditionNames::TIMEZONE, PreconditionRole::Operational,
          [this]() -> PreconditionStatus {
              return _timeManager.isConfigured()
                  ? PreconditionStatus{true, Cause::Met}
                  : PreconditionStatus{false, Cause::NotConfigured};
          },
          [](Cause, char* buf, size_t len) {
              snprintf(buf, len, "Set your timezone in the Time settings");
          });

      // 2. Time Sync — Operational, Requires WiFi Station (§9.1)
      static const Edge syncEdges[] = {
          {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
      };
      reg.add(PreconditionNames::TIME_SYNC, PreconditionRole::Operational,
          [this]() -> PreconditionStatus {
              auto status = _timeManager.getSyncStatus();
              if (status == TimeSyncStatus::SYNCED)
                  return {true, Cause::Met};
              if (status == TimeSyncStatus::IN_PROGRESS)
                  return {false, Cause::Connecting};
              return {false, Cause::Unreachable};
          },
          nullptr,  // single-cause — no describe needed (CG-16)
          syncEdges, 1);
  }
  ```
  Takes a `TimeManager&` reference. Check that `TimeManager` exposes `getSyncStatus()` (or equivalent) — if not, the worker must add a public accessor for `TimeSyncStatus`.

- [ ] **`modules/Application/Application.hpp`** — Add members:
  1. `WifiPreconditionContributor _wifiPrecondition;` (constructed with `_wifiManager` reference).
  2. `TimePreconditionContributor _timePrecondition;` (constructed with `_timeManager` reference).

- [ ] **`modules/Application/Application.cpp`** — In `begin()`, register the new precondition contributors:
  ```cpp
  _wifiPrecondition.registerPreconditions(_preconditionEngine);
  _timePrecondition.registerPreconditions(_preconditionEngine);
  ```
  Place these **before** `_preconditionEngine.resolveEdges()`.
  Keep the old `registerModule(&_wifiGate)` for now (parallel path during coexistence).

- [ ] **`test/test_unit/test_config_gate_resolver/`** — Add new test file or extend:
  - Test `WifiPreconditionContributor` registers exactly 1 precondition with Startup role.
  - Test `TimePreconditionContributor` registers exactly 2 preconditions with Operational role.
  - Test `Time Sync` has a Requires edge to `WiFi Station`.
  - Test severity derivation: WiFi in AP mode → `FallbackEngaged` / Degraded.

- [ ] Verify: native build + all tests pass.

## Verification Requirement

1. Native build with zero warnings.
2. All existing tests still pass.
3. New precondition contributor tests pass.
4. The old gate path still functions (coexistence).
5. Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

Surface any findings in the Completion Report — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
