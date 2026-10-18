---
title: Work Order 1 — Framework Core Implementation (PR-52 InputModule)
distilled_at: 2026-07-31T09:24:00Z
original_plan_id: pr-52-input-module
artifacts:
  - context_artifacts/adr_framework_core.md
  - context_artifacts/graveyard_framework_core.md
  - context_artifacts/state_files_changed.md
---

# Context Bridge — Work Order 1: Framework Core

## Executive Summary
Work Order 1 is **COMPLETE, APPROVED, and VALIDATED**. Created `include/InputConfig.h` type definitions, `InputModule.hpp/cpp` implementation, updated `hardware/_common/board_contract.h` for `WITH_INPUT` validation, and added native build stubs (`iot_button.h` and `freertos/queue.h`). All 358 native unit tests pass green.

## Next Steps
Proceed to **Work Order 2 (`wo2-board-wiring`)**:
1. Add `BOARD_INPUTS[]` array descriptors to target board configurations (`hardware/esp32dev/board.h`, `hardware/esp32s3nano/board.h`, `hardware/native/board.h`).
2. Wire `InputModule` in `main.cpp` behind `#ifdef WITH_INPUT`.
3. Add `-D WITH_INPUT` build flags and `idf_component.yml` dependency for `espressif/button`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_framework_core.md` - Key design choices: member ring-buffer, static shim, two-phase ADC grouping, interfaces
- `context_artifacts/graveyard_framework_core.md` - Pitfalls avoided: native platform stubs, IMCPContributor method signature
- `context_artifacts/state_files_changed.md` - Complete ledger of files created, modified, and test verification results
