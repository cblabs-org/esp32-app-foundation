---
title: "WO1: Display Lifecycle Fixes — Screensaver, Modal Cleanup, Sleep Diagnostics"
distilled_at: "2026-05-31T22:04:00Z"
original_plan_id: pr-13-test-app
artifacts:
  - context_artifacts/adr_sleep_screensaver_modal.md
  - context_artifacts/graveyard_overrideboard_mapping.md
  - context_artifacts/state_verification_results.md
---

## Executive Summary

WO1 fixed three display lifecycle bugs in `U8g2DisplayModule`: stale modal cleanup during empty-pool carousel advance, replacing OLED hardware power-off with screensaver-as-modal activation for burn-in prevention, and adding a diagnostic log for the sleep timer. All changes compile cleanly on `esp32dev`; the 2 pre-existing test failures in `test_carousel.cpp` are unrelated to WO1.

## Next Steps

1. **Fix pre-existing test failures**: `ShowBoard_NonPoolBoard_SetsOverride` and `ShowBoard_BackToPoolBoard_ClearsOverride` in `test/test_unit/test_u8g2Display/test_carousel.cpp` — these test an `_overrideBoard` concept that was removed during PR-12's Base+Modal refactor. Tests need updating to match the current `_modal` architecture.
2. **On-device verification**: Flash `esp32dev` and confirm:
   - Screensaver activates after 5 minutes of inactivity (check for `ESP_LOGW` diagnostic at 2.5 minutes)
   - Activity (web API, button press) dismisses screensaver and returns to carousel
   - Boot → clearStatus() → carousel transition works with empty pool (no stale modal)
3. **Commit WO1 changes** on branch `pr-13-test-app-wo1-display-lifecycle`

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_sleep_screensaver_modal.md` - Why `pushModal()` was used instead of `showBoard()` for the screensaver, and why `_sleeping` is never set
- `context_artifacts/graveyard_overrideboard_mapping.md` - The WO referenced `_overrideBoard` which doesn't exist; how it was mapped to `_modal`
- `context_artifacts/state_verification_results.md` - Full build and test verification results
