# PROJECT.md — pr-mk01: EventBus Concurrency & Allocation Contract

**WO branch stack base:** `main` (wo1 bases on `main`)
**Depends on:** — (prerequisite; nothing)
**Blocks:** pr-mk03 (sequencing), pr-mk04 (display-on-EventBus needs a safe bus)
**Design refs:** MK-11, MK-12 · **Gating decision:** **OQ-MK-3** (must be resolved before PA kickoff)
**Status:** ✅ **APPROVED — human approved decomposition + full autonomous execution (2026-07-29)**

---

## Architectural Overview

`EventBus` is the microkernel's IPC bus, and the program leans harder on it
(MK-9 moves display state onto it). Today it is unsafe for that role:
[`EventBus::publish`](../../../../modules/EventBus/EventBus.cpp) holds **no lock**
and **copies the whole subscriber vector (with `std::string` topics) on every
publish** — a data race against the confirmed multi-task/multi-core reality
(DataManager, OTA, AuthManager tasks + AsyncTCP on core 1) and a per-event heap
allocation on the hot path. This Project gives the bus a defined concurrency
contract and removes the allocation, **without changing its public interface**
(preserving native testability).

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| EB-01 | Keep the `EventBus` interface; change the implementation only | Native GoogleTest build must keep compiling; callers unchanged where possible |
| EB-02 | **OQ-MK-3 RESOLVED:** back on-target impl with ESP-IDF `esp_event` (delegating to hardened existing libs) | `esp_event` is thread-safe, ISR-safe, already linked and used by HomeAssistant — reuse a hardened primitive (MK-12). All 3 consumers endorsed async dispatch. |
| EB-03 | Intern topics as hashed `uint32_t`/`const char*` identity | Removes per-publish `std::string` allocation (efficiency mandate) |
| EB-04 | Publish a written **concurrency contract** (who may publish/subscribe, from which context) | There is none today; the whole IPC premise depends on it |
| EB-05 | If `esp_event`: document the sync→async dispatch semantic change | Callbacks move to the event-task context; audit subscribers assuming in-caller synchronous dispatch |
| EB-06 | The **native sync-stub dispatch is a defined, documented contract** | Consumers (departures-board, power-monitor) run native tests over `IDataSource`+bus; the stub's ordering/semantics must be stable and specified, not incidental (#48 meta-review) |

## Strict Boundary Box (for the PA/workers)

- Touch **only** `modules/EventBus/*`, the EventBus concurrency contract, and the
  minimum publisher/subscriber call-site changes required by EB-03/EB-05.
- **Do not** begin module extraction, the boot seam, or composition work.
- **Do not** change `Application`'s module ownership.

## Tooling Constraints

Use the `platformio` MCP tools (`build_project`, `run_tests`) — **not** raw `pio`
CLI (rule 04). Native test env: `native`. Verify both `native` and at least one
hardware env (`esp32dev`) compile.

## Proposed Work Order Ledger _(PA to finalise)_

- **[ ] wo1 — contract + interning:** define the concurrency contract; replace
  `std::string` topics with interned identity; in-place dispatch. `standard`
- **[ ] wo2 — on-target backing:** implement the OQ-MK-3 choice (`esp_event`
  delegation or `portMUX`) behind the interface; native sync stub. `architect`
- **[ ] wo3 — caller migration + tests:** migrate publishers/subscribers; add
  concurrency + regression tests. `standard`

## Verification

Native suite green; `esp32dev` builds; no new heap allocation per publish
(spot-check); a concurrency test exercising cross-task publish/subscribe.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval + OQ-MK-3 resolution before PA spawn. |
| 2026-07-29 | **OQ-MK-3 RESOLVED → `esp_event` delegation.** Human approved full autonomous execution of the microkernel-refactor program (all 7 projects). Consumer review complete (3/3 endorsed via RFC #48). Approval recorded in `PROGRAM.md` status log. PA spawn authorised. |
