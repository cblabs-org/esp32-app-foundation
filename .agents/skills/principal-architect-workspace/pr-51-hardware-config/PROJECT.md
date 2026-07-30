# PR-51 — Hardware Configuration (`board.h` pin-map migration)

> **Principal Architect project.** Decomposition of the hardware-configuration
> design into a linear stack of Work Orders.

- **Design authority:** [`docs/hardware-configuration-design.md`](../../../../docs/hardware-configuration-design.md) — HC-1…HC-12, all OQs closed except non-blocking OQ-7.
- **Consumer sign-off:** RFC [#50](https://github.com/matthewmcneill/esp32-app-foundation/issues/50) — endorsed by power-monitor, departures-board, DraytonWiser; **no blockers**. Must-fixes already folded into the design doc.
- **Base branch:** `main` @ `39c2777` (the full microkernel refactor MK-1…MK-18 has landed here).
- **Relationship to microkernel program:** completes the `hardware-config` dependency referenced by [`pr-mk07-persistence-partitions`](../microkernel-refactor/pr-mk07-persistence-partitions/PROJECT.md). MK landed availability/composition (`WITH_DISPLAY_*`), partition-runtime (`ConfigRoot`), and honest-`main.cpp` composition — but **not** the `board.h` pin map. That is this project's entire scope.

---

## Status

📦 **FULLY PREPPED — READY FOR IMPLEMENTING PA HANDOFF.**
**No branches created. No workers spawned. No code written.**

All four `WORKORDER.md` files are authored (WO1–WO4) and this decomposition is
complete. Prepared by the **designer PA**; execution is handed off to a fresh
**implementing PA** via the bridge prompt at
[`context_bridge_implementing_pa.md`](context_bridge_implementing_pa.md).

**Plan gate (unchanged):** per `.agents/rules/00-operating-model.md` and
`01-plan-before-execute.md`, the implementing PA MUST obtain the human's explicit
go **in its own session** and record it in the **Approval Log** below **before**
cutting the `hwconfig/wo1-scaffold-facade` branch or spawning any worker. The
bridge prompt is context, **not** approval (rule 01). A Tier-3 worker's
`WORKORDER.md` only becomes authorization once that approval is committed here.

---

## Architectural Shift

Move per-board hardware truth out of scattered `platformio.ini` `-D` flags into a
single, legible `hardware/<id>/board.h` per variant (the single source of
compile-time hardware truth), with `platformio.ini` reduced to a selector. Net
effects when complete:

- Zero display pins in `platformio.ini`; **no cross-file drift detection** needed.
- The framework's own U8G2/LVGL modules read their pin block **from `board.h`**
  (deletes the `-DDISPLAY_*_PIN` / `-DLCD_*` / `-DTOUCH_*` / `-DENCODER_*` flags
  and any consumer `static_assert` weld — power-monitor's #1 win).
- A facade + compile-time contract check ([HC-5](../../../../docs/hardware-configuration-design.md)) with a build-time echo (anti-silent-fallback).
- A `board.native.h` null board making the board interface total on host.
- Zero-edit downstream custom boards by path selection ([HC-11](../../../../docs/hardware-configuration-design.md)).

---

## Mandatory `board.h` documentation (enforced acceptance criteria)

Per [HC-1](../../../../docs/hardware-configuration-design.md) and explicit user
direction, **every** `board.h` this project authors (WO2, WO3, and the WO1
template) MUST include, and a WO is **not accepted without them**:

1. **ASCII-art pinout diagram** of the physical board at the top of the file —
   board outline, every pin labelled down both sides (GPIO # + signal + connector
   label), plus a legend. Match the **PowerMonitor `pins_esp32s3nano.h`** exemplar
   in style and completeness. *This is the primary hardware-wiring aid and the
   whole point of the design — it is not optional and not to be abbreviated.*
2. **Pin-priority / allocation notes** as structured comments — the
   contention-tier / "safest-first" allocation strategy (Tier 1 clean GP →
   Tier 2 sacrifices-I2C → Tier 3 strapping pins), recording *why* each pin was
   chosen and which to consume **last**.

**WO1 must ship a documented `board.h` authoring template** that includes both
elements as a filled skeleton, so WO2/WO3 produce them consistently. Reviewers
(and the human approval gate on each WO) check for these explicitly.

---

## Work Order Ledger (linear stack)

Each WO branches on the previous (no separate feature branch); the tip is the PR head.

| # | Work Order | Scope | Bases on | Complexity | Status |
|---|---|---|---|---|---|
| 1 | **[wo1-scaffold-facade](wo1-scaffold-facade/WORKORDER.md)** | `hardware/` tree; shared `board_pins.h` (`PIN_UNASSIGNED` + helpers); facade `board.h` (`#include BOARD_HEADER`); `board_contract.h` (conditional symbol checks + `#pragma message` echo); `board.native.h` null board; wire `native` env to the selector; **documented `board.h` authoring template with the mandatory ASCII pinout + pin-priority notes** (see criteria above). Proves the mechanism end-to-end on host. | `main` | standard | ✅ Complete `f5d459a` |
| 2 | **[wo2-u8g2-boards](wo2-u8g2-boards/WORKORDER.md)** | `hardware/esp32dev/board.h` + `hardware/esp32s3nano/board.h` (U8G2 pin blocks + `U8G2_DISPLAY_TYPE`); refactor `U8G2DisplayModule` to read `PIN_DISPLAY_*` from `board.h`; delete the 5 `-DDISPLAY_*_PIN` flags; co-locate `WITH_DISPLAY_U8G2` fragment + `partitions.csv`; wire both envs to the selector; U8G2 branch of the contract check. | wo1 | standard | ✅ Complete `456897f` |
| 3 | **[wo3-lvgl-board](wo3-lvgl-board/WORKORDER.md)** | `hardware/crowpanel-128-rotary/board.h` (LVGL block: LCD/touch/encoder/aux-I2C) **+ fix the RFC #50 gaps** (add power-enable pins GPIO 1/2; document `TOUCH_INT` 0→5 and `TOUCH_RST` −1→13 for HW verification; aux-I2C outside the display guard); refactor `CrowPanel128Adapter`/`LVGLDisplayModule` to read pins from `board.h`; delete the 13 `-DLCD/TOUCH/ENCODER_*` flags; co-locate `WITH_DISPLAY_LVGL` fragment + `partitions.csv`; LVGL branch of the contract check. | wo2 | standard | 📋 Planned |
| 4 | **[wo4-consumers-docs](wo4-consumers-docs/WORKORDER.md)** | Migrate `sample-app` + `template-app` `platformio.ini` to the `BOARD_HEADER` selector model; update `docs/developer/hardware-setup.md` + `using-optional-modules.md`; add the HC-11 zero-edit custom-board authoring guide with a worked example; final multi-target build + native test verification. | wo3 | standard | 📋 Planned |

**Branch prefix:** `hwconfig/wo{N}-{topic}`.

---

## Concurrency & Conflict Advisory

The WOs form a **linear stack** — WO2 and WO3 both edit `platformio.ini` and each
refactors a display module, so they **cannot** be parallelized without conflict.
Stacking serializes them cleanly; no parallel-conflict decision is required.

---

## Explicitly Out of Scope (this project)

- **OQ-5 — native U8G2 simulation / render core.** Departures-board contributes
  this as their own migration PR-13; it is *enabled by* WO1's null board + WO2's
  pin-agnostic module refactor, and fixes [#46](https://github.com/matthewmcneill/esp32-app-foundation/issues/46) — but it is **not** built here. Sequence it with the display PRs separately.
- **OQ-7 — formal per-pin risk/capability metadata** (power-monitor). Deferred,
  non-blocking; comments remain the convention for now.
- **Downstream consumer migrations** (power-monitor / departures-board /
  DraytonWiser moving their own repos onto `board.h`) — their work, enabled by HC-11.

---

## Approval Log

- **2026-07-29 — Design & prep authorised (designer PA session).** The human
  directed: *"make sure we have the work orders and project all prepped … produce a
  bridge prompt for another agent to take on the PA role and implement this work."*
  This authorises **preparation and handoff**, not autonomous branch-cutting by the
  designer PA. Design finalised (HC-1…HC-12, all OQs closed) and RFC #50 consumer
  review closed off.
- **2026-07-30 — Execution approved (implementing PA session `951cba1d`).** The human
  reviewed the full decomposition (4-WO linear stack, serialized, `flash` model workers)
  and directed: *"Approved to proceed - please operate autonomously to complete this
  work. When complete please stage a PR for review from the downstream consumers."*
  **Plan gate satisfied.** Workers may be spawned for WO1→WO4 sequentially.
