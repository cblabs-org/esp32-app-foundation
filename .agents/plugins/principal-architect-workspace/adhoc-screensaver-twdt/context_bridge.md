---
title: Screensaver TWDT Crash — Root Cause Found & Fixed
distilled_at: 2026-06-07T00:43:00Z
original_plan_id: standalone
session_id: 34de947b-cd38-4c49-8e6b-3c79452f4912
artifacts:
  - context_artifacts/adr_screensaver_twdt.md
  - context_artifacts/graveyard_screensaver_investigations.md
  - context_artifacts/state_screensaver_final.md
---

## Executive Summary

The deterministic 300s boot-loop crash on the UM NanoS3 is **fully resolved**. Root cause was a Task Watchdog Timer (TWDT) firing because `U8G2DisplayModule::tick()` returned in microseconds when blanked (no rendering path), causing `loopTask` to spin at MHz speed and starve `IDLE1` on CPU 1. Fixed with a single `vTaskDelay(33ms)` in the blanked early-return path. `setPowerSave(1)` (genuine hardware sleep) is confirmed working with this fix in place.

## Current Device State

- **Firmware:** `v0.0.6-beta.3` running on `esp32s3nano` (UM NanoS3)
- **Screensaver lifecycle:** Working end-to-end — carousel → dim (bouncing clock at 150s) → hardware sleep `setPowerSave(1)` at 300s → screen truly off (DC-DC converter down)
- **Uptime at close:** 379s+ stable, no TWDT, no crash
- **USB JTAG console:** Re-enabled (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`) — correct port is `/dev/cu.usbmodem101`
- **Git:** All commits pushed to `origin/main`, tag `v0.0.6-beta.3` pushed

## Commits This Session (all on `main`)

| Hash | Description |
|------|-------------|
| `7e5d9fd` | chore: close screensaver TWDT crash item in todo list |
| `842a7e5` | fix(screensaver): use setPowerSave(1) for true hardware sleep on blank |
| `7383f2b` | fix(screensaver): clear display buffer on blank (intermediate step) |
| `026354b` | **fix(screensaver): TWDT crash fix — vTaskDelay in blanked path** ← core fix |
| `767ee4f` | fix(debug): USB JTAG console re-enabled; localtime_r thread safety |
| `ea48bbc` | fix(display): setPowerSave → setContrast (superseded, kept for history) |
| `088df9d` | merge(pr-19): sdkconfig base+delta layering (from previous session) |

## Next Steps

1. **Verify wake behaviour** — touch the device (reset activity) and confirm the screen wakes correctly from `setPowerSave(0)` + `setContrast(255)`. Not yet tested in this session.
2. **Drop stash{1}** — `34f07b6` "remaining-unstaged". ScreensaverBoard `setClockFont` changes are already confirmed in main. Drop the stash: `git stash drop stash@{1}`.
3. **Investigate portal screensaver toggles** — Always-grey toggle switches in `display.js` (see todo list). The stash drop above may be a prerequisite — check first.
4. **OTA end-to-end test** — `v0.0.6-beta.3` on device, `v0.0.6-beta.2` on GitHub. OTA auto-check fires at boot but currently fails with `JSON Parse Error: IncompleteInput` — the GitHub API response may be too large for the device's HTTP buffer. Investigate `CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN` or streaming JSON parse.
5. **Harden OTA task stack** — See todo list item: pre-flight heap check, `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM)` for OTA task stack.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_screensaver_twdt.md` — Why vTaskDelay, why setPowerSave(1), why not setContrast or clearBuffer
- `context_artifacts/graveyard_screensaver_investigations.md` — Every failed hypothesis tested: MSPI bus theory, ELF mismatch decode, setPowerSave-as-cause, setContrast-as-cause, no-hardware-call — all dead ends with evidence
- `context_artifacts/state_screensaver_final.md` — Final implementation state of ScreensaverController and U8G2DisplayModule tick
