---
name: pr-27-wo2
description: Handoff instructions for Work Order 2 — Build, test, and measure TLS optimization impact
complexity: standard
---

# Work Order 2 — Build, Test, and Measure TLS Optimization Impact

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Build the firmware for esp32s3nano, run native tests, measure before/after heap and flash metrics, and verify all TLS-dependent features still work. Document results for WO3.

## Versioning

Read and follow `.agents/rules/03-versioning.md`. If this build includes the new TLS optimisation settings (which constitute new functionality), create a dev tag following the decision tree in that rule.

## Strict Boundary Box

- Do **NOT** modify any source code files (`.cpp`, `.hpp`, `.h`, `.c`)
- Do **NOT** modify `sdkconfig.defaults` or `platformio.ini`
- Do **NOT** create any branches or make git commits (the PA will handle this)
- You **MAY** run build commands, test commands, and flash commands
- You **MAY** create measurement result files in the PA workspace directory

## Prerequisites

WO1 must be complete — `sdkconfig.defaults` must already contain the PR-27 TLS optimisation block, and the generated `sdkconfig.*` files must have been deleted.

## Line-Item Instructions

### Phase A: Baseline Capture (before-optimisation reference)

> **NOTE**: If a before-optimisation firmware is NOT currently flashed on the device,
> skip Phase A and note "baseline not available" in results. The PA may have baseline
> numbers from previous sessions.

- [ ] **A1.** Query the device's current heap state via the web portal `/api/system` endpoint or MCP `system_info` tool. Record: `heap_free`, `heap_min_free`, `heap_max_alloc`, `psram_free`, `psram_total`.

### Phase B: Build & Flash

- [ ] **B1.** Run native unit tests to confirm no regressions from the config change:
  ```bash
  pio test -e native
  ```
  All 525+ tests must pass.

- [ ] **B2.** Build for esp32s3nano and capture firmware size:
  ```bash
  pio run -e esp32s3nano
  ```
  Record the flash size output (`.text`, `.data`, `.rodata`, `.bss`, total).

- [ ] **B3.** Also build for esp32dev to confirm it compiles (no flash needed):
  ```bash
  pio run -e esp32dev
  ```

- [ ] **B4.** Flash the esp32s3nano board:
  ```bash
  pio run -e esp32s3nano -t upload
  ```

### Phase C: Post-Optimisation Measurement — Idle Steady-State

- [ ] **C1.** Wait for the device to boot and connect to WiFi.

- [ ] **C2.** Query `/api/system` or MCP `system_info`. Record: `heap_free`, `heap_min_free`, `heap_max_alloc`, `psram_free`.

- [ ] **C3.** Compare with Phase A baseline (if available). Document the delta for each metric.

- [ ] **C4.** Check the OLED DiagnosticsBoard display for the heap reading.

### Phase D: Functional Regression Tests

- [ ] **D1. HTTPS Portal Test**: Access the web portal over HTTPS (port 443).
  - Verify all tabs load
  - Verify Settings page saves work
  - Verify the self-signed cert is served correctly

- [ ] **D2. GitHub API Test**: Trigger an OTA check (via web portal or MCP tool) to verify GithubClient can fetch release info from the GitHub API (CA-validated TLS using pinned Sectigo Root E46).

- [ ] **D3. OTA Download Test**: If an OTA update is available, allow the download to proceed. During the active download:
  - Capture `heap_free` and `heap_max_alloc` (expect peak usage similar to pre-optimisation — buffers are allocated during active transfer)
  - Verify `heap_min_free` doesn't drop below 20 KB (the heap guard threshold)
  - If no OTA is available, note this and skip the active-transfer measurement.

### Phase E: Results Documentation

- [ ] **E1.** Create a results file at:
  `.agents/skills/principal-architect-workspace/pr-27-tls-optimization/wo2-build-test-measure/RESULTS.md`

  Include:
  - Before/after heap metrics (idle steady-state)
  - Heap metrics during active OTA download (if captured)
  - Flash size comparison (before/after from `pio run -e esp32s3nano` output)
  - Pass/fail status for each functional test (portal, GitHub API, OTA)
  - Any unexpected observations

## Verification Requirement

After completing all checklist items:
1. Confirm all functional tests passed
2. Confirm results are documented in `RESULTS.md`
3. **STOP and send a message back to the Principal Architect** reporting completion with a summary of key metrics. Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
