---
name: p2-issue-remediation-wo4
description: Handoff instructions for Work Order 4 — revive dormant test_application and test_otaUpdateManager test suites
complexity: architect
---

# WO4 — Revive test_application + test_otaUpdateManager Test Suites

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Enable the dormant `test_application` (~40 tests, 38KB) and `test_otaUpdateManager` test suites on the `native` platform. These test the framework's core microkernel orchestrator (Application — registration, compact-and-lock, state machine, begin/tick dispatch, soft-reset) and the OTA update manager.

## Context
- WO2 already wired the real U8G2 library for native and WO3 fixed the U8G2 display test suite.
- `Application.hpp` includes ~24 headers. Most are already native-safe — EventBus, ConfigManager, WifiManager, KeyManager, AuthManager, DataManager, WebServerManager all have live native suites.
- An `exclude_on_native.py` already exists at `modules/Application/exclude_on_native.py` — verify it correctly excludes `Application.cpp` on native while keeping headers available.
- `OTAUpdateManager.hpp` pulls in FreeRTOS task headers, HTTPClient, SPI, etc. — stubs exist for most of these in `test/stubs/platform/`.
- Display module references in `Application.hpp` may need `#ifdef UNIT_TEST` guards if they pull in display-specific types that don't resolve.
- The `PngWriter` and `DisplayServiceHelper` libraries are in `lib_ignore` for native — any references to them from Application may need guards.

## Strict Boundary Box
- **ONLY** modify files within `test/test_unit/test_application/` and `test/test_unit/test_otaUpdateManager/`.
- You MAY modify `platformio.ini` `[env:native]` to remove these suites from `test_ignore`.
- You MAY add stubs to `test/stubs/platform/` if needed.
- You MAY add `#ifdef UNIT_TEST` guards to framework headers if absolutely necessary to resolve native compilation — but keep changes minimal and document each one.
- **DO NOT** modify module `.cpp` implementation files.
- **DO NOT** modify test files in `test/test_unit/test_u8g2Display/` or `test/test_unit/test_lvglDisplay/`.
- **DO NOT** branch. You are working on the branch that is already checked out.

## Tooling Constraints
- Use the `platformio` MCP server for test execution (`run_tests`) and build verification (`build_project`).
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### PlatformIO configuration
- [ ] `platformio.ini` `[env:native]` — Remove `test_unit/test_application` and `test_unit/test_otaUpdateManager` from `test_ignore`. After this change, `test_ignore` should only contain `test_native/test_hardware_board`.

### Application test revival
- [ ] **Read `test/test_unit/test_application/test_application.cpp`** — understand all ~40 tests and what APIs they exercise.
- [ ] **Attempt a native test build** — collect compile errors.
- [ ] **Fix include paths and API drift** — Application has evolved significantly. Update mocks, includes, and test instantiations.
- [ ] **Add missing stubs** to `test/stubs/platform/` if needed for headers pulled in transitively by `Application.hpp`.
- [ ] **Add `#ifdef UNIT_TEST` guards** to framework headers ONLY if absolutely necessary — prefer creating stubs over modifying framework source. Document every guard you add.

### OTAUpdateManager test revival
- [ ] **Read `test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp`** — understand what it tests.
- [ ] **Fix compile errors** — similar process to Application tests.
- [ ] **Add stubs/guards as needed** — same constraints as above.

### Integration verification
- [ ] **Run full native test suite** — all existing tests (including the new U8G2 tests from WO3) plus the newly enabled Application and OTA tests must pass.
- [ ] Report the final test count.

## Verification Requirement
1. Run `pio test -e native` via the MCP server — **all tests must pass**. This is the final gate.
2. Report the total native test count (should be significantly higher than the WO3 baseline).
3. Build `esp32dev` and `esp32s3nano` environments to verify no hardware regression.
4. **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
