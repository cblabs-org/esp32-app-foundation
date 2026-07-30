---
title: hwconfig / WO2 - U8G2 Boards + Module Pin Migration
distilled_at: 2026-07-30T08:32:00Z
original_plan_id: pr-51-hardware-config
artifacts:
  - context_artifacts/adr_u8g2_board_migration.md
  - context_artifacts/graveyard_psram_macro_collision.md
---

# Context Bridge: hwconfig / WO2 — U8G2 Boards + Module Pin Migration

## Executive Summary
WO2 has successfully migrated all U8G2 display pin configuration from `platformio.ini` `-D` build flags into target board headers (`hardware/esp32dev/board.h` and `hardware/esp32s3nano/board.h`). `U8G2DisplayModule` now consumes `PIN_DISPLAY_*` `constexpr` values directly from `board.h`. Partition tables were co-located under `hardware/<id>/partitions.csv` (HC-10), and all build flags for U8G2 display pins were deleted from `platformio.ini`. All 358 native test cases pass, and both `esp32dev` and `esp32s3nano` compile cleanly.

## Next Steps
Proceed to **WO3 (`wo3-lvgl-board`)**:
1. Author `hardware/crowpanel-128-rotary/board.h` with LVGL LCD, touch, encoder, and aux-I2C pin definitions + RFC #50 pin fixes.
2. Refactor `CrowPanel128Adapter` / `LVGLDisplayModule` to read pins from `board.h`.
3. Delete the 13 `-DLCD/TOUCH/ENCODER_*` flags from `platformio.ini`.
4. Co-locate `hardware/crowpanel-128-rotary/partitions.csv`.
5. Enable and verify the LVGL branch of `board_contract.h`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_u8g2_board_migration.md` - Architecture decision record for U8G2 board headers, module pin migration, facade inclusion, and co-located partition layout.
- `context_artifacts/graveyard_psram_macro_collision.md` - Technical post-mortem of the `-DBOARD_HAS_PSRAM` macro collision with `constexpr bool BOARD_HAS_PSRAM` and its resolution.
