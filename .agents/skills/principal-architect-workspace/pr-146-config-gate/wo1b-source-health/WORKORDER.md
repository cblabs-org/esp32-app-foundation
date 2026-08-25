---
name: 146-config-gate-wo1b
description: Handoff instructions for Work Order 1b — SourceHealth Model (prerequisite from #128)
complexity: standard
---

# WO1b — SourceHealth Model & DataManager Health Tracking

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Implement the `SourceHealth` model that CG-15 and CG-4a depend on. This delivers
the **minimum viable health tracking** from the #128 DataManager resilience program:
per-source health state, consecutive-failure counting, `everSucceeded` tracking, and
`UpdateStatus::Implausible`. The full backoff/quarantine state machine and recovery
probing from #128 are **out of scope** — they enhance DataManager's own resilience
but are not required by the precondition engine.

## Context

Read:
- `docs/config-gate-design.md` — CG-15 (one health vocabulary with SourceHealth),
  CG-4a (severity discriminated by trust history), CG-5a (Implausible)
- `include/interfaces/IDataSource.hpp` — current interface: `UpdateStatus` enum,
  `fetchData()`, `getParsedData()`. No health methods today.
- `modules/DataManager/DataManager.hpp` — current `SourceRecord` struct (just source
  pointer, lastFetchTimeMs, priorityRequested)
- `modules/DataManager/DataManager.cpp` — `executeLoop()` fetch cycle, `_fetchStartMs`
  timing instrumentation

**What exists on the unmerged `128-datamanager-resilience/wo2-source-health-backoff`
branch:** A more elaborate health model with quarantine state machine. We are building
a simpler version that provides the interface the precondition engine needs, without
the full quarantine/backoff machinery (that remains in #128's scope for later).

**What the precondition engine needs from SourceHealth (CG-15, CG-4a):**
- `everSucceeded` — has this source ever returned a valid payload? (Used by
  `deriveSeverity()` for the `Unreachable`/`Implausible` trust discrimination.)
- `consecutiveFailures` — how many fetches have failed in a row?
- `lastStatus` — what was the last `UpdateStatus` result?
- `lastSuccessMs` — when did it last succeed? (For stale-data detection.)
- `SourceHealthState` — aggregate health: Healthy, Degraded, Unreachable.

## Strict Boundary Box

- **DO NOT** implement backoff scheduling, quarantine state machine, or recovery
  probing — those are #128 WO3+ scope.
- **DO NOT** modify `Application`, display modules, portal, sample-app, or template-app.
- **DO NOT** modify the precondition types from WO1 — they are already written.
- Stay within `IDataSource.hpp`, `DataManager`, and new test files.

## Tooling Constraints

- Use `platformio` MCP `build_project` (env: `native`) for compilation.
- Use `platformio` MCP `run_tests` (env: `native`) for test execution.

## Line-Item Instructions

- [ ] **`include/interfaces/IDataSource.hpp`** — Add `UpdateStatus::Implausible`:
  ```cpp
  enum class UpdateStatus : uint8_t {
      Success = 0,
      NoChange,
      RateLimited,
      FetchError,
      ParseError,
      AuthError,
      Implausible    // NEW: CG-5a — transport succeeded, domain check rejected the answer
  };
  ```
  Add a doc comment explaining: the source got a response but the payload is
  semantically invalid (e.g., a temperature sensor returning −273°C, an energy
  meter reporting negative kWh). This is distinct from `ParseError` (couldn't
  decode) and `FetchError` (couldn't reach).

- [ ] **`include/interfaces/IDataSource.hpp`** — Add `SourceHealthState` enum and
  `SourceHealth` struct (place after `UpdateStatus`):
  ```cpp
  enum class SourceHealthState : uint8_t {
      Healthy = 0,    ///< Last fetch succeeded or returned NoChange
      Degraded,       ///< Intermittent failures (< threshold consecutive)
      Unreachable     ///< Sustained failures (>= threshold consecutive)
  };

  struct SourceHealth {
      SourceHealthState state   = SourceHealthState::Healthy;
      UpdateStatus lastStatus   = UpdateStatus::FetchError;
      uint16_t consecutiveFailures = 0;
      uint32_t lastSuccessMs    = 0;
      uint32_t lastAttemptMs    = 0;
      bool everSucceeded        = false;

      /// Convenience: true if this source has never produced a valid result
      bool neverConnected() const { return !everSucceeded; }
  };
  ```

- [ ] **`modules/DataManager/DataManager.hpp`** — Extend `SourceRecord` with health tracking:
  ```cpp
  struct SourceRecord {
      IDataSource* source = nullptr;
      uint32_t lastFetchTimeMs = 0;
      bool priorityRequested = false;
      SourceHealth health;               // NEW
  };
  ```
  Add a public accessor:
  ```cpp
  const SourceHealth* getSourceHealth(const char* sourceName) const;
  ```
  Also add a constant for the degraded→unreachable threshold:
  ```cpp
  static constexpr uint16_t UNREACHABLE_THRESHOLD = 5;
  ```

- [ ] **`modules/DataManager/DataManager.cpp`** — Update `executeLoop()` fetch cycle
  to track health after each fetch:
  1. After calling `source->fetchData()` and receiving an `UpdateStatus`:
     ```cpp
     auto& h = record.health;
     h.lastStatus = status;
     h.lastAttemptMs = nowMs;
     if (status == UpdateStatus::Success || status == UpdateStatus::NoChange) {
         h.consecutiveFailures = 0;
         h.lastSuccessMs = nowMs;
         h.everSucceeded = true;
         h.state = SourceHealthState::Healthy;
     } else {
         h.consecutiveFailures++;
         h.state = (h.consecutiveFailures >= UNREACHABLE_THRESHOLD)
                   ? SourceHealthState::Unreachable
                   : SourceHealthState::Degraded;
     }
     ```
  2. Implement `getSourceHealth()`:
     ```cpp
     const SourceHealth* DataManager::getSourceHealth(const char* sourceName) const {
         for (const auto& rec : _sources) {
             if (rec.source && strcmp(rec.source->getSourceName(), sourceName) == 0)
                 return &rec.health;
         }
         return nullptr;
     }
     ```
     (Check that `IDataSource` has a `getSourceName()` method — if not, add one or
     use the index-based lookup that DataManager already supports.)

- [ ] **`modules/DataManager/DataManager.hpp`** — Add EventBus topic for health changes:
  ```cpp
  static constexpr const char* DATA_SOURCE_HEALTH = "data.source.health";
  ```

- [ ] **`modules/DataManager/DataManager.cpp`** — Publish health changes:
  After updating health state, if `state` changed from the previous value, publish
  to `DATA_SOURCE_HEALTH` with a payload struct:
  ```cpp
  struct DataSourceHealthPayload {
      const char* sourceName;
      SourceHealthState state;
      UpdateStatus lastStatus;
      uint16_t consecutiveFailures;
      bool everSucceeded;
  };
  ```
  Define this payload in `include/events/DataSourceHealthPayload.hpp` [NEW].

- [ ] **`test/test_unit/test_datamanager/`** — Add health tracking tests:
  1. Successful fetch sets `Healthy`, clears `consecutiveFailures`, sets `everSucceeded`.
  2. Failed fetch increments `consecutiveFailures`.
  3. 5 consecutive failures → `Unreachable`.
  4. Recovery after failures → back to `Healthy`, `consecutiveFailures` reset.
  5. `Implausible` status treated as a failure (not success).
  6. `everSucceeded` persists across failures (once true, stays true).
  7. `neverConnected()` returns true until first success.

  Check existing test structure — if `test_datamanager` tests already exist, add
  to them. If not, create the test file.

- [ ] Verify: native build + all tests pass.

## Verification Requirement

1. Native build with zero warnings.
2. All existing tests still pass.
3. New health tracking tests pass.
4. `UpdateStatus::Implausible` compiles in all environments.
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
