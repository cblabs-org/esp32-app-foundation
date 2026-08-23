---
name: 146-config-gate-wo3
description: Handoff instructions for Work Order 3 — Application Integration
complexity: architect
---

# WO3 — Application Integration & State Machine

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Wire `PreconditionEngine` into `Application`, replacing the `IConfigGate`-based state machine with the new precondition-based projection. This WO also removes the `DataManager._appRunning` gate (CG-17) and introduces the `precondition.health` EventBus topic (CG-18b).

**The old `IConfigGate` infrastructure must remain functional alongside the new system in this WO** — WO4 migrates the built-in gates, and WO7 removes the old interface. During this WO, both systems coexist.

## Context

Read:
- `docs/config-gate-design.md` — CG-1 (three questions), CG-14 (projection), CG-17 (DataManager gate removal), CG-18b (precondition.health topic)
- `modules/Application/Application.hpp` — especially L244 (registerModule template), L491–496 (gate storage), L305–317 (state machine methods)
- `modules/Application/Application.cpp` — L267–279 (boot transition), L291–365 (tick loop), L541–552 (gate registration), L694–696 (getActionableGate), L706–735 (_publishConfiguringEvent)
- `modules/DataManager/DataManager.cpp` — L268–270 (executeLoop gate), L340–342 (setAppRunning)
- `include/events/AppConfiguringPayload.hpp` — L29–36 (current payload)
- WO1 types: `include/interfaces/IPrecondition.hpp`, `include/interfaces/IPreconditionContributor.hpp`
- WO2 engine: `modules/PreconditionEngine/PreconditionEngine.hpp`

## Strict Boundary Box

- **DO NOT** modify `WifiConfigGate`, `TimeConfigGate`, `WeatherModule`, or `MyApp` — those migrate in WO4/WO7.
- **DO NOT** delete `IConfigGate.hpp` or `ConfigGateResolver.hpp` — that is WO7.
- **DO NOT** modify display modules or portal — those are WO5/WO6.
- **DO NOT** change sample-app or template-app.
- Keep the old `_configGates` vector and `_allGatesSatisfied()` functional as a parallel path during this WO.

## Tooling Constraints

- Use `platformio` MCP `build_project` (env: `native`) for compilation.
- Use `platformio` MCP `run_tests` (env: `native`) for test execution.

## Line-Item Instructions

- [ ] **`modules/Application/Application.hpp`** — Add:
  1. `#include "PreconditionEngine/PreconditionEngine.hpp"` and `#include "interfaces/IPreconditionContributor.hpp"`.
  2. New member: `PreconditionEngine _preconditionEngine;`
  3. In `registerModule<T>()` template (L244 area): add `if constexpr (std::is_base_of_v<IPreconditionContributor, T>) { module->registerPreconditions(_preconditionEngine); }` — auto-detection of the new interface.
  4. New public accessor: `PreconditionEngine& getPreconditionEngine() { return _preconditionEngine; }`
  5. New event topic constant: `static constexpr const char* PRECONDITION_HEALTH = "precondition.health";` in the `ApplicationEvents` namespace.

- [ ] **`include/events/PreconditionHealthPayload.hpp`** [NEW] — New event payload for `precondition.health` (CG-18b):
  ```cpp
  struct PreconditionHealthPayload {
      const char* name;
      Cause cause;
      Severity severity;
      bool everMet;
      // NO message/detail — subscribers call describe() into their own buffer (CG-2a)
  };
  ```

- [ ] **`modules/Application/Application.cpp`** — Modify the tick loop and state machine:
  1. In `begin()` (after gate registration, around L636): call `_preconditionEngine.resolveEdges()`. Log if it returns false (cycle detected).
  2. In `tick()`: call `_preconditionEngine.tick(millis())` every tick.
  3. **New projection logic (CG-14):** The boot transition uses `_preconditionEngine.allStartupMet()` in addition to `_allGatesSatisfied()` (parallel path during coexistence). Both must be true to enter RUNNING.
  4. **Precondition health publishing:** After `tick()`, for each precondition whose status changed since last publish, publish `PRECONDITION_HEALTH` with `PreconditionHealthPayload`. Throttle to same 500ms as existing configuring publish.
  5. **Regression logic:** The existing per-gate regression timer stays for old gates. For preconditions, regression is driven by the engine's damping — when a Startup-role precondition's damped status becomes suppressed, regress to CONFIGURING.

- [ ] **`modules/DataManager/DataManager.hpp`** — Remove the `_appRunning` gate (CG-17):
  1. Remove `std::atomic<bool> _appRunning{false};` member.
  2. Remove `void setAppRunning(bool running);` method.

- [ ] **`modules/DataManager/DataManager.cpp`** — Remove the gate:
  1. Remove `setAppRunning()` implementation.
  2. Remove the `if (!_appRunning.load()) return;` guard in `executeLoop()`.
  3. Data sources now fetch in all app states — this is the CG-17 resolution.

- [ ] **`modules/Application/Application.cpp`** — Remove calls to `_dataManager.setAppRunning()`:
  - Remove from `begin()` transitions (L271, L277).
  - Remove from `tick()` transitions (L314, L351).

- [ ] **`test/test_unit/test_application/test_application.cpp`** — Update tests:
  1. Tests that assert `setAppRunning` behavior need updating (the method no longer exists).
  2. Add new test cases:
     - `PreconditionEngine` is accessible via `app.getPreconditionEngine()`.
     - A registered `IPreconditionContributor` has its preconditions registered.
     - `allStartupMet()` gates boot transition.
     - `PRECONDITION_HEALTH` events are published on status change.
     - Regression is driven by damped precondition state.
  3. Keep existing gate-based tests functional (old path still works during coexistence).

- [ ] Verify: native build succeeds, all tests pass (old and new).

## Verification Requirement

1. Native build: zero warnings.
2. All existing tests pass (the old gate path still works).
3. New precondition-engine tests pass.
4. Confirm `DataManager::executeLoop()` no longer has the `_appRunning` guard.
5. Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report — do not file issues directly.

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
