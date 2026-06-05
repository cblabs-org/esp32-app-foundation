---
title: WO2 — Auto-Rollback Wiring + Fail-Closed OTA Fix
distilled_at: 2026-06-05T08:42:00Z
original_plan_id: pr-14-ota-production
artifacts:
  - context_artifacts/adr_fail_closed_ota.md
  - context_artifacts/adr_twdt_lifecycle.md
  - context_artifacts/state_modified_files.md
---

# WO2 Context Bridge

## Executive Summary

Work Order 2 is **COMPLETE**. All three objectives are implemented and verified: `markAppValid()` is wired into the boot sequence as the self-test barrier (Step 17b), the fail-open OTA signature vulnerability is closed at three independent checkpoints, and the Task Watchdog is subscribed/fed/unsubscribed correctly across the OTA task lifecycle with `CONFIG_ESP_TASK_WDT_PANIC=y` enabled.

## Verification Results

- **Native tests**: 19/19 suites PASSED (including new `test_ota_fail_closed`)
- **esp32dev build**: SUCCESS (81.40s)
- **esp32s3nano build**: SUCCESS (70.54s)
- **Regression greps**: "security degraded" and "Skipping signature verification" confirmed absent

## Next Steps

- No further action required for WO2
- PA should proceed to the next work order in the PR-14 queue
- The `SECURITY_VERSION` macro is set to `1` — future anti-rollback bumps should increment this value in `platformio.ini`

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_fail_closed_ota.md` - Decision record for the three-layer fail-closed signature verification model
- `context_artifacts/adr_twdt_lifecycle.md` - Decision record for TWDT subscribe/feed/unsubscribe lifecycle in the OTA task
- `context_artifacts/state_modified_files.md` - Complete list of modified/created files with architectural notes
