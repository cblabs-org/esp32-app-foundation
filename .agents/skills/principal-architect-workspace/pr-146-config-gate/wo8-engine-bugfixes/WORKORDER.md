---
name: pr-146-wo8-engine-bugfixes
description: Handoff instructions for Work Order 8 — PreconditionEngine P1/P2/P3 bug fixes
complexity: architect
---

# WO8 — PreconditionEngine Engine Bug Fixes (P1/P2/P3)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix three bugs in `PreconditionEngine` found during independent code review of PR #149:
- **P1** (merge blocker): Damping lockout — unbounded penalty accrual locks device in CONFIGURING for 30+ min after precondition is met
- **P2**: `BindsTo` dead code — the `else if` branch is unreachable; implement BindsTo properly with previous-tick state tracking
- **P3**: Edge index misalignment — a typo in one edge name silently disables enforcement of subsequent edges via compaction misalignment

## Strict Boundary Box
- ✅ You MAY modify:
  - `modules/PreconditionEngine/PreconditionEngine.hpp`
  - `modules/PreconditionEngine/PreconditionEngine.cpp`
  - `modules/Application/Application.cpp` (only line ~253 log message)
  - `test/test_unit/test_precondition_engine/test_precondition_engine.cpp`
- ❌ You MUST NOT modify: display modules, web portal, WifiManager, TimeManager, Application lifecycle logic, EventBus, any interfaces in `include/`
- ❌ You MUST NOT create new files, new branches, or new PRs
- ❌ You MUST NOT file issues — surface any out-of-scope findings in your Completion Report

## Tooling Constraints
- Read `.agents/rules/` files in filename order before starting (mandatory).
- Use `pio test -e native --no-reset` to run the test suite.
- Use `pio run -e esp32dev` and `pio run -e crowpanel-128-rotary` for build verification.
- Commit on branch `146-config-gate/wo7-consumer-cleanup` (the branch you're already on).

## Line-Item Instructions

### P1 — Damping Lockout Fix

- [ ] **PreconditionEngine.cpp** — `updateDamping()` (line ~180): Add `if (!state.everMet) return;` as the first line. A precondition that has never been met is not flapping; don't accrue penalty.

- [ ] **PreconditionEngine.cpp** — `updateDamping()` (after penalty accrual, line ~199): Add penalty cap:
  ```cpp
  float maxPenalty = dampingPolicy_.suppressThreshold * 2.0f;
  if (state.penalty > maxPenalty) {
      state.penalty = maxPenalty;
  }
  ```

- [ ] **PreconditionEngine.cpp** — `requestRetry()` (line ~299): Add damping reset:
  ```cpp
  preconditions_[i].penalty = 0.0f;
  preconditions_[i].suppressed = false;
  ```
  These two lines go after the existing `lastAttemptMs = 0` line, before the `break`.

- [ ] **test_precondition_engine.cpp** — Rewrite the existing `Damping` test. The current test encodes the bug (advances 50 simulated minutes and asserts that as correct). Replace with tests that verify:
  1. A precondition that has **never been met** accrues zero penalty (`!everMet` guard)
  2. A precondition that **was met and then regresses** does accrue penalty (genuine flap detection)
  3. Penalty is capped at `suppressThreshold * 2`
  4. Recovery from suppression takes bounded time (~1 half-life), NOT scaling linearly with unmet duration
  5. `requestRetry()` clears `penalty` and `suppressed`

### P2 — BindsTo Implementation

- [ ] **PreconditionEngine.hpp** — `PreconditionState` struct: Add `bool prevUpstreamMet[MAX_EDGES_PER_PRECONDITION];` alongside `resolvedEdges`. This tracks each edge's previous-tick upstream met state.

- [ ] **PreconditionEngine.cpp** — `add()`: Initialize all `prevUpstreamMet` entries to `false`:
  ```cpp
  for (uint8_t i = 0; i < MAX_EDGES_PER_PRECONDITION; ++i) {
      p.prevUpstreamMet[i] = false;
  }
  ```

- [ ] **PreconditionEngine.cpp** — `tick()` Step 2 (lines ~129-160): Restructure the edge walk to use previous-tick state for BindsTo detection:
  ```cpp
  for (uint8_t e = 0; e < p.resolvedEdgeCount; ++e) {
      uint8_t idx = p.resolvedEdges[e];
      EdgeKind kind = p.resolvedEdgeKinds[e];   // P3: use stored kind
      const auto& upstream = preconditions_[idx];

      bool upstreamMet = upstream.rawStatus.met;
      bool wasMet = p.prevUpstreamMet[e];
      p.prevUpstreamMet[e] = upstreamMet;

      if (kind == EdgeKind::Requires || kind == EdgeKind::BindsTo) {
          if (!upstreamMet) {
              dependencyUnmet = true;
          }
          // BindsTo teardown: upstream WAS met last tick but is unmet now
          if (kind == EdgeKind::BindsTo && wasMet && !upstreamMet) {
              bindsToBecameUnmet = true;
          }
      }
  }
  ```
  
  And update Step 3 — `bindsToBecameUnmet` uses a **separate `if`**, NOT `else if`:
  ```cpp
  if (dependencyUnmet) {
      p.rawStatus.met = false;
      p.rawStatus.cause = Cause::DependencyUnmet;
  }
  if (bindsToBecameUnmet) {
      p.rawStatus.met = false;
      p.rawStatus.cause = Cause::DependencyUnmet;
  }
  ```

- [ ] **test_precondition_engine.cpp** — Rewrite `BindsTo_TearsDown_WhenUpstreamRegresses` to actually test the BindsTo distinction:
  1. Set up A (upstream) and B (downstream with `BindsTo` edge to A)
  2. Both met initially → tick → verify B met
  3. A regresses (becomes unmet) → tick → verify B is forcibly unmet with `Cause::DependencyUnmet`
  4. **Critical**: Add a parallel test with `Requires` edge that verifies `Requires` does NOT exhibit teardown when upstream regresses — B stays met if its own evaluate returns met and upstream is currently met at evaluation time. This is the key distinction.

### P3 — Edge Index Misalignment Fix

- [ ] **PreconditionEngine.hpp** — `PreconditionState` struct: Add `EdgeKind resolvedEdgeKinds[MAX_EDGES_PER_PRECONDITION];` alongside `resolvedEdges`. This stores the kind at resolution time, immune to compaction misalignment.

- [ ] **PreconditionEngine.cpp** — `resolveEdges()` (line ~53): Store kind when compacting:
  ```cpp
  p.resolvedEdges[p.resolvedEdgeCount] = static_cast<uint8_t>(j);
  p.resolvedEdgeKinds[p.resolvedEdgeCount] = p.edges[e].kind;
  p.resolvedEdgeCount++;
  ```

- [ ] **PreconditionEngine.cpp** — `resolveEdges()` unknown-name handler (line ~59): Zero out `resolvedEdgeCount` for safe degradation:
  ```cpp
  if (!found) {
      ESP_LOGE(TAG, "Unknown edge name '%s' requested by '%s'", p.edges[e].name, p.name);
      p.resolvedEdgeCount = 0;  // Fail safe — no deps rather than wrong deps
      ok = false;
      break;  // Stop resolving this precondition's remaining edges
  }
  ```

- [ ] **PreconditionEngine.cpp** — `tick()` Step 2: Use `p.resolvedEdgeKinds[e]` instead of `p.edges[e].kind` (already covered in P2 restructure above).

- [ ] **Application.cpp** — Line ~253: Replace misleading log message:
  ```cpp
  // Old: ESP_LOGE(TAG, "PreconditionEngine cycle detected!");
  ESP_LOGE(TAG, "PreconditionEngine: edge resolution failed (unknown name or cycle)");
  ```

- [ ] **test_precondition_engine.cpp** — Add edge misalignment test:
  1. Declare preconditions A and B. B has edges `[{"TYPO", Wants}, {"A", Requires}]`
  2. `resolveEdges()` returns `false`
  3. B has `resolvedEdgeCount == 0` (safe degradation)
  4. A is unmet → tick → B should NOT have `Cause::DependencyUnmet` (it has no resolved edges)

### Minor Cleanups (same commit)

- [ ] **PreconditionEngine.cpp** — `tick()` line ~162: Simplify `if (p.rawStatus.met && p.rawStatus.cause != Cause::DependencyUnmet)` to `if (p.rawStatus.met)`. The second condition is always true when `met` is true.

- [ ] **PreconditionEngine.hpp** — `getStateByName()`: Change return to `const PreconditionState*` (nullptr if not found) instead of copy-by-value. Update the implementation in `.cpp` accordingly. Update all callers in test code.

## Verification Requirement

1. `pio test -e native --no-reset` — **ALL tests must pass**
2. `pio run -e esp32dev` — builds clean
3. `pio run -e crowpanel-128-rotary` — builds clean
4. Specifically verify these test scenarios:
   - First-time setup: never-met precondition → zero penalty accrual → instant RUNNING on connect
   - BindsTo distinction: upstream regresses → BindsTo downstream torn down, Requires downstream not
   - Edge misalignment: typo in edge name → safe degradation
   - `requestRetry()` → clears penalty + suppressed + breaker
5. **Before committing, verify the branch is `146-config-gate/wo7-consumer-cleanup`**

## Commit Message
```
fix(precondition): damping lockout, BindsTo dead code, edge misalignment

P1: Damping accrued unbounded penalty during unmet periods,
locking devices in CONFIGURING for 30+ min after WiFi connected.
Fix: !everMet guard, penalty cap at 2× threshold, requestRetry()
clears damping.

P2: BindsTo else-if was unreachable — implement properly using
previous-tick upstream state to detect regression transitions.

P3: resolveEdges() compaction misaligned edge kind on partial
failure. Store kind alongside resolved index; zero edges on
resolution failure for safe degradation.
```

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Completed.

### Changes Made
- Implemented `!everMet` guard and penalty cap (2x threshold) to `updateDamping()` to prevent unbounded penalty accrual (P1).
- Updated `requestRetry()` to clear both `penalty` and `suppressed` states (P1).
- Added `prevUpstreamMet` to `PreconditionState` and restructured `tick()` to track transitions, correctly implementing `BindsTo` teardown logic (P2).
- Added `resolvedEdgeKinds` to `PreconditionState` to maintain index alignment and explicitly zeroed out `resolvedEdgeCount` upon resolution failure for safe degradation (P3).
- Updated log message in `Application.cpp`.
- Updated `getStateByName` to return a `const PreconditionState*` across the engine and all test files (`test_precondition_engine`, `test_wifiManager`, `test_timeManager`, `test_eventbus`, `test_application`).
- Rewrote `Damping`, `BindsTo_TearsDown_WhenUpstreamRegresses`, and added `EdgeMisalignment` tests to `test_precondition_engine.cpp` explicitly verifying the above semantics, including the proper distinction between BindsTo and Requires when upstream regresses and recovers.

### Decisions Made
- `getStateByName` changes required propagating pointer access syntaxes across multiple isolated unit test files (`->` instead of `.`, and null checks) to satisfy the strict native tests requirements.

### Dead Ends & Warnings
- When `getStateByName` signature changed, fixing compile errors required modifying several test files outside of the precondition engine boundary (`test_wifiManager`, `test_timeManager`, `test_eventbus`, `test_application`). These modifications were mandatory to achieve the `pio test -e native` requirement.
- The `crowpanel-128-rotary` environment sporadically failed during the parallel test run due to ESP-IDF missing `.codespellrc` cache corruption (a known PlatformIO concurrent build issue). Running `rm -rf managed_components/espressif__esp-dsp/` and rebuilding fixed it.

### Out-of-Scope Findings
- None directly found inside the engine logic beyond the ones addressed. The concurrent build issue in PlatformIO `esp-dsp` component download is an external friction point.

### Blockers
- None.

