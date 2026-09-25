---
name: pr-225-ble-manager-wo11
description: Handoff instructions for Work Order 11 — remove consumer-app-specific names from framework tests and sample
complexity: mechanical
---

# WO-11 — App-neutral tests and sample (PR #235)

**Parent project:** `pr-225-ble-manager` (PR #235). The approval is recorded in `../PROJECT.md` → *Approval Record* (WO-11 entry, 2026-09-25).
**Branch:** `ble-manager/wo11-app-neutral`, already checked out in this worktree. Base: `ble-manager/wo6-validation`.

## Role Definition
You are an execution-focused worker agent. You are NOT the Principal Architect. Do not create branches, do not push, and do not plan. Execute the checklist exactly.

## Objective
The framework must be app-agnostic. Remove the downstream consumer app's name, and its spec references, from the framework's BLE tests and sample. **Test behaviour and byte counts must stay identical.**

## Strict Boundary Box
- **Only these files may change:**
  - `test/test_unit/test_bleManager/test_bleAdvertisingBuilder.cpp`
  - `test/test_unit/test_bleManager/test_bleConfig.cpp`
  - `sample-app/src/ble/SampleFtmsProvider.cpp`
  - the Completion Report in this WORKORDER
- Change comments and string literals only, plus the one test name and the one appearance constant listed below. No logic changes. Don't touch any other file, and in particular don't touch `.agents/` except this WORKORDER.
- Do NOT push, tag, flash, or create branches.

## Tooling Constraints
The `platformio` MCP server is not available in this harness. Use `~/.platformio/penv/bin/pio` for build and test only. No flashing. Run from the worktree root. Note that the test filter needs a glob:
- `~/.platformio/penv/bin/pio test -e native -f "*test_bleManager"`
- `cd sample-app && ~/.platformio/penv/bin/pio run -e esp32s3nano_ble`

## Line-Item Instructions
- [ ] `test_bleAdvertisingBuilder.cpp`:
  - Rename the test `EchenonWorkedExample` → `FitnessProfileWorkedExample`.
  - In the file header (line ~10), change "Validates RFC §7 Echenon worked example" → "Validates the RFC §7 worked example (standard SIG FTMS + Heart Rate profiles)".
  - Change the comment "Echenon elements from RFC §7 and §20:" → "Worked-example elements (RFC §7):".
  - Replace the device name `"Echenon"` with `"Example"` in the `pack()` call. Both names are 7 characters, so the scan response stays 14 bytes.
  - Update the expected string `"Echenon-1A2B"` → `"Example-1A2B"`, and its comment.
  - Replace `adv.setAppearance(0x0491);` and its comment with `adv.setAppearance(0x0540); // Generic Sensor (valid SIG appearance; same 4-byte AD size)`. `0x0491` is not a defined SIG Appearance value.
  - Every `EXPECT_EQ` byte count must remain unchanged.
- [ ] `test_bleConfig.cpp`: `"TestEchenon"` → `"TestDevice"`, in both the input and the expected value.
- [ ] `SampleFtmsProvider.cpp`: change the comment `// FTMS Indoor Bike Data packet (Echenon SPEC §7.4):` → `// FTMS Indoor Bike Data packet (Bluetooth SIG Fitness Machine Service v1.0, Indoor Bike Data):`.
- [ ] Afterwards, run `grep -rn -i "echenon" test/ sample-app/src/ modules/ src/ docs/`. It must return nothing.

## Verification Requirement
1. `pio test -e native -f "*test_bleManager"` passes, with the same test count as before.
2. `sample-app` `esp32s3nano_ble` builds.
3. The grep above is empty.
4. **Commit** on `ble-manager/wo11-app-neutral` as `test(ble): make BLE tests and sample app-neutral`. End the commit message with the `Co-Authored-By` trailer that matches your own model identity. **Do not push.**

## Issue Surfacing
Out-of-boundary findings go under *Out-of-Scope Findings* in your Completion Report.

---
## Completion Report

### Status
✅ COMPLETE

All objectives achieved: framework tests and sample are now app-neutral. Consumer application references removed, replaced with generic RFC §7 terminology and standard Bluetooth SIG spec references.

### Changes Made

**test_bleAdvertisingBuilder.cpp:**
- Renamed test: `EchenonWorkedExample` → `FitnessProfileWorkedExample`
- Line 10 file header: updated description to "Validates the RFC §7 worked example (standard SIG FTMS + Heart Rate profiles)"
- Line 21 comment: "Echenon elements from RFC §7 and §20:" → "Worked-example elements (RFC §7):"
- Line 30 appearance value: 0x0491 → 0x0540 (Generic Sensor, valid SIG value, maintains 4-byte AD size)
- Line 38 device name: "Echenon" → "Example" (7 characters, scan response remains 14 bytes)
- Line 52-59: expected name "Echenon-1A2B" → "Example-1A2B" with updated comment

**test_bleConfig.cpp:**
- Line 65 & 75: "TestEchenon" → "TestDevice" (both input and expected values)

**SampleFtmsProvider.cpp:**
- Line 134 comment: "FTMS Indoor Bike Data packet (Echenon SPEC §7.4):" → "FTMS Indoor Bike Data packet (Bluetooth SIG Fitness Machine Service v1.0, Indoor Bike Data):"

### Verification Results
- ✅ Tests: 52 test cases passed (native environment, same count as before)
  - Test name successfully changed to `BLEAdvertisingBuilderTest.FitnessProfileWorkedExample`
  - All byte counts verified (primary: 20 bytes, scan response: 14 bytes unchanged)
- ✅ Build: sample-app compiled for esp32s3nano_ble (1.7 MB firmware.bin generated)
- ✅ Grep: zero "echenon" references remain in test/, sample-app/src/, modules/, src/, docs/

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.

**Committed:** `0ec19b87` (ble-manager/wo11-app-neutral)
**Worker:** Claude Haiku 4.5
