# PROGRAM.md — Microkernel Architecture Refactor

**Program ID:** `microkernel-refactor`
**Tier:** Chief Architect (Tier 1)
**Design authority:** [`docs/microkernel-architecture-design.md`](../../../../docs/microkernel-architecture-design.md) (decisions `MK-1`…`MK-18`, open questions `OQ-MK-1`…`OQ-MK-5`)
**Base branch:** `main`
**Status:** 📋 **PLANNED — awaiting human approval of the decomposition** (plan gate, rules 00/01). No branches created, no workers spawned.

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
| **[pr-mk02-kernel-hardening](./pr-mk02-kernel-hardening/PROJECT.md)** | Tick-budget contract + debug instrumentation (MK-10) | `main` (independent) | 📋 Planned |
| **[pr-mk03-module-composition](./pr-mk03-module-composition/PROJECT.md)** | Optionality axes + composition unit; extract HomeAssistant as proof (MK-2, MK-4–7) | pr-mk01 (sequencing) | 📋 Planned |
| **[pr-mk04-boot-seam-display](./pr-mk04-boot-seam-display/PROJECT.md)** | `IBootReporter` + display extraction + display-on-EventBus (MK-8, MK-9) | pr-mk01, pr-mk03 | 📋 Planned |
| **[pr-mk05-extract-optionals](./pr-mk05-extract-optionals/PROJECT.md)** | Extract MCP, OTA (default-off), Time via composition (MK-2, MK-6) | pr-mk03 | 📋 Planned |
| **[pr-mk06-periodic-helper](./pr-mk06-periodic-helper/PROJECT.md)** | DRY ~87 `millis()` throttles into a loop-context `Periodic` (MK-13) | `main` (independent) | 📋 Planned |
| **[pr-mk07-persistence-partitions](./pr-mk07-persistence-partitions/PROJECT.md)** | Four-tier persistence + partition profiles + config sizing (MK-14–18) | pr-mk05 **+ hardware-config program (cross-program)** | 📋 Planned |

## 4. Open Decisions Gating Kickoff

These are the human's to resolve; the Chief must not spawn the gated Project's
PA until each is settled (recorded here, then in the Project's `PROJECT.md`).

| Decision | Gates | Leaning |
|---|---|---|
| **OQ-MK-3** — EventBus: delegate to `esp_event` (async dispatch) vs keep-and-lock synchronous | pr-mk01 | Delegate to `esp_event` (fixes race + allocation by reuse) |
| **OQ-MK-2** — Composition home: honest `main.cpp` (7a) vs thin `ModuleComposition` unit (7b) | pr-mk03 | 7b |
| **OQ-MK-1** — WebServer + AuthManager: Core or Optional? | pr-mk03, pr-mk05 (scope) | Core |
| **OQ-MK-4** — Config lifecycle: full-data retention + per-channel Golden (Stable/Beta/Dev) | pr-mk07 | Yes (raise as its own issue; partition sized for its worst case regardless) |
| **OQ-MK-5** — Heavy-consumer config payload `P` sizing | pr-mk07 | Pending PowerMonitor input (issue #40) |

## 5. Cross-Program Dependencies

- **hardware-config program** (`docs/hardware-configuration-design.md`, HC-1…HC-10,
  currently *pending PA scoping*): **pr-mk07** depends on `board.<id>.h` +
  `BOARD_DISPLAY_KIND` + partition selection. **pr-mk04** touches the display
  enable-flag surface (`BOARD_DISPLAY_KIND` vs `HAS_DISPLAY`/`HAS_LVGL_DISPLAY`,
  hardware-config OQ-2) and must be coordinated with it.
- **issue #40** (config/asset partition separation) and the **per-channel-golden
  config issue** (OQ-MK-4, to be filed): both feed **pr-mk07**.

## 6. Pre-Kickoff Branch Safety

- Working tree is currently on `feat/carousel-can-advance`, **not** `main`, with
  the two design docs (`docs/microkernel-architecture-design.md`,
  `docs/hardware-configuration-design.md`) **untracked**.
- Before kicking off pr-mk01, the design docs + this program workspace should be
  committed and the base established on `main`. This is an implementation action
  requiring the human's explicit go — the Chief does not commit or branch during
  planning.

## 7. Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Program drafted from `docs/microkernel-architecture-design.md`. 7 Projects defined; dependency graph and gating decisions recorded. **Awaiting human approval of the decomposition before any PA/worker is spawned.** |

## 8. Distillation

_(Chief Architect distillation link added on program wrap-up — do not populate until projects are delivered.)_
