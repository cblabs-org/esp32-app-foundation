---
name: hwconfig-wo1
description: Scaffold hardware/ tree, shared primitives, facade + contract check, native null board, and the board.h authoring template
complexity: standard
---

# hwconfig / WO1 — Scaffold, Facade, Contract, Null Board

**You are an expert senior software engineer and execution-focused worker agent.**
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

## Objective
Stand up the `board.h` mechanism end-to-end — the `hardware/` directory, shared
primitives, the selector facade, the compile-time contract check, and the
`native` "null board" — proving it compiles on host, **without yet migrating any
real board's pins** (that is WO2/WO3).

## Design Authority (read first)
- `docs/hardware-configuration-design.md` — HC-1 (mandatory ASCII pinout + priority
  notes), HC-2 (facade/selector, zero drift), HC-5 (facade + contract check + echo),
  HC-7 (null board), HC-9 (`PIN_UNASSIGNED`), HC-11 (path selection), OQ-3 (Option A).

## Line-Item Instructions
- [ ] Create `hardware/` at repo root and `hardware/_common/`.
- [ ] `hardware/_common/board_pins.h`: `constexpr int8_t PIN_UNASSIGNED = -1;` plus
      any shared helper macros. Board headers `#include "board_pins.h"`.
- [ ] Facade `hardware/board.h`: `#include BOARD_HEADER`, then
      `#include "board_contract.h"`, then a build-time echo
      (`#pragma message("board: using " ...)`) of the resolved header (HC-5).
- [ ] `hardware/_common/board_contract.h`: compile-time checks that are
      **conditional on the display flag** — `#ifdef WITH_DISPLAY_U8G2` → `static_assert`
      the U8G2 pin symbols exist; `#ifdef WITH_DISPLAY_LVGL` → the LVGL block; neither
      → nothing (headless). Emit **legible** errors (e.g. "WITH_DISPLAY_U8G2 set but
      PIN_DISPLAY_CS undefined"), not bare "undeclared identifier".
- [ ] `hardware/native/board.h`: the **null board** — every pin `PIN_UNASSIGNED`,
      no `WITH_DISPLAY_*` flag, headless by construction (HC-7). Include the ASCII/notes
      header per the template below (a "no physical wiring — host build" note is fine).
- [ ] **`hardware/_TEMPLATE.board.h`** (or a `hardware/README.md` template): a
      documented `board.h` authoring skeleton that WO2/WO3 copy, containing BOTH
      mandatory documentation elements as a filled example (see Acceptance Criteria).
- [ ] Wire the `native` env in `platformio.ini` to the selector:
      `-DBOARD_HEADER=\"hardware/native/board.h\"` and ensure `hardware/` is on the
      include path (`-I hardware/_common` or equivalent). Do **not** touch other envs.
- [ ] Add a tiny native compile/unit check that `#include`s `hardware/board.h` and
      asserts `PIN_UNASSIGNED == -1` (proves facade + null board + contract compile on host).

## Mandatory Acceptance Criteria — board.h documentation (HC-1)
The template AND `hardware/native/board.h` MUST include, or this WO is **rejected**:
1. **ASCII-art pinout diagram** header block (board outline, pins labelled both
   sides: GPIO# + signal + connector label, plus legend) — match the PowerMonitor
   `pins_esp32s3nano.h` exemplar in style. (For the null native board, a clearly
   labelled "host build — no physical pinout" placeholder block that still shows the
   intended structure is acceptable.)
2. **Pin-priority / allocation notes** as structured comments (contention tiers:
   clean → sacrifices-I2C → strapping; which pins to consume last).

## Strict Boundary Box — do NOT
- Do NOT migrate esp32dev / esp32s3nano / crowpanel pins (WO2/WO3).
- Do NOT modify `U8G2DisplayModule`, `LVGLDisplayModule`, `CrowPanel128Adapter`, or
  any consumer app (`sample-app`, `template-app`, `src/`).
- Do NOT change any env other than `native`.
- Do NOT create branches, open PRs, or spawn sub-agents.

## Tooling Constraints
Per `.agents/rules/04-mcp-tooling.md`, use the **platformio MCP tools** (via the
`pio-manager` skill) for all builds — never raw `pio` CLI. Verify: `native` builds
green and the new native check passes. (No hardware envs are affected by this WO.)

## Verification Requirement
- `native` build + tests green; the `board.h`-include check compiles and passes.
- **Before distilling or committing, STOP and ask the user (via the PA) if they are
  happy.** Only after approval, invoke `@distillery` targeting this WO directory,
  then append a status line to this WORKORDER and terminate.

## Findings, don't file
If you discover an out-of-boundary defect or upstream concern, **surface it to the
PA in your completion report** — do NOT file issues yourself (PA/Chief authority).
