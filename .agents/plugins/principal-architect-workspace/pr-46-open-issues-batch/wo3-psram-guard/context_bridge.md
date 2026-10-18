---
title: "WO3 — PSRAM Configuration Guard Distillation"
distilled_at: "2026-07-22T19:36:00Z"
original_plan_id: "pr-46-open-issues-batch"
commit: "73b5b7f"
artifacts:
  - context_artifacts/adr_psram_guard.md
---

# WO3 — PSRAM Configuration Guard

## Executive Summary

Added a compile-time `#error` and a runtime `ESP_LOGE` guard to `Application.cpp` that catch PSRAM misconfiguration (Issue #36), removed the inert `-mfix-esp32-psram-cache-issue` S3 flag, and updated build-system docs. Committed as `73b5b7f` on branch `open-issues-batch/wo3-psram-guard`.

## What Changed

| File | Change |
|:---|:---|
| `platformio.ini` | Removed `-mfix-esp32-psram-cache-issue` from `[env:esp32s3nano]` |
| `modules/Application/Application.cpp` | Added `#error` guard (L51-58) + runtime SPIRAM size check (L95-103) |
| `.agents/skills/framework-expert/references/build-system.md` | New "PSRAM Configuration" section |

## Verification

- **native**: 423/423 tests passed — guard correctly inert (no `BOARD_HAS_PSRAM` defined)
- **esp32s3nano**: Firmware compiled successfully (17.1% RAM, 32.1% Flash) — guard correctly inert (both `BOARD_HAS_PSRAM` and `CONFIG_SPIRAM` defined)

## Next Steps

- WO4 (ArduinoJson docs) is the next and final work order in this batch.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_psram_guard.md` - Why the guard is structured as two layers (compile-time + runtime), placement rationale, and why no NATIVE_BUILD wrapping was needed
