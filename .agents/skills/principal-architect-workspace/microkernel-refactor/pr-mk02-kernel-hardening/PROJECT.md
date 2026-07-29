# PROJECT.md — pr-mk02: Kernel Hardening (Tick-Budget Contract)

**WO branch stack base:** `main` (independent side PR)
**Depends on:** — · **Blocks:** — (land early to avoid rebase churn)
**Design refs:** MK-10 · **Gating decision:** none
**Status:** ✅ **APPROVED — human approved decomposition + full autonomous execution (2026-07-29)**

---

## Architectural Overview

Under dogfooding (MK-2), application modules register into the **same** tick
vector as framework modules, and the **Task Watchdog is active** (DataManager +
OTA feed it). A single overrunning `tick()` therefore stalls every module *and
can reset the whole device* — the honest embedded consequence of a structural
(non-memory-isolated) microkernel (MK-3). This Project makes the tick budget a
first-class contract and adds cheap debug-build visibility. **No scheduler** — the
over-engineering line is firmly held.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| KH-01 | Promote "≤10 ms per `tick()`" from a comment to an explicit `ITickable` contract clause | Make the obligation discoverable and reviewable |
| KH-02 | Debug-build-only per-module tick-duration instrumentation (`millis()` wrap + `ESP_LOGW` over budget) in `Application::tick()` | Turns "random resets" into "module X took 240 ms"; zero release-build cost |
| KH-03 | No priority arbitration / no per-module task isolation | YAGNI; add only if a second concrete case forces it |

## Strict Boundary Box

- Touch **only** the `ITickable` contract/docs and the `Application::tick()`
  dispatch loop instrumentation.
- **Do not** alter module ownership, EventBus, or composition.
- Instrumentation must be compiled out in release builds (guard on a debug flag).

## Tooling Constraints

`platformio` MCP tools; verify `native` + `esp32dev`. Confirm the instrumentation
is absent from the release binary (no size/latency regression).

## Proposed Work Order Ledger _(PA to finalise)_

- **[x] [wo1-contract-docs](wo1-contract-docs/WORKORDER.md):** `ITickable` budget clause; update
  `creating-a-module.md` / performance guide. `mechanical`
- **[x] [wo2-debug-instrumentation](wo2-debug-instrumentation/WORKORDER.md):** guarded tick-duration timing in the
  dispatch loop. `standard`

## Verification

Native + `esp32dev` green; debug build logs an over-budget warning for a
deliberately-slow test module; release build unchanged in size.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval. Independent — safe to land early. |
| 2026-07-29 | Human approved full autonomous execution of the microkernel-refactor program (all 7 projects). Consumer review complete (3/3 endorsed via RFC #48). Approval recorded in `PROGRAM.md` status log. PA spawn authorised. |
