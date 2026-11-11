---
name: hwconfig-wo4
description: Migrate sample-app + template-app to the BOARD_HEADER selector, update docs, add HC-11 custom-board authoring guide, final multi-target verification
complexity: standard
---

# hwconfig / WO4 — Consumer Apps, Docs, Custom-Board Guide, Final Verification

**You are an expert senior software engineer and execution-focused worker agent.**
You are NOT the Principal Architect. Do not branch or plan. Execute the checklist.

## Objective
Migrate the two consumer apps to the `board.h` selector model, document the new
system (including the HC-11 zero-edit custom-board path), and run final
multi-target verification for the whole PR-51 stack.

## Design Authority (read first)
- `docs/hardware-configuration-design.md` — HC-11 (zero-edit downstream custom boards),
  HC-4 (`hardware/<id>/` layout), §8 (consumer implementation notes).
- Depends on WO3 (branch bases on WO3).

## Current State (verified)
- `sample-app/platformio.ini`: esp32dev (`DISPLAY_CS_PIN=26`, SPI 18/19/23,
  `U8G2_DISPLAY_TYPE`, `../partitions.esp32dev.csv`), esp32s3nano (`CS=10`, SPI 48/47/38,
  `../partitions.esp32s3nano.csv`), native.
- `template-app/platformio.ini`: mirror structure (esp32dev/esp32s3nano/native).
- Both reference framework partitions by relative path (moved by WO2/WO3).

## Line-Item Instructions
- [x] `sample-app/platformio.ini`: delete the per-env `-DDISPLAY_*_PIN` +
      `-DU8G2_DISPLAY_TYPE` blocks; add `-DBOARD_HEADER=\"…/hardware/<id>/board.h\"` and
      point `board_build.partitions` at the moved `hardware/<id>/partitions.csv`. Keep
      `WITH_DISPLAY_U8G2` and `BOARD_ID`. Ensure `hardware/_common` is on the app `-I`.
- [x] `template-app/platformio.ini`: same migration.
- [x] Confirm `src/main.cpp` (framework template main) needs no pin changes (it uses
      `WITH_DISPLAY_*` composition only) — verify, don't edit unless broken.
- [x] `docs/developer/hardware-setup.md`: replace the "PlatformIO Environment Reference"
      `-DDISPLAY_*_PIN` section with the `board.h` model; keep the wiring tables (now
      cross-referenced to each `hardware/<id>/board.h` ASCII pinout).
- [x] `docs/developer/using-optional-modules.md`: update display-pin references to the
      `board.h` mechanism.
- [x] **New: HC-11 custom-board authoring guide** (a section in hardware-setup.md or a
      new `docs/developer/custom-boards.md`): a worked example of a downstream repo
      adding `hardware/<myboard>/board.h`, setting `-DBOARD_HEADER`, with **zero framework
      edits** — including the mandatory ASCII pinout + priority notes requirement, and the
      PlatformIO board-JSON boundary note.
- [x] Sanity-scan the `framework-expert` skill for board/pin authoring references that
      now need updating; surface a list to the PA (update only if trivially in-scope).

## Strict Boundary Box — do NOT
- Do NOT re-migrate framework board headers or display modules (WO2/WO3 own them).
- Do NOT change framework module code.
- Do NOT create branches, open PRs, or spawn sub-agents.

## Tooling Constraints
Use **platformio MCP tools** (`pio-manager` skill). **Final full-stack verification:**
build `native` (+ full GoogleTest suite green), `esp32dev`, `esp32s3nano`,
`crowpanel-128-rotary`, and the `sample-app` + `template-app` esp32dev/esp32s3nano
targets. All must compile; native tests must pass.

## Verification Requirement
- Whole stack builds across all targets; docs render; the custom-board guide is complete.
- **STOP and ask the user (via the PA) before distilling/committing.** After approval,
  `@distillery` this WO dir, append status, terminate.

## Findings, don't file
Surface out-of-boundary findings to the PA; do not file issues yourself.

---

## Status Log

- **2026-07-30 — Work Order 4 Executed & Approved.**
  - `sample-app/platformio.ini` and `template-app/platformio.ini` migrated to `-DBOARD_HEADER=\".../hardware/<id>/board.h\"` and `hardware/<id>/partitions.csv`.
  - Created [`docs/developer/custom-boards.md`](../../../../docs/developer/custom-boards.md) for zero-edit custom-board authoring (HC-11). Updated developer guides (`hardware-setup.md`, `using-optional-modules.md`, `README.md`).
  - Full-stack build and test verification passed across all 8 environments (root native 358 tests green, root esp32dev/esp32s3nano/crowpanel-128-rotary, sample-app esp32dev/esp32s3nano, template-app esp32dev/esp32s3nano).
  - Explicit approval received; distilled artifacts created at `.agents/plugins/principal-architect-workspace/pr-51-hardware-config/wo4-consumers-docs/`. WO4 complete.

