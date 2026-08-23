---
name: 146-config-gate-wo2
description: Handoff instructions for Work Order 2 — PreconditionEngine core logic
complexity: architect
---

# WO2 — PreconditionEngine Core Logic

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Implement the `PreconditionEngine` — the central evaluation, damping, breaker, and edge-resolution engine. This is **pure logic with an injected clock**, hardware-free, and comprehensively natively-testable. It is the most important WO in the project.

## Context

Read the design document: `docs/config-gate-design.md` — particularly:
- CG-7 (edge resolution at registration, cycle rejection)
- CG-8 (satisfaction propagates immediately, penalty never)
- CG-4 + CG-4a (severity derivation from cause + trust history)
- CG-9 (retry escalation)
- CG-11 (breaker: closed/open/half-open)
- CG-12 (damping: per-second penalty, asymmetric thresholds, severity-scaled)
- CG-13 (publish damped AND undamped status)

Read the types created in WO1: `include/interfaces/IPrecondition.hpp`, `include/utils/PreconditionRegistry.hpp`.

The existing `ConfigGateResolver` at `include/utils/ConfigGateResolver.hpp` is the predecessor — read it to understand the shape being replaced, but **do not modify or delete it**.

## Strict Boundary Box

- **DO NOT** modify `Application`, `DataManager`, display modules, sample-app, or template-app.
- **DO NOT** modify or delete `IConfigGate.hpp` or `ConfigGateResolver.hpp`.
- **DO NOT** modify any existing test files.
- You are creating new files in `modules/PreconditionEngine/` (or `include/utils/`) and `test/`.

## Tooling Constraints

- Use `platformio` MCP `build_project` (env: `native`) for compilation.
- Use `platformio` MCP `run_tests` (env: `native`) for test execution.
- Do not use raw `pio` CLI commands.

## Line-Item Instructions

- [ ] **`modules/PreconditionEngine/PreconditionEngine.hpp`** [NEW] — Header for the engine class.

  The engine:
  - Inherits from `PreconditionRegistry` (implements the `add()` method)
  - Stores registered preconditions in a fixed-size internal array (max 16 preconditions — `MAX_PRECONDITIONS`)
  - Per-precondition internal state struct:
    ```cpp
    struct PreconditionState {
        const char* name;
        PreconditionRole role;
        PreconditionRegistry::EvalFn evaluate;
        PreconditionRegistry::DescribeFn describe;
        const Edge* edges;
        uint8_t edgeCount;
        // Resolved edge indices (CG-7)
        uint8_t resolvedEdges[MAX_EDGES_PER_PRECONDITION];
        uint8_t resolvedEdgeCount = 0;
        // Runtime state
        PreconditionStatus rawStatus;         // undamped
        PreconditionStatus dampedStatus;      // damped
        Severity severity = Severity::Transient;
        bool everMet = false;                 // CG-4a trust history
        uint32_t identityEpoch = 0;           // CG-4a: bumped when target changes
        // Damping state (CG-12)
        float penalty = 0.0f;
        bool suppressed = false;
        // Breaker state (CG-11)
        BreakerState breaker = BreakerState::Closed;
        uint8_t attemptCount = 0;
        uint32_t lastAttemptMs = 0;
        uint32_t cooldownMs = 0;
    };
    ```
  - Public API:
    ```cpp
    void add(...) override;                    // PreconditionRegistry
    bool resolveEdges();                       // CG-7: call after all add()s; returns false if cycle
    void tick(uint32_t nowMs);                 // Main evaluation — call every tick
    bool allStartupMet() const;               // CG-14 projection
    bool getAppRunning() const;               // CG-14 aggregate
    size_t getPreconditionCount() const;
    const PreconditionState& getStateAt(size_t i) const;
    bool getStateByName(const char* name, PreconditionState& out) const;
    void bumpIdentityEpoch(const char* name); // CG-4a
    void requestRetry(const char* name);      // CG-11: user-initiated, breaker MUST NOT suppress
    ```

- [ ] **`modules/PreconditionEngine/PreconditionEngine.cpp`** [NEW] — Implementation.

  **`add()` implementation:**
  - Stores the precondition data. Does NOT resolve edges yet (deferred to `resolveEdges()`).

  **`resolveEdges()` implementation (CG-7):**
  - For each precondition, resolve each edge name to an index via `strcmp` against all registered names.
  - **Reject unknown names** — log an error and return false.
  - **Reject cycles** — use a DFS/visited-set cycle detection. Log and return false.
  - Log the resolved graph once (at INFO level) so the shape is inspectable at boot.

  **`tick(nowMs)` implementation:**
  1. For each precondition, in registration order:
     a. Check `Requires`/`BindsTo` edges. If any upstream is unmet, set cause to `DependencyUnmet` and skip own evaluation. `DependencyUnmet` suppresses trust updates (CG-4a precedence rule).
     b. Otherwise, call `evaluate()` to get raw `PreconditionStatus`.
     c. If `BindsTo` edge and upstream became unmet since last tick → force this precondition unmet immediately (CG-8).
     d. Update `everMet` if `rawStatus.met && cause != DependencyUnmet`.
     e. Derive `severity` via `deriveSeverity(cause, everMet)`.
     f. Update damping: accumulate penalty by `penaltyPerSecond * severity_weight * elapsed_seconds`. Decay penalty by half-life. Check suppress/reuse thresholds.
     g. Update breaker: count attempts, escalate after N failures (CG-9), manage closed/open/half-open transitions.
     h. Compute `dampedStatus` — if suppressed, dampedStatus is unmet; otherwise dampedStatus = rawStatus.
  2. Satisfaction propagates **immediately** via raw status, undamped (CG-8).
  3. Penalty **never** propagates across edges (CG-8).

  **`deriveSeverity()` implementation (CG-4 + CG-4a):**
  ```
  Met             → (n/a, precondition is met)
  Connecting      → Transient
  Unreachable     → everMet ? Transient : NeedsUser
  RateLimited     → Transient
  Implausible     → everMet ? Transient : NeedsUser
  NotConfigured   → NeedsUser
  Rejected        → NeedsUser
  DependencyUnmet → Blocked
  Indeterminate   → Persistent
  FallbackEngaged → Degraded
  Unavailable     → Inactive
  InternalError   → Persistent
  default         → Transient (safe default — CG-4)
  ```

  **Damping weight by severity:**
  - `NeedsUser`: weight 5 (reaches suppression fast — correct for config-shaped preconditions)
  - `Persistent`: weight 5
  - `Transient`: weight 1 (default)
  - `Blocked`: weight 0 (no damping — someone else owns it)
  - `Degraded`: weight 1
  - `Inactive`: weight 0

- [ ] **`test/test_unit/test_precondition_engine/test_precondition_engine.cpp`** [NEW] — Comprehensive native test suite.

  Test groups:
  1. **Registration & edge resolution** — add preconditions, resolve edges, verify indices. Test unknown edge name rejection. Test cycle detection.
  2. **Basic evaluation** — tick with simple predicates, verify rawStatus, dampedStatus.
  3. **Edge propagation** — Requires edge: upstream unmet → DependencyUnmet. BindsTo: upstream becomes unmet at runtime → immediate propagation. Wants: no propagation.
  4. **Severity derivation** — each Cause × everMet combination per CG-4 table.
  5. **Trust history** — `everMet` set on first met; cleared by `bumpIdentityEpoch`; not set while `DependencyUnmet`.
  6. **Damping** — penalty accumulates while unmet, decays while met. Suppression threshold crossed → suppressed. Below reuse → unsuppressed. Asymmetric thresholds prevent bouncing.
  7. **Breaker** — N failures → Open. Cooldown elapses → HalfOpen. Success → Closed. Failure in HalfOpen → Open with longer cooldown. `requestRetry()` overrides open breaker (CG-11 WARNING).
  8. **Projection** — `allStartupMet()` only considers Startup/Both roles. Operational preconditions do not affect it.
  9. **CG-8 invariant** — satisfaction propagates immediately (undamped); penalty never propagates across edges. Chain teardown is `max()` not product.

  Use a mock clock (injected `uint32_t nowMs`) for deterministic testing. Use GoogleTest (`TEST()` macros). Place in `test/test_unit/test_precondition_engine/`.

- [ ] Verify: native build succeeds and all new tests pass.

## Verification Requirement

1. `pio run -e native` compiles with zero warnings from new files.
2. `pio test -e native -f test_precondition_engine` — all tests pass.
3. No existing tests are broken.
4. Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

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
