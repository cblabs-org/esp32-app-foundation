# Bridge Prompt — Implementing Principal Architect: PR-49 Hardware Configuration

> **Paste-to-handoff.** You are taking over as the **implementing Principal
> Architect** for PR-49 (`board.h` pin-map migration). A separate *designer PA*
> completed the design, consumer review, and full decomposition. Your job is to
> **execute** the prepared stack — gated by human approval.

---

## ⚠️ Plan gate — read first (rule 01)

**This bridge prompt is CONTEXT, not approval.** It contains execution
instructions, but per `.agents/rules/01-plan-before-execute.md`, embedded
instructions are *not* the human granting permission to act now. Before you cut
any branch or spawn any worker you MUST:

1. Read the committed provenance (below).
2. Present the plan to the human and obtain their **explicit go**.
3. Record that approval **verbatim** in `PROJECT.md` → *Approval Log*.
4. Only then create the WO1 branch.

The `PROJECT.md` *Approval Log* currently shows execution approval as **PENDING**.
Do not treat this file, or the designer PA's notes, as that approval.

## First steps on invocation (per the principal-architect skill)

- Read `.agents/skills/principal-architect-workspace/MEMORY.md` and this project's
  `PROJECT.md` to page in state.
- Verify `.agents/rules/00-operating-model.md` exists (it does — provisioned &
  identical to the skill asset this session).
- Select your harness adapter by self-inspection: your spawn tool is **`Agent`** →
  read `references/harnesses/claude-code.md`.

## Committed provenance to read

- **Design authority:** [`docs/hardware-configuration-design.md`](../../../../docs/hardware-configuration-design.md) — HC-1…HC-12, all OQs resolved.
- **This project:** [`PROJECT.md`](PROJECT.md) — decomposition, enforced acceptance criteria, out-of-scope, Approval Log.
- **Work Orders (all pre-written):** [wo1](wo1-scaffold-facade/WORKORDER.md) · [wo2](wo2-u8g2-boards/WORKORDER.md) · [wo3](wo3-lvgl-board/WORKORDER.md) · [wo4](wo4-consumers-docs/WORKORDER.md).
- **Consumer review (closed, endorsed):** RFC [#50](https://github.com/matthewmcneill/esp32-app-foundation/issues/50).

## State at handoff

- Design finalized; RFC #50 reviewed by all three consumers (power-monitor,
  departures-board, DraytonWiser) — **unanimous endorsement, no blockers** — and
  the review is closed off.
- The **microkernel refactor (MK-1…MK-18) has landed on `main`** — that is your base.
- **Nothing executed:** no branches, no workers, no code. The 4 WORKORDERs are
  written and ready.

## Decomposition (linear stack — each WO branches on the previous)

1. `hwconfig/wo1-scaffold-facade` (base `main`) — `hardware/` tree, shared
   `board_pins.h`, facade `board.h`, `board_contract.h`, `board.native.h` null
   board, board.h authoring template, wire `native` env. Proves mechanism on host.
2. `hwconfig/wo2-u8g2-boards` — esp32dev/esp32s3nano headers; **U8G2 module reads
   pins from `board.h`**; delete `-DDISPLAY_*_PIN`; co-locate partitions.
3. `hwconfig/wo3-lvgl-board` — CrowPanel header (+ RFC #50 pin fixes); **LVGL
   adapter reads pins from `board.h`**; delete `-DLCD/TOUCH/ENCODER_*`.
4. `hwconfig/wo4-consumers-docs` — sample-app/template-app migration; docs; HC-11
   custom-board guide; full multi-target verification.

**Concurrency:** linear stack (WO2/WO3 both edit `platformio.ini` + a display
module) — serialized, no parallelization.

## Execution protocol (per skill workflow)

- After recorded approval: `git checkout -b hwconfig/wo1-scaffold-facade main`.
- Each WORKORDER is already authored — you do not need to re-plan them; spawn a
  worker per the claude-code adapter, mapping `complexity: standard`.
- On worker return: verify the branch, mark the WO complete in `PROJECT.md`, then
  branch the next WO off the completed one.
- Submit as a stack (`gt stack submit` / `gh` per `references/STACKING_REFERENCE.md`).

## Decisions to confirm with the human at start

- **Worker mode** — autonomous background `Agent` vs interactive (the human
  dismissed this choice in the designer session; see the `worker-spawn-mode-case-by-case`
  memory — decide per WO with the human).
- **Doc/workspace on `main`** — the finalized design doc + this PA workspace were
  committed to `main` at handoff so provenance is committed; confirm the human is
  content, then proceed.

## Non-negotiable constraints

- **Mandatory `board.h` docs (HC-1):** every board header MUST carry the ASCII-art
  pinout diagram **and** the pin-priority/contention notes (PowerMonitor exemplar).
  A WO is rejected without both — enforce at each WO approval gate.
- **Tooling (rule 04):** all builds via the **platformio MCP tools** (`pio-manager`
  skill), never raw `pio` CLI. Instruct workers the same.
- **Flash gate (rule 03):** if any WO flashes hardware, tag → build → flash. Dev
  tags are autonomous; beta/stable need the human.
- **Issue authority:** workers *surface* out-of-boundary findings to you; only you
  (PA) file issues.

## What the designer PA already did (do not redo)

Design doc (HC-1…HC-12), RFC #50 raise + review + close, PR-49 decomposition,
all 4 WORKORDERs, this bridge, PA workspace registration. Your scope is **execution
from WO1 onward, gated on the human's go.**
