# PROGRAM.md — Microkernel Architecture Refactor

**Program ID:** `microkernel-refactor`
**Tier:** Chief Architect (Tier 1)
**Design authority:** [`docs/microkernel-architecture-design.md`](../../../../docs/microkernel-architecture-design.md) (decisions `MK-1`…`MK-18`, open questions `OQ-MK-1`…`OQ-MK-5`)
**Base branch:** `main`
**Status:** 🚀 **EXECUTING — PAs spawned for pr-mk01 (critical path), pr-mk02 (independent), pr-mk06 (independent).** Human granted full autonomous execution 2026-07-29.

---

## 1. Program Intent

Finish the microkernel. The framework is already a microkernel *in shape*
(`Application` = kernel, `registerModule` = registry, the interface set =
syscall surface, `EventBus` = IPC) but not *in discipline* — the kernel
hard-owns and `#include`s concrete modules, and "optional" modules are mostly
not optional. This program makes `Application` a **pure kernel** and routes
**every** module — framework and application alike — through the single
`registerModule` door, so the framework dogfoods its own extension API.

It must do so **without sacrificing ESP32 efficiency**: contracts (interfaces)
are cheap and added freely; orchestration mechanism is added only when a second
concrete case forces it.

See the design record for the full rationale. This document manages the
**macro dependency graph of Pull Requests (Projects)**; each Project is
decomposed into Work Orders by a Tier-2 Principal Architect at kickoff.

## 2. Three-Tier Orchestration Model

- **Tier 1 — Chief Architect (this program):** owns `PROGRAM.md` +
  `PROGRAM-PLAN.md`; defines the sequence/graph of Projects (PRs).
- **Tier 2 — Principal Architect (spawned per Project):** decomposes one
  Project into atomic Work Orders (`wo1`, `wo2`, …).
- **Tier 3 — Worker (spawned per Work Order):** executes a single WORKORDER on
  the filesystem within its Strict Boundary Box.

## 3. Project Registry (the ledger)

Stacked main line + two independent side PRs. Full topology and per-project
mandate in [`PROGRAM-PLAN.md`](./PROGRAM-PLAN.md).

| Project | Scope (MK decisions) | Depends on | Status |
|---|---|---|---|
| **[pr-mk01-eventbus-concurrency](./pr-mk01-eventbus-concurrency/PROJECT.md)** | Thread-safe, allocation-free EventBus (MK-11, MK-12) | `main` | 📋 Planned |
| **[pr-mk02-kernel-hardening](./pr-mk02-kernel-hardening/PROJECT.md)** | Tick-budget contract + debug instrumentation (MK-10) | `main` (independent) | ✅ **COMPLETE** — 2 WOs, tick budget contract + debug instrumentation |
| **[pr-mk03-module-composition](./pr-mk03-module-composition/PROJECT.md)** | Optionality axes + composition unit; extract HomeAssistant as proof (MK-2, MK-4–7) | pr-mk01 (sequencing) | 📋 Planned |
| **[pr-mk04-boot-seam-display](./pr-mk04-boot-seam-display/PROJECT.md)** | `IBootReporter` + display extraction + display-on-EventBus (MK-8, MK-9) | pr-mk01, pr-mk03 | 📋 Planned |
| **[pr-mk05-extract-optionals](./pr-mk05-extract-optionals/PROJECT.md)** | Extract MCP, OTA (default-off), Time via composition (MK-2, MK-6) | pr-mk03 | 📋 Planned |
| **[pr-mk06-periodic-helper](./pr-mk06-periodic-helper/PROJECT.md)** | DRY ~87 `millis()` throttles into a loop-context `Periodic` (MK-13) | `main` (independent) | 📋 Planned |
| **[pr-mk07-persistence-partitions](./pr-mk07-persistence-partitions/PROJECT.md)** | Four-tier persistence + partition profiles + config sizing (MK-14–18) | pr-mk05 **+ hardware-config program (cross-program)** | 📋 Planned |

## 4. Resolved Decisions

All five gating decisions have been resolved by the human (2026-07-29). Recorded
here and propagated to each Project's `PROJECT.md`.

| Decision | Resolution | Rationale |
|---|---|---|
| **OQ-MK-3** — EventBus backing | ✅ **Delegate to `esp_event`** | Reuse hardened existing libs wherever possible. Thread-safe, ISR-safe, alloc-free, already linked. All 3 consumers endorsed async dispatch. |
| **OQ-MK-2** — Composition home | ✅ **7a — Honest `main.cpp`** *(overrides leaning of 7b)* | Zero magic, fully greppable. Consumer controls their own composition explicitly. |
| **OQ-MK-1** — WebServer + AuthManager | ✅ **Core (Ring-1)** — always present, no enable flag | AP-mode first-run provisioning is the web portal; headless is the exception. |
| **OQ-MK-4** — Per-channel Golden config | ✅ **Yes — full implementation** | Per-channel golden (Stable/Beta/Dev) with full config data retention. Partition sizing absorbs the ~5× multiplier. |
| **OQ-MK-5** — Config payload `P` | ✅ **Resolved** (consumer reviews) | Per-target partition sizing: 1 MB on 16 MB boards, 256 KB on 4 MB boards. Real P: power-monitor ≈ 10–11 KB, departures single-digit KB, DraytonWiser ≈ 2–3 KB. |

## 5. Cross-Program Dependencies

- **hardware-config program** (`docs/hardware-configuration-design.md`, HC-1…HC-10,
  currently *pending PA scoping*): **pr-mk07** depends on `board.<id>.h` +
  `BOARD_DISPLAY_KIND` + partition selection. **pr-mk04** touches the display
  enable-flag surface (`BOARD_DISPLAY_KIND` vs `HAS_DISPLAY`/`HAS_LVGL_DISPLAY`,
  hardware-config OQ-2) and must be coordinated with it.
- **issue #40** (config/asset partition separation) and the **per-channel-golden
  config issue** (OQ-MK-4, to be filed): both feed **pr-mk07**.

### Consumer coordination (decided, #48)

**All downstream consumers (power-monitor, departures-board, DraytonWiser) will
pause their own migrations and wait for this program to complete, then build once
against the finished seams.** This retires the sequencing critical-path risk the
#48 review flagged: **no compatibility shim, no early interface-only headers, no
intermediate `framework.lock` pins.** Consumers still supply *design input* on the
RFC now (e.g. DraytonWiser's Preset-B requirements) — that is input, not the start
of their build, so there is no circular wait. The stack merge order (mk01 → mk03 →
mk04 → …) still holds for the program's *own* internal correctness, but is no
longer a consumer-unblocking deadline.

## 6. Pre-Kickoff Branch Safety ✅ COMPLETE

- ✅ `feat/carousel-can-advance` merged to `main` (27 commits, Option A).
- ✅ Design docs + program workspace committed to `main` (`549843d`).
- ✅ Both stashes dropped (fully superseded).
- ✅ `main` pushed to origin. Clean working tree.
- Base is established on `main` at `549843d`, ready for branch-cutting.

## 7. Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Program drafted from `docs/microkernel-architecture-design.md`. 7 Projects defined; dependency graph and gating decisions recorded. **Awaiting human approval of the decomposition before any PA/worker is spawned.** |
| 2026-07-29 | Consumer RFC #48 reviewed (power-monitor, departures-board, DraytonWiser). Design refinements applied: per-target `config` sizing (MK-18), MK-9 async-ordering contract, MK-6a optional-module-contract-on-facade, OTA pre-reboot hook (EO-08), FW-1 `canAdvance()` carry-forward (BS-07), native sync-stub contract (EB-06); Preset B deferred (no current consumer). **Consumer coordination decided: all downstream consumers wait for program completion — sequencing risk retired, no shim.** |
| 2026-07-29 | **Pre-flight complete.** `feat/carousel-can-advance` merged to `main`; stashes dropped; design docs + workspace committed (`549843d`); pushed to origin. |
| 2026-07-29 | **ALL GATING DECISIONS RESOLVED.** OQ-MK-1: Core. OQ-MK-2: 7a (honest `main.cpp`, overrides leaning). OQ-MK-3: `esp_event`. OQ-MK-4: full per-channel golden. OQ-MK-5: per-target sizing (resolved from consumer data). **Decomposition approved. Program status → APPROVED.** |
| 2026-07-29 | **HUMAN GRANTED FULL AUTONOMOUS EXECUTION.** "You have full approval to proceed autonomously — please record this so sub-agents have the proof." Testing = compile + native test verification; physical device deferred. PAs spawned for pr-mk01 (EventBus, critical path), pr-mk02 (tick budget, independent), pr-mk06 (Periodic, independent). Each PROJECT.md carries the approval record as committed provenance. **Program status → EXECUTING.** |

## 8. Distillation

_(Chief Architect distillation link added on program wrap-up — do not populate until projects are delivered.)_
