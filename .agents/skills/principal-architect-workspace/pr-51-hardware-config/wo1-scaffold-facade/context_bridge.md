---
title: WO1 — Scaffold, Facade, Contract, Null Board Distillation
distilled_at: 2026-07-30T08:24:00Z
original_plan_id: pr-51-hardware-config/wo1-scaffold-facade
artifacts:
  - context_artifacts/adr_scaffold_facade.md
  - context_artifacts/graveyard_build_issues.md
---

# Executive Summary
WO1 successfully scaffolded the hardware header tree (`hardware/`, `hardware/_common/`, `hardware/native/`), implemented the top-level `board.h` facade with contract checking (`board_contract.h`), defined the null board, prepped `_TEMPLATE.board.h` with HC-1 documentation, and wired `env:native` in `platformio.ini`. All 358 unit test cases pass.

# Next Steps
1. Hand off to WO2 (`wo2-u8g2-boards`): Migrate `esp32dev` and `esp32s3nano` pin definitions into their respective `hardware/<board_id>/board.h` files and refactor `U8G2DisplayModule`.

# Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_scaffold_facade.md` - Architecture decision record for facade, contract, null board, and shared primitives
- `context_artifacts/graveyard_build_issues.md` - Troubleshooting notes on PlatformIO native lib_ignore rules and GoogleTest main symbols
