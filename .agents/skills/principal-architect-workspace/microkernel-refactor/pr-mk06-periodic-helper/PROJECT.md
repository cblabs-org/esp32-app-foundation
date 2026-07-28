# PROJECT.md — pr-mk06: `Periodic` Helper (DRY the millis()-throttles)

**WO branch stack base:** `main` (independent side PR)
**Depends on:** — · **Blocks:** — (land early to reduce rebase churn)
**Design refs:** MK-13, MK-12 · **Gating decision:** none
**Status:** 📋 **PLANNED — awaiting human approval**

---

## Architectural Overview

~87 sites hand-roll "every N ms do X" via `millis()` throttling
(`_lastHeartbeatMs`, `GATE_CHECK_INTERVAL_MS`, carousel/screensaver timing, …).
This Project factors that duplicated pattern into a tiny **loop-context**
`Periodic{interval}` helper. It deliberately does **not** use FreeRTOS software
timers — their callbacks run in the timer-service task, reintroducing the exact
cross-task locking the cooperative loop avoids (MK-13). Here the "reinvention" is
the correct embedded call; it just wants DRYing.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| PH-01 | Introduce a header-only `Periodic` (holds interval + last-fire; `bool due(now)`) | Zero-cost abstraction; loop-context, no locking |
| PH-02 | Migrate the ~87 sites incrementally | Mechanical; keep each diff reviewable |
| PH-03 | **Not** `xTimerCreate` | Avoids cross-task context + locking (MK-13) |
| PH-04 | No behaviour change — pure refactor | Timing semantics must be identical |

## Strict Boundary Box

- Add the helper and migrate throttle sites **only**. **No behavioural change** —
  intervals and fire semantics identical.
- **Do not** touch module ownership, EventBus, or composition.
- Skip any throttle that is intentionally not periodic (e.g. one-shot guards).

## Tooling Constraints

`platformio` MCP tools; verify `native` + `esp32dev`. Existing timing tests must
pass unchanged.

## Proposed Work Order Ledger _(PA to finalise)_

- **[ ] wo1 — helper + first cohort:** add `Periodic`; migrate the
  `Application`/kernel throttles. `standard`
- **[ ] wo2 — module migration:** migrate remaining module throttle sites.
  `mechanical`

## Verification

Native + `esp32dev` green; timing tests unchanged; no measurable change in tick
behaviour; site count of raw `millis()`-throttles materially reduced.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval. Independent — safe to land early. |
