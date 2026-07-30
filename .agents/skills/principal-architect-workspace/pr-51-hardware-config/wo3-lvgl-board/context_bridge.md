---
title: HW Config WO3 — LVGL Board (CrowPanel) + Adapter Pin Migration
distilled_at: 2026-07-30T08:39:00Z
original_plan_id: pr-51-hardware-config
artifacts:
  - context_artifacts/adr_crowpanel_board_migration.md
  - context_artifacts/graveyard_build_issues.md
---

# Executive Summary

Work Order 3 (`hwconfig / WO3`) is complete and verified. Authored `hardware/crowpanel-128-rotary/board.h` with HC-1 ASCII pinout diagram and pin priority allocation notes. Placed auxiliary I2C bus outside display guard (HC-12), captured RFC #50 power-enable pins (GPIO 1/2) and touch pin comments (GPIO 5/13). Refactored `CrowPanel128Adapter.cpp` to read `board.h` pin constants and eliminated all 13 `-DLCD/TOUCH/ENCODER_*` flags from `platformio.ini`. `crowpanel-128-rotary` compiles green (`SUCCESS`), and all 358 native unit tests pass.

# Next Steps

Proceed to Work Order 4 (`wo4-consumers-docs`):
1. Migrate `sample-app/platformio.ini` and `template-app/platformio.ini` to use the `BOARD_HEADER` selector model.
2. Update documentation (`docs/developer/hardware-setup.md`, `using-optional-modules.md`).
3. Author HC-11 custom-board authoring guide.
4. Execute final full-stack multi-target verification across all environments.

# Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_crowpanel_board_migration.md` - Architecture decision record for CrowPanel 1.28 board header migration and pin mappings
- `context_artifacts/graveyard_build_issues.md` - Build resolution for displayModule MCP wiring scope in main.cpp
