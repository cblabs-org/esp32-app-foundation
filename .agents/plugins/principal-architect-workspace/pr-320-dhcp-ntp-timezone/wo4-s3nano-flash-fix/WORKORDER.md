---
schema_version: 1
wo_id: WO-320.4
pr_id: PR-320
title: Restore board_upload.arduino.flash_extra_images suppression for esp32s3nano
  in platformio.ini
complexity: standard
status: PENDING
allowed_files:
- platformio.ini
forbidden_files:
- docs/design_specification.md
required_skills:
- pio-manager
verification_command: python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: f2183cac0db0a98c189131bf73ee821408ed68eae1269115fdd17f96ec484fbb
completion: null
---

# WO-320.4 — Restore board_upload.arduino.flash_extra_images suppression for esp32s3nano in platformio.ini

## 1. Objective & Scope

Restore the `board_upload.arduino.flash_extra_images =` suppression in `[env:esp32s3nano]` inside `platformio.ini` so `esp32s3nano` (and `[env:esp32s3nano_ble]`, which extends it) builds and flashes cleanly in hybrid `arduino, espidf` mode without failing on a broken Tasmota-prefixed path for `nora_recovery.ino.bin`.

## 2. Detailed Specification

1. Edit `platformio.ini` under `[env:esp32s3nano]`, immediately below `upload_protocol       = esptool` (line 92), and restore:
   ```ini
   ; Suppress nora_recovery.ino.bin merge — espressif32 platform builder (espidf.py
   ; L3168) hardcodes a Tasmota-specific path prefix when resolving flash_extra_images,
   ; producing a broken path in hybrid arduino+espidf builds. The binary exists in the
   ; framework package but the builder can't resolve it. Platform bug, not ours.
   board_upload.arduino.flash_extra_images =
   ```
2. Do **NOT** duplicate this setting in `[env:esp32s3nano_ble]`: `[env:esp32s3nano_ble]` sets `extends = env:esp32s3nano` and therefore inherits `board_upload.arduino.flash_extra_images =` automatically.
3. Make no other edits to `platformio.ini` or any other file.

## 3. Verification Requirements

1. Run `python3 scripts/doc_lint.py` and confirm exit code `0`.
2. Run PlatformIO MCP `build_project` on `esp32s3nano` with `forceExecution: true` (ALWAYS `acquire_lock` -> `build_project` -> `release_lock`, never shell `pio`!) and verify the build succeeds cleanly without `nora_recovery.ino.bin` merge errors.
3. Confirm `git status --porcelain` shows only `platformio.ini` (plus this `WORKORDER.md` once Part II is updated).
4. Record your work summary in Part II (`completion.verification_summary`, `completion.decisions_made`, and the final report section below) BEFORE running `pa wo verify WO-320.4` as your final mutating action.

## Completion Report

_Populated by pa-engineer upon completion._
