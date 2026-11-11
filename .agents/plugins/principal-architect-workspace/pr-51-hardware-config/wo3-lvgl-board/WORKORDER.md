---
name: hwconfig-wo3
description: Author CrowPanel LVGL board header (with RFC #50 pin fixes), refactor CrowPanel128Adapter to read pins from board.h, delete -DLCD/TOUCH/ENCODER flags
complexity: standard
---

# hwconfig / WO3 — LVGL Board (CrowPanel) + Adapter Pin Migration

**You are an expert senior software engineer and execution-focused worker agent.**
You are NOT the Principal Architect. Do not branch or plan. Execute the checklist.

## Objective
Make `hardware/crowpanel-128-rotary/board.h` the single source of the CrowPanel's
LVGL pin surface, refactor `CrowPanel128Adapter` to read it, delete the
`-DLCD/TOUCH/ENCODER_*` flags, and **capture the RFC #50 pin gaps** in the header.

## Design Authority (read first)
- `docs/hardware-configuration-design.md` — HC-6 (LVGL pin block; modules read from
  board.h), HC-12 (aux-I2C outside display guard; app-peripherals), §8 (DraytonWiser
  consumer notes — the authoritative pin table).
- Depends on WO2 (branch bases on WO2).

## Current State (verified)
- `platformio.ini` crowpanel-128-rotary: `WITH_DISPLAY_LVGL`; `LCD_SPI_SCLK=10
  LCD_SPI_MOSI=11 LCD_CS=9 LCD_DC=3 LCD_RST=14 LCD_BL=46`; `TOUCH_SDA=6 TOUCH_SCL=7
  TOUCH_INT=0 TOUCH_RST=-1`; `ENCODER_A=45 ENCODER_B=42 ENCODER_SW=41`;
  `build_unflags = -DWITH_DISPLAY_U8G2`.
- Consumer: `adapters/lvgl/CrowPanel128Adapter.cpp` — LCD (lines ~17,18,20,26,27,45),
  TOUCH (~58,63,64), ENCODER (~125,129,152).

## Line-Item Instructions
- [x] `hardware/crowpanel-128-rotary/board.h` from the WO1 template, inside
      `#ifdef WITH_DISPLAY_LVGL`: `constexpr` `PIN_LCD_SPI_SCLK/MOSI, PIN_LCD_CS/DC/RST/BL`,
      `PIN_TOUCH_SDA/SCL/INT/RST`, `PIN_ENCODER_A/B/SW` (values above).
- [x] **Capture the RFC #50 fixes in board.h (documentation-first):**
      - Add `PIN_LCD_PWR_EN1 = 1` and `PIN_LCD_PWR_EN2 = 2` with a **prominent comment**:
        "must be driven HIGH at init — display will not light otherwise" (currently
        absent from the build entirely).
      - `PIN_TOUCH_INT`: keep current `0` but add a comment flagging MakerGuides verified
        `5` — **needs hardware verification**. Do not silently change the wired value.
      - `PIN_TOUCH_RST`: keep `PIN_UNASSIGNED` (was `-1`) with a comment that MakerGuides
        resets CST816D via GPIO 13; note LovyanGFX may handle internally — needs verify.
      - Place `PIN_AUX_I2C_SDA = 38` / `PIN_AUX_I2C_SCL = 39` **outside** the
        `#ifdef WITH_DISPLAY_LVGL` guard (board capability, not a display fact — HC-12).
- [x] Refactor `CrowPanel128Adapter.cpp` (+ header if needed) to `#include "board.h"` and
      read the `PIN_*` `constexpr`s instead of the `-D` macros. Confirm LovyanGFX /
      ESP32Encoder accept the values programmatically (DraytonWiser confirmed they do).
- [x] Delete the 13 `-DLCD/TOUCH/ENCODER_*` flags from the crowpanel env; add
      `-DBOARD_HEADER=\"hardware/crowpanel-128-rotary/board.h\"`. Keep
      `WITH_DISPLAY_LVGL` + the `build_unflags` U8G2 removal.
- [x] `hardware/crowpanel-128-rotary/partitions.csv`: own copy (HC-10 — currently shares
      `partitions.esp32s3nano.csv`); update `board_build.partitions` for this env.
- [x] Contract check's LVGL branch passes for this board.

## Mandatory Acceptance Criteria (HC-1) — board.h MUST include:
1. **ASCII-art pinout diagram** of the CrowPanel (LCD/touch/encoder/aux-I2C buses,
   power-enable pins, legend). This board's 3-bus surface is the strongest case for it.
2. **Pin-priority / bus-grouping notes.**
*Rejected without both.*

## Strict Boundary Box — do NOT
- Do NOT change the U8G2 boards/module (WO2) or the shared WO1 files.
- Do NOT modify `sample-app`, `template-app`, `src/` (WO4).
- Do NOT wire the new power-enable pins into adapter init as a blind functional change
  — capture them in board.h and, if you add init wiring, gate it clearly and note it
  **requires hardware verification** (surface to PA). This WO is a pin-map migration,
  not a display-bring-up fix.
- Do NOT create branches, open PRs, or spawn sub-agents.

## Tooling Constraints
Use **platformio MCP tools** (`pio-manager` skill). Build targets: `native` (green),
`crowpanel-128-rotary` (must compile). NOTE: crowpanel is excluded from `default_envs`
— build it explicitly by name.

## Verification Requirement
- `crowpanel-128-rotary` compiles from board.h with **no `-DLCD/TOUCH/ENCODER_*` flags**;
  `native` green. Resolved-board echo appears.
- **STOP and ask the user (via the PA) before distilling/committing.** After approval,
  `@distillery` this WO dir, append status, terminate.

## Findings, don't file
Surface out-of-boundary findings (esp. the touch-pin hardware-verification items) to
the PA in your report; do not file issues yourself.

---

## Status Update

- **2026-07-30 — Work Order 3 Complete & Approved.**
  - Authored `hardware/crowpanel-128-rotary/board.h` with HC-1 ASCII-art pinout diagram and pin-priority allocation notes.
  - Positioned `PIN_AUX_I2C_SDA` and `PIN_AUX_I2C_SCL` outside display guard (HC-12).
  - Captured RFC #50 power-enable pins (`PIN_LCD_PWR_EN1 = 1`, `PIN_LCD_PWR_EN2 = 2`) with prominent comments.
  - Documented touch pin hardware verification notes (`PIN_TOUCH_INT = 0` vs `5`, `PIN_TOUCH_RST = -1` vs `13`).
  - Refactored `CrowPanel128Adapter.cpp` to read pins from `board.h`.
  - Deleted 13 `-DLCD/TOUCH/ENCODER_*` build flags from `platformio.ini` and set `-DBOARD_HEADER`.
  - Co-located `hardware/crowpanel-128-rotary/partitions.csv` (HC-10).
  - `crowpanel-128-rotary` compiles green (`SUCCESS`). All 358 native unit tests pass.
  - Committed at `d045511`. Distilled to `.agents/plugins/principal-architect-workspace/pr-51-hardware-config/wo3-lvgl-board/`.

