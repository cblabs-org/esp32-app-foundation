---
schema_version: 1
wo_id: WO-320.4
pr_id: PR-320
title: Restore board_upload.arduino.flash_extra_images suppression for esp32s3nano
  in platformio.ini
complexity: standard
status: COMPLETED
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
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_5137802c5a45
  commit_sha: 504bb1ee44b1cc9a4f5bdbf809f9cd231ab3c033
  files_modified:
  - platformio.ini
  verification_summary: doc_lint.py exited 0; esp32s3nano MCP build SUCCEEDED (00:02:26.144,
    RAM 18.6%, Flash 32.9%, web tests passed, nora_recovery.ino.bin merge suppressed).
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Restored the 4-line explanatory comment and empty board_upload.arduino.flash_extra_images
    = assignment directly below upload_protocol = esptool in [env:esp32s3nano] inside
    platformio.ini.
  - Did not duplicate board_upload.arduino.flash_extra_images = into [env:esp32s3nano_ble]
    because it extends env:esp32s3nano and inherits the setting automatically.
  - Executed esp32s3nano compilation via PlatformIO MCP (acquire_lock -> build_project
    with forceExecution=true -> release_lock) rather than shell pio, and escalated
    receipt stamping to PA due to HARNESS_PORCELAIN_LSTRIP_BUG.
  dead_ends: []
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

### Execution Summary
Restored `board_upload.arduino.flash_extra_images =` (with the 4-line explanatory comment) immediately below `upload_protocol       = esptool` inside `[env:esp32s3nano]` in `platformio.ini`, without duplicating it in `[env:esp32s3nano_ble]` (which sets `extends = env:esp32s3nano` and inherits the suppression automatically). Made no other edits to `platformio.ini` or any other source file.

### Verification Results
- **Doc Lint (`python3 scripts/doc_lint.py`):** Exited `0` with zero errors.
- **PlatformIO MCP Build (`acquire_lock` -> `build_project(projectDir="/Users/mcneillm/Documents/Projects/esp32-app-foundation", environment="esp32s3nano", forceExecution=true)` -> `release_lock`):**
  - Status: `SUCCESS` in `00:02:26.144` (`146.14s`), including `[web-tests] All web tests passed`.
  - Memory usage: RAM `18.6%` (`61,084` / `327,680` bytes), Flash `32.9%` (`1,381,139` / `4,194,304` bytes).
  - Build log (`.pio-mcp-workspace/logs/build/build-2026-11-15T13-16-29-488Z-e5571604.log`): confirmed `board_upload.arduino.flash_extra_images: ;`, zero occurrences of `nora_recovery` in the build output, and `firmware.factory.bin` assembled cleanly from `bootloader.bin` (`0x0`), `partitions.bin` (`0x8000`), `boot_app0.bin` (`0xe000`), and `firmware.bin` (`0x10000`).
  - Hardware lock cleanly acquired and released (`sessionId: 8e1b3267-a0fb-4c6e-ad7c-5de90013677c`).
- **Git Status (`git status --porcelain`):** Shows only `platformio.ini` (plus `.agents/plugins/principal-architect-workspace/.state/state.db` and this `WORKORDER.md` Part II update).

### Escalation Notice
`pa wo verify WO-320.4` was intentionally skipped per instructions (`HARNESS_PORCELAIN_LSTRIP_BUG`), because `pa_cli.py:_parse_porcelain_paths()` applies `.lstrip("./")` to porcelain paths and strips the leading dot from `.agents/.../WORKORDER.md`, causing a false scope-hygiene rejection. All work in `allowed_files` (`platformio.ini`) is complete and verified, Part I `spec_hash` is unchanged, and `WO-320.4` is escalated to `pa` for receipt stamping and commit creation.
