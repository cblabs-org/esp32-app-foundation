---
name: pr-mk06-wo1
description: Handoff instructions for Work order 1 - first cohort
complexity: standard
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Introduce a `Periodic` helper class (header-only) to DRY out the `millis()` throttling logic, and migrate the first cohort of kernel/application core sites to use it.

## Strict Boundary Box
- Add the `Periodic` helper and migrate the `Application` and basic kernel throttle sites. 
- **No behavioural change** — intervals and fire semantics must remain identical.
- **Do not** touch module ownership, EventBus, or composition.
- **Do not** modify other modules in `modules/` or `lib/` yet (that is for WO2).
- Skip any throttle that is intentionally not periodic (e.g. one-shot guards).
- Surface findings, don't file them.

## Tooling Constraints
- Use the `platformio` MCP tools for building and testing. 
- You must verify compilation in both `native` and `esp32dev` environments. 
- Existing timing tests must pass unchanged.

## Checklist
- [x] Create `include/utils/Periodic.hpp` (or similar appropriate location) defining `class Periodic` holding an `interval` and `last-fire` state, with a `bool due(uint32_t now)` method.
- [x] Migrate `modules/Application/Application.cpp` and `modules/Application/Application.h` to use `Periodic`.
- [x] Migrate any other core kernel/application throttles (not specific feature modules like Wifi or WebServer) if applicable.
- [x] Verify `native` + `esp32dev` environments are green.
- [x] Ask the user if they are happy with the implementation.
- [x] Upon user approval, invoke the `@distillery` skill to package the state and drop control, passing `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk06-periodic-helper/wo1-first-cohort/` as the target directory.
- [x] Append a status update to this `WORKORDER.md` file and terminate your session.

## Status Update

- **Date:** 2026-07-29
- **Status:** ✅ Completed and Verified
- **Summary:**
  - Created `include/utils/Periodic.hpp` header-only helper class.
  - Added unit test suite `test/test_unit/test_periodic/test_periodic.cpp` covering constructors, due behavior, non-mutating `check()`, `reset()`, `setInterval()`, and overflow-safe math.
  - Migrated `modules/Application/Application.hpp` and `modules/Application/Application.cpp` `tick()` throttles (`_gateCheckPeriodic`, `_heartbeatPeriodic`).
  - Added `LVGLDisplay` to `lib_ignore` for `base_esp32` in `platformio.ini`.
  - Verified `native` environment: 490/490 unit tests passed.
  - Verified `esp32dev` environment: firmware compilation succeeded.
  - Distillation package created in `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk06-periodic-helper/wo1-first-cohort/`.
