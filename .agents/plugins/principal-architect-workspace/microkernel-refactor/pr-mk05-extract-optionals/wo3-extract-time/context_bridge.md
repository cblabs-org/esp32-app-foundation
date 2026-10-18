---
title: Work Order 3 — Extract Time (TimeManager & TimeConfigGate)
distilled_at: 2026-07-29T12:48:20+01:00
original_plan_id: pr-mk05-extract-optionals
artifacts:
  - context_artifacts/adr_extract_time.md
  - context_artifacts/graveyard_extract_time.md
---

# Work Order 3 — Extract Time (TimeManager & TimeConfigGate)

## Executive Summary

Work Order 3 is 100% complete and verified. `TimeManager` and `TimeConfigGate` have been extracted from `Application`'s owned members and are now explicitly composed in `main.cpp` behind the `#ifdef WITH_TIME` flag (default ON). All 449 native unit tests passed, and hardware builds (`esp32dev` and `esp32s3nano`) succeeded across the flag matrix, saving ~12.8 KiB flash when disabled.

## Next Steps

Proceed to **WO4 — App Templates, Tests & Size Check** in `pr-mk05-extract-optionals` to finalize template apps, update test coverage, and record final binary size deltas.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `./context_artifacts/adr_extract_time.md` - Structural design decisions, API changes, and flash impact for TimeManager extraction.
- `./context_artifacts/graveyard_extract_time.md` - Resolved test failures and fail-fast sentinel handling.
