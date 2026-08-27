---
name: drayton-consumer-fixes-wo5
description: Handoff instructions for Work Order 5 — device verification and soak test
complexity: standard
parent_project: pr-drayton-consumer-fixes
branch: drayton-consumer-fixes/wo5-device-verification
base: drayton-consumer-fixes/wo2-docs-facade
---

# WO5 — Device Verification & Soak Test

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Build, flash, and soak-test the complete PR on hardware. Verify the firmware boots cleanly, all MCP tools respond, serial output is healthy, and the device is stable over a 30-minute soak test with no memory leaks, crashes, or watchdog resets.

## Context
All four code WOs (WO1, WO3, WO4, WO2) have landed on the branch. This WO is the final gate before the PR is ready for merge. The firmware now includes:
- Dual-transport support (`WiFiClient*` base pointer, `requiresTls()`)
- `UpdateStatus::Skipped` with correct health semantics
- Per-source `getMaxBackoffMs()` with decoupled quarantine threshold
- Fixed `applyDeadline()` units (#168)
- Façade event payload includes
- FetchGuard RAII extraction
- Supervisory backstop for unsupervised-path sources

## Strict Boundary Box
- **Do NOT** modify any source code — this is a verification-only WO.
- **Do NOT** file GitHub issues. Surface findings in the Completion Report.
- **Do NOT** create new branches — the branch is already checked out.
- **Exception:** You MAY cut a `dev` tag per `.agents/rules/03-versioning.md` — this is autonomous and required before flashing.

## Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `upload_firmware`, `start_monitor`, `query_logs`, `stop_monitor`) — never raw `pio` CLI.

## Line-Item Instructions

### 1. Pre-flight

- [ ] Verify all native tests pass: `platformio` MCP `run_tests`
- [ ] Verify both hardware builds succeed:
  - `platformio` MCP `build_project` for `esp32dev`
  - `platformio` MCP `build_project` for `crowpanel-128-rotary`

### 2. Version Tag

Per `.agents/rules/03-versioning.md` — Tag → Build → Flash:
- [ ] Check current highest tag: `git describe --tags --abbrev=0`
- [ ] Increment dev tag: `git tag v0.X.Y-dev.(N+1)` and push
- [ ] Rebuild after tagging so `inject_version.py` picks up the new tag

### 3. Flash

- [ ] Upload firmware to hardware via `platformio` MCP `upload_firmware` for the `crowpanel-128-rotary` environment
- [ ] Wait for device to boot (allow ~10s for WiFi connection)

### 4. Boot Verification

- [ ] Start serial monitor via `platformio` MCP `start_monitor`
- [ ] Verify clean boot sequence in serial output — no crash, no TWDT panic, no abort
- [ ] Check firmware version matches the new tag via MCP probe or serial output
- [ ] Verify WiFi connects successfully

### 5. MCP Probe

Use the `platformio` MCP tools or direct MCP calls to verify the device's MCP server responds:
- [ ] Probe `get_data_sources` — verify it returns the registered data sources with health states
- [ ] Probe `system_info` or equivalent — verify `firmware_version` matches the new tag
- [ ] Verify all expected MCP tools are listed and responsive

### 6. Soak Test (30 minutes)

- [ ] Ensure serial monitor is running via `platformio` MCP `start_monitor`
- [ ] Set a 30-minute timer
- [ ] During the soak, periodically (every ~5 minutes):
  - Query serial logs via `platformio` MCP `query_logs` — check for:
    - No TWDT panics or watchdog resets
    - No `abort()` calls
    - No heap exhaustion warnings
    - No repeated error spam
    - Data source fetches executing normally (Success/NoChange/Skipped logged)
  - Probe MCP `get_data_sources` — verify health states are sensible:
    - Configured sources should be `Healthy` or `Degraded` (not stuck in `Unknown`)
    - Unconfigured sources should show `Skipped` as `lastStatus` (if WeatherModule has no API key)
    - No sources should be `Quarantined` without visible failures in the log
  - Note DRAM free heap — verify it is stable (not monotonically decreasing)

- [ ] After 30 minutes, collect final state:
  - Final `query_logs` dump
  - Final `get_data_sources` probe
  - Final heap reading
  - Note any anomalies

### 7. Wrap-up

- [ ] Stop serial monitor via `platformio` MCP `stop_monitor`
- [ ] Record results in the Completion Report below

## Verification Requirement

The soak test passes if ALL of the following are true:
1. Device ran for 30 minutes without crash, panic, or reboot
2. DRAM free heap is stable (±2KB of initial reading — no monotonic decline)
3. No TWDT or watchdog resets in serial output
4. MCP tools remained responsive throughout
5. Data source health states are sensible (no false quarantine, Skipped working correctly)
6. No repeated error spam in serial output

**Before recording the Completion Report, ask the user if they are happy with the results.**

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Soak Test Results

| Metric | Value |
|---|---|
| Duration | |
| Initial DRAM free heap | |
| Final DRAM free heap | |
| Heap delta | |
| TWDT panics | |
| Reboots | |
| Error spam | |
| MCP responsive throughout | |
| Firmware version | |

### Data Source Health States (final)
| Source | State | Last Status | Consecutive Failures |
|---|---|---|---|

### Serial Log Anomalies
(none / list)

### MCP Probe Results
(summary)

### Out-of-Scope Findings
(none / list)

### Blockers
(none)
