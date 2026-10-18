---
name: hwconfig-wo2
description: Author U8G2 board headers (esp32dev, esp32s3nano), refactor U8G2 module to read pins from board.h, delete -DDISPLAY_*_PIN flags
complexity: standard
---

# hwconfig / WO2 — U8G2 Boards + Module Pin Migration

**You are an expert senior software engineer and execution-focused worker agent.**
You are NOT the Principal Architect. Do not branch or plan. Execute the checklist.

## Objective
Make `hardware/esp32dev/board.h` and `hardware/esp32s3nano/board.h` the single
source of U8G2 display pins, refactor `U8G2DisplayModule` to read them, and delete
the `-DDISPLAY_*_PIN` flags — realising HC-6's must-fix (kill the pin duplication).

## Design Authority (read first)
- `docs/hardware-configuration-design.md` — HC-1 (mandatory docs), HC-2 (end-state:
  zero pins in `.ini`, no drift), HC-6 (**display modules read pins from board.h**),
  HC-4/HC-10 (co-located `partitions.csv`), §8 (power-monitor notes: 16-CS tiered map).
- Depends on WO1's facade/contract/`board_pins.h`/template (branch bases on WO1).

## Current State (verified)
- `platformio.ini` esp32dev: `DISPLAY_CS_PIN=26 DISPLAY_DC_PIN=5 DISPLAY_SPI_SCK=18
  DISPLAY_SPI_MISO=19 DISPLAY_SPI_MOSI=23`, `U8G2_DISPLAY_TYPE=U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI`.
- esp32s3nano: `CS=10 DC=9 SCK=48 MISO=47 MOSI=38`, same display type.
- Consumers today: `U8G2DisplayModule.cpp` lines ~60-63, 95-98, 105-107, 181-182;
  `U8G2DisplayModule.hpp` lines ~161-164 (`U8G2_DISPLAY_TYPE` default), ~578-587
  (constructor `_u8g2{U8G2_R0, DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN}`,
  `#ifdef DISPLAY_CS_PIN` guard, `DISPLAY_RST_PIN` default `U8X8_PIN_NONE`).

## Line-Item Instructions
- [x] `hardware/esp32dev/board.h` and `hardware/esp32s3nano/board.h` from the WO1
      template: `#define U8G2_DISPLAY_TYPE …`, and `constexpr int8_t PIN_DISPLAY_CS/DC/
      SCK/MISO/MOSI` (+ `PIN_DISPLAY_RST` = `PIN_UNASSIGNED` if none) inside
      `#ifdef WITH_DISPLAY_U8G2`. Values from Current State above.
- [x] Refactor `U8G2DisplayModule.cpp` + `.hpp` to `#include "board.h"` and read the
      `PIN_DISPLAY_*` `constexpr`s instead of the `-DDISPLAY_*_PIN` macros; replace the
      `#ifdef DISPLAY_CS_PIN` presence-guard with `#ifdef WITH_DISPLAY_U8G2`. Keep the
      write-only MISO=-1 and RST-optional behaviour (via `PIN_UNASSIGNED`/`U8X8_PIN_NONE`).
- [x] Delete the 5 `-DDISPLAY_*_PIN` flags and the `-DU8G2_DISPLAY_TYPE` flag from the
      esp32dev + esp32s3nano env blocks in root `platformio.ini`; add
      `-DBOARD_HEADER=\"hardware/<id>/board.h\"` to each. Keep `-DWITH_DISPLAY_U8G2`
      (co-locate it per OQ-2 if a per-board fragment mechanism is used; otherwise leave
      in-env with a comment that it is the board's hardware-sourced availability flag).
- [x] Move `partitions.esp32dev.csv` → `hardware/esp32dev/partitions.csv` and
      `partitions.esp32s3nano.csv` → `hardware/esp32s3nano/partitions.csv` (HC-10:
      one per board, duplicate not share); update `board_build.partitions` in the root
      esp32dev/esp32s3nano envs. (Consumer-app path refs are WO4.)
- [x] Ensure the WO1 contract check's U8G2 branch now passes for both boards.

## Mandatory Acceptance Criteria (HC-1) — both board.h files MUST include:
1. **ASCII-art pinout diagram** (board outline, all pins labelled, legend) — the
   SSD1322 4-wire SPI wiring is the key bring-up aid. Match the PowerMonitor exemplar.
2. **Pin-priority / allocation notes** (why each pin; which to consume last).

## Strict Boundary Box — do NOT
- Do NOT touch LVGL / crowpanel / `CrowPanel128Adapter` (WO3).
- Do NOT modify `sample-app`, `template-app`, or `src/` (WO4).
- Do NOT change the `native` env or WO1's shared files (except adding the U8G2 boards).
- Do NOT create branches, open PRs, or spawn sub-agents.

## Tooling Constraints
Use the **platformio MCP tools** (`pio-manager` skill), not raw `pio`. Build targets:
`native` (must stay green), `esp32dev`, `esp32s3nano` (both must compile).

## Verification Requirement
- `esp32dev` + `esp32s3nano` compile from board.h with **no `-DDISPLAY_*_PIN` flags
  present**; `native` green. Confirm the resolved-board `#pragma message` echo appears.
- **STOP and ask the user (via the PA) before distilling/committing.** After approval,
  `@distillery` this WO dir, append a status line, terminate.

## Findings, don't file
Surface any out-of-boundary defect to the PA in your report; do not file issues.

---

## Status Log
- **2026-07-30 — Completed & Approved.** Authored `hardware/esp32dev/board.h` and `hardware/esp32s3nano/board.h` with mandatory ASCII pinout diagrams and priority notes (HC-1). Refactored `U8G2DisplayModule` to consume `PIN_DISPLAY_*` constexprs from `board.h`. Removed `-DDISPLAY_*_PIN` and `-DU8G2_DISPLAY_TYPE` flags from `platformio.ini`. Co-located partitions to `hardware/<id>/partitions.csv` (HC-10). Verified 358/358 passing tests on `native`, and clean compilation on `esp32dev` and `esp32s3nano`. Approved by PA and `@distillery` invoked to package artifacts. Work Order 2 is complete.
