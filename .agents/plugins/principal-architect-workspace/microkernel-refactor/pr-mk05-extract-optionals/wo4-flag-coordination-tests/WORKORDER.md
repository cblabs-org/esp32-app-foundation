---
name: microkernel-refactor-wo4
description: Handoff instructions for Work order 4 - Flag Coordination and App Templates
complexity: standard
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Finalize the extraction of the remaining Ring-2 optionals by ensuring that `template-app` and `sample-app` compile successfully across the flag matrix. Fix any remaining compilation issues in `sample-app` and record the final flash sizes with optionals off vs on.

## Strict Boundary Box
- Do not modify Ring-1 Core services' ownership.
- Modify `sample-app/` and `template-app/` source code.
- Run tests and record flash size savings.

## Tooling Constraints
Use the `platformio` MCP tools for building and testing. Do not use raw CLI commands for PlatformIO tasks. Verify `native` and `esp32dev` compilation across the flag matrix.

## Checklist
- [x] **Fix `sample-app` display extraction:** Ensure that `sample-app/src/main.cpp` does not call the removed `app.getDisplay()` method. Refactor any calls to use the extracted display instance (`displayModule`).
- [x] **Fix `sample-app` WeatherModule:** Ensure that the `_tempSensor` variable in `sample-app/src/WeatherModule.cpp` is properly guarded by `#ifdef WITH_HOMEASSISTANT` or fixed to avoid undeclared identifier errors.
- [x] **Verify `sample-app` and `template-app`:** Compile both apps with `esp32dev` using the following flag combinations:
  - All optionals ON (`WITH_MCP`, `WITH_OTA`, `WITH_TIME`).
  - All optionals OFF (default for MCP/OTA; manually set `-UWITH_TIME`).
- [x] **Record Flash Deltas:** Record the flash sizes for `template-app` in the `all ON` vs `all OFF` configurations. Append these findings to this `WORKORDER.md`.
- [x] **Verification**: Run `native` tests to ensure no regressions.
- [x] When finished, confirm compilation, then STOP and message the Principal Architect. Do NOT distill or wrap up yet.

## Flash & RAM Delta Measurements (`esp32dev` Board)

### 1. `template-app` (Skeleton Application Template)
| Configuration | Optionals Flag Matrix | Flash Size | Flash Delta | RAM Size | RAM Delta |
|---|---|---|---|---|---|
| **All ON** | `WITH_MCP`, `WITH_OTA`, `WITH_TIME` | 1,246,047 B (1.246 MB) | Baseline | 47,396 B (46.29 KB) | Baseline |
| **All OFF** | Default MCP/OTA off, `-UWITH_TIME` | 1,140,523 B (1.140 MB) | **-105,524 B (-103.05 KB / -8.47%)** | 39,492 B (38.57 KB) | **-7,904 B (-7.72 KB / -16.68%)** |

### 2. `sample-app` (Weather Station Sample Application)
| Configuration | Optionals Flag Matrix | Flash Size | Flash Delta | RAM Size | RAM Delta |
|---|---|---|---|---|---|
| **All ON** | `WITH_MCP`, `WITH_OTA`, `WITH_TIME` | 1,385,915 B (1.386 MB) | Baseline | 54,796 B (53.51 KB) | Baseline |
| **All OFF** | Default MCP/OTA off, `-UWITH_TIME` | 1,334,163 B (1.334 MB) | **-51,752 B (-50.54 KB / -3.73%)** | 48,148 B (47.02 KB) | **-6,648 B (-6.49 KB / -12.13%)** |

### Summary & Verification Status
- **Native Unit Tests**: 449 / 449 test cases succeeded (100% pass rate across all 22 test suites).
- **Compilation**: Clean compilation verified on `esp32dev` for both `template-app` and `sample-app` across the full flag matrix.
- **Payoff**: Disabling optional Ring-2 modules (`WITH_MCP`, `WITH_OTA`, `WITH_TIME`) saves **>100 KB of Flash** and **~8 KB of RAM** in `template-app`, fully validating the Microkernel PR-MK05 extraction objective.

## Status Update
- **Status**: ✅ **COMPLETE & DISTILLED** (2026-07-29)
- **Git Commit**: `6441282d70fe48c48eb1cfd9c5b019e334b8e803` on `pr-mk05-extract-optionals/wo4-flag-coordination-tests`
- **Distillation**: Artifacts created in `context_artifacts/` (`adr_wo4_flag_coordination_tests.md`, `graveyard_wo4_flag_coordination.md`), indexed in `context_bridge.md`, and session registered in `sessions.md`.

