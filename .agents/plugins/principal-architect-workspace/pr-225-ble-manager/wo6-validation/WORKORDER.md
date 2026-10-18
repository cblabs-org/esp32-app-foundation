---
name: pr-225-ble-manager-wo6
description: Human-in-the-loop hardware acceptance run of RFC §17.2 (amended) on esp32s3nano_ble with sample-app reference providers and real FTMS apps
complexity: interactive
---

# WO-6 — Hardware Acceptance Run (Interactive)

**Parent project:** [`pr-225-ble-manager`](../PROJECT.md) (approval provenance: PROJECT.md → *Approval Record*)
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225). Design: RFC rev 2, §17.2, as amended by the [acknowledgement](https://github.com/matthewmcneill/esp32-app-foundation/issues/225#issuecomment-5735199501) (item 6 is split into 6a/6b/6c).
**Branch:** `ble-manager/wo6-validation` (base: `ble-manager/wo5-coexistence-ota`). This branch is the **PR head**.

## Role Definition
> You are an expert engineer running an **interactive** validation session **with the human**. You are NOT the Principal Architect. You execute the acceptance protocol, record evidence, and fix **only** defects that are clearly local to `modules/BLEManager/` or the sample providers, each in a small commit the human approves. Anything larger is written down as a finding for the PA, not fixed.

## Objective
Run the acceptance criteria on real hardware with real centrals, record measured evidence in `wo6-validation/ACCEPTANCE.md`, and leave the stack ready for PR submission.

## Strict Boundary Box
- **Fixes allowed:** `modules/BLEManager/**` and `sample-app/src/ble/**` only, each fix approved by the human before commit.
- **Not allowed:** interface changes, changes to other modules, and design changes. Record those as findings.
- Do NOT create branches.

## Tooling Constraints
- `platformio` MCP: `build_project`, `upload_firmware`, `start_monitor`, `capture_serial_window`, `query_logs`. **Versioning rule 03: tag `dev`, then build, then flash**, and confirm `firmware_version` via `/api/system` after boot.
- Test kit (confirm with the human at the start):
  - `esp32s3nano`;
  - phones/tablets running nRF Connect;
  - ≥ 2 FTMS apps (e.g. Zwift + one of TrainerRoad / MyWhoosh / Kinomap / Garmin);
  - a BLE HR source (a real strap, or nRF Connect's peripheral simulator);
  - a Wi-Fi client for the WebSocket/HTTP load. The sample-app's portal/status polling can serve as the 2 Hz Wi-Fi stream if no overlay exists. Document what you used.

## Acceptance Protocol → `ACCEPTANCE.md` (evidence per item: numbers, log excerpts, screenshots)
1. **Builds:** root + sample-app `esp32s3nano_ble` clean; non-BLE envs unchanged (quote the WO-1 deltas).
2. **Budgets:** flash, internal-RAM and PSRAM at idle; at 3 peripheral + 1 central; and during OTA. The OTA image fits the 4 MB slot.
3. **Discovery/adverts:** the nRF Connect primary PDU and scan response match the boot log; **≥ 2 real FTMS apps** discover and connect to `SampleFtmsProvider`.
4. **Coexistence at max config (5 min):** 3 peripherals (FTMS notifying 2 Hz on ≥ 1) + 1 central (HR ~1 Hz) + 2 Hz Wi-Fi traffic + one large HTTP POST.
   - Pass: no FTMS gap > **600 ms**, no HR steady-state gap > **2.5 s**, no Wi-Fi/WebSocket disconnect.
   - Record the maximum gaps observed and how you measured them (e.g. nRF Connect log timestamps).
5. **Relay latency:** host receives the HR notification → re-broadcast queued, **p99 < 50 ms** over ≥ 300 samples (from the `receivedAtMs` debug log).
6. **Dropout and recovery:**
   - **6a:** HR off 10–30 s → reconnected ≤ **35 s** after power-on;
   - **6b:** off > 90 s → reconnected ≤ **65 s**;
   - **6c:** app stale detection ≤ **12 s**.
7. **Forget / re-pair:** no phantom bond (bond count checked).
8. **OTA:** a manual OTA pauses BLE and disconnects peers, and the portal stays responsive; a failed OTA resumes BLE.
9. **CONFIGURING:** no `ble.scan.*`, no advertising, no central activity (from logs/events).
10. **Portal/MCP:** scan/pair/forget from the portal; `ble_status` in read-only MCP mode; mutating tools refused in read-only mode.

## Completion
- Append the Completion Report (below) with pass/fail per item and every local fix committed.
- **Give the human a return prompt for the PA**, stating that WO-6 is complete, the verdict per item, and where `ACCEPTANCE.md` is.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
complete | partial | blocked

### Changes Made
- `path/to/file`: what changed and why

### Decisions Made
- Decision description and rationale

### Dead Ends & Warnings
- What was attempted, why it failed, and what to avoid

### Out-of-Scope Findings
- Finding description and suggested action

### Blockers
(none)
