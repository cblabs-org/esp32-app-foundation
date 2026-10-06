# PROJECT — U8G2GL PR-05: the integration module and the first migrated board

**Tier 2 · Principal Architect**
**Programme**: U8G2GL (governed from `cblabs-org/u8g2gl`; this repository is its first consumer)
**Twin project**: `cblabs-org/u8g2gl` →
`.agents/skills/principal-architect-workspace/pr-05-integration/PROJECT.md` (packaging, the
programme view, the full scoping record). This file is the **ledger of record for the foundation
work orders** and their provenance.
**Source**: U8G2GL Chief Architect bridge "PR-05: foundation integration and the first real board"
(pasted by the owner, 2026-10-06)
<!-- Last updated: 2026-10-06 -->

Not to be confused with this workspace's `pr-05-display-module`, which is foundation's own PR-05
from 2026. The `u8g2gl-` prefix names the programme.

## Mandate

Per U8G2GL `docs/implementation_plan.md` §3 and PD-4, following the `lvglpro` precedent:

- **`modules/U8G2GLDisplay/`**, Apache-2.0. It consumes the MIT `u8g2gl` runtime as an external
  PlatformIO dependency, pinned by git commit. Nothing is published.
- **`uidesign/ssd1322-256x64/u8g2gl/`**: the XML project and its committed `generated/`, consumed
  through `lib_extra_dirs`. It is named for the panel because `esp32dev` and `esp32s3nano` share
  it.
- A **pre-build hook** that runs `u8g2gl generate` from a local CLI checkout when one is
  configured, and otherwise builds from the committed `generated/` (U8G2GL [D-11]).
- A **`THIRD-PARTY-NOTICES.md`** row for U8G2GL (MIT).
- **`DiagnosticsBoard` migrated** in the root firmware (`src/main.cpp`). `sample-app` and
  departures-board-v3 keep the legacy class.
- **PR-07's "before" evidence**, captured while the legacy board is live: pixels, flash and RAM on
  `esp32dev`, line counts, and edit-loop timing.

**PD-4:** when this lands, `modules/U8G2Display` is **frozen**: correctness and security fixes only.
The PR description announces it. **Nothing is deleted.**

**Out of scope:** other boards (PR-10), the Wasm preview and pixel-diff harness (PR-06),
publishing or tagging U8G2GL, any change to the U8G2GL generator or runtime (report, don't fix),
and web diagnostics (R7).

## Human Approval

> "Please proceed with the development - I will work with you to connect to the platformio mcp
> (which is already on the machine) and also attache a device later when back at the bench.  For
> now get on with the dev - thanks."
>
> — the owner, 2026-10-06, in reply to the PA's scoping report and proposed decomposition
> (U8G2GL WO-A; foundation WO-B1..B4, plus B5 when hardware is attached) with rulings R1–R7.

## Rulings that bind foundation work

- **R1 — board host, not display owner.** In PR-05, `U8G2GLDisplay` provides an `IDisplayBoard`
  that runs a generated U8G2GL screen inside the **legacy** carousel. It calls the screen's
  `tick_fn` / `render_fn` directly (`u8g2gl_screen` in the runtime's `u8g2gl_core.h`) and never
  calls `ui_handler()`, which clears and sends the buffer itself. The legacy module already does
  both (`U8G2Display.cpp:1413`). The display, carousel and screensaver stay in
  `modules/U8G2Display` until PR-10.
- **R4 — PlatformIO through the MCP only.** Foundation `rules/04` stands. Until the owner connects
  the PlatformIO MCP, any step needing a PlatformIO build is **deferred and reported**, never run
  through the `pio` CLI. Host tools that are not PlatformIO (CMake, a C++ compiler, Python, Node)
  are permitted.
- **R5 — hardware later.** On-device checks wait for WO-B5 (interactive).
- **R6 — pin.** `lib_deps` pins the pushed U8G2GL WO-A commit. Before this PR merges, it is
  re-pinned to the U8G2GL merge commit on `main`, which merges first.
- **R7 — no new web diagnostics.** The legacy Screenshot tab captures the new board.

## Known hazards (for WO-B2)

- **Inherited U8G2 state.** The legacy module sets `setFontPosTop()`, `setFontMode(1)` and
  `setDrawColor(1)` once at begin (`U8G2Display.cpp:141-146`). A U8G2GL screen rendering inside
  the legacy carousel inherits that state. Whatever the U8G2GL runtime assumes must be
  established by the host board, **measured**, not assumed.
- **The "·" separator.** The legacy fonts are `_tr` and have no glyph `0xB7`, so the separator
  probably never renders. Pixel identity means reproducing what legacy actually draws. WO-B1's
  baseline settles it.
- **Live inputs.** The legacy `tick()` reads WiFi, `millis()` and the heap directly. The migrated
  board needs the same values through an injectable path, so the harness can render it with fixed
  inputs.

## Branch Strategy

- **Worktree:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`, from
  `origin/main` @ `e5342373`. The main checkout belongs to other work and to the bug-fixer
  daemon; never touch it.
- **Branches:** `u8g2gl-integration/wo<N>-<topic>`, stacked serially, each based on the previous
  WO's branch by name, with no upstream until submission.
- **The PR:** opens from the stack tip against `main` and is pushed only at submission.

## Work Order Ledger

- **[ ] [wo1-baseline](wo1-baseline/WORKORDER.md):** the "before" evidence.
  - **Part 1** (host pixel baseline, line counts): *dispatched*.
  - **Part 2** (flash/RAM, native suite count, legacy edit-loop timing): **pending the
    PlatformIO MCP**.
- **[ ] wo2-module:** the board host, the generated UI, the pinned dependency, the hook, the
  notices row, and the `main.cpp` switch. *Needs WO-A's pushed commit and the MCP.*
- **[ ] wo3-evidence:** the "after" measurements through the same harness, `EVIDENCE.md`, and the
  PR description.
- **[ ] wo4-readability:** a house-style pass (`.agents/skills/house-style-docs`).
- **[ ] wo5-device (interactive):** host render vs on-device capture, when the owner is at the
  bench.

## Evidence

All four of PR-07's evidence sets live in [evidence/EVIDENCE.md](evidence/EVIDENCE.md). PR-07's PA
reads them there and does not re-derive them.

## Session Log

- **2026-10-06 — dispatch.** Worktree created at `e5342373`. The `house-style-docs` submodule
  initialised in it. WO-B1 Part 1 dispatched, in parallel with U8G2GL WO-A.
