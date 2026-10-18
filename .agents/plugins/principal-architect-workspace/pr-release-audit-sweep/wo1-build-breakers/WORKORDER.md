---
name: release-audit-sweep-wo1
description: Handoff instructions for Work Order 1 — Build Breakers
complexity: standard
---

# WO1 — Build Breakers

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix three compile failures that break specific `platformio.ini` build environments. These are small, targeted fixes — include guard placement, lambda capture correction, and predicate alignment.

## Strict Boundary Box
- **DO NOT** modify any files outside the ones listed in the checklist
- **DO NOT** refactor OTA logic, display rendering, or Home Assistant entity handling
- **DO NOT** touch `DataManager`, `WebServer`, `Application`, portal JS/CSS, or test files
- **DO NOT** create new branches — you are already on `release-audit-sweep/wo1-build-breakers`
- **DO NOT** file GitHub issues — surface any out-of-scope findings in your Completion Report

## Tooling Constraints
- Use the `platformio` MCP tools (`build_project`) to verify builds, NOT raw `pio` CLI
- Build verification must cover at minimum: `esp32s3nano`, `esp32dev_all_on`, and `native` environments
- Read the framework-expert skill at `.agents/skills/framework-expert/SKILL.md` if you need interface/pattern context

## Line-Item Instructions

### Issue #155 — HomeAssistantModule include guard
- [ ] `modules/HomeAssistant/HomeAssistantModule.cpp` — The `#include "utils/serveGzipAsset.hpp"` at line 16 is OUTSIDE the `#ifdef WITH_HOMEASSISTANT` guard at line 18. Move this include inside the guard so it's only pulled in when the HA module is compiled. The file should compile cleanly when `WITH_HOMEASSISTANT` is not defined.

### Issue #153 — U8G2 OTA predicate static call
- [ ] `modules/U8G2Display/U8G2DisplayModule.cpp` — Around lines 821–823, the OTA predicate lambda calls `OTAUpdateManager::getState()` as a static method, but `getState()` is an instance method. Fix by capturing the OTA manager pointer (if available via the service locator in `begin()`) or by using an alternative approach that doesn't require a static call. The predicate is set via `_serviceHelper.setOtaPredicate(...)`.

### Issue #138 — LVGL OTA predicate alignment
- [ ] `modules/LVGLDisplay/LVGLDisplayModule.cpp` — Around lines 1008–1010, the LVGL display module sets its own OTA predicate. This must be aligned with whatever fix was applied to the U8G2 module above. Both display modules should use the same approach (injected OTA manager pointer or equivalent) for the service-route predicate.
- [ ] Verify both predicates have identical semantics: they should return `true` when OTA is in progress (blocking display service routes) and `false` otherwise.

## Verification Requirement
1. Build all environments using PlatformIO MCP `build_project` tool:
   - `esp32s3nano` — the primary board
   - `esp32dev_all_on` — the env that #153 breaks
   - `native` — host build for tests
2. Confirm zero compile errors and zero new warnings
3. **CRITICAL**: Before committing, ask the user if they are happy with the changes

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->
<!-- WORKER: Fill this in when you finish -->
