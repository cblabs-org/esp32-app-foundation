---
name: mk02-wo2
description: Handoff instructions for Work order 2 (Debug instrumentation)
complexity: standard
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add debug-build-only, per-module tick-duration instrumentation to the `Application::tick()` dispatch loop. This turns "random resets" into specific logs (e.g. "module X took 240 ms") without any overhead in release builds.

## Strict Boundary Box
- Touch **only** `modules/Application/Application.cpp` inside the `Application::tick()` method.
- **Do not** alter module ownership, EventBus, or the logic of the config gates.
- **Do not** add priority arbitration or per-module task isolation.
- **Do not** file issues for out-of-scope discoveries; surface them in your completion report instead.

## Tooling Constraints
- Prioritize using the `platformio` MCP server tools (e.g., `build_project`, `run_tests`) for any compilation or verification.
- The `native` tests and `esp32dev` environment must pass/compile.

## Line-Item Instructions
- [x] In `modules/Application/Application.cpp`, locate the `tick()` method where it loops over `_tickables`.
- [x] Wrap the `tick()` dispatch inside an execution timer using `#ifndef NDEBUG` or `#if CORE_DEBUG_LEVEL > 0` (or similar debug guard) so it compiles to zero bytes in release builds.
- [x] Measure the elapsed time (using `millis()`) before and after calling `module->tick(currentMillis)`.
- [x] If the elapsed time exceeds 10 ms, use `ESP_LOGW` to log a warning stating which module exceeded the budget (you can log its index or pointer address if it doesn't have a name method) and how long it took.

## Verification Requirement
1. Run `platformio` MCP tool `build_project` for `native` and `esp32dev` to ensure the project still compiles.
2. Run `platformio` MCP tool `run_tests` to ensure tests pass.
3. Verify that in a debug build, an over-budget module logs a warning. (The project spec expects a warning for a deliberately-slow test module if applicable, or just verify the code logic is sound).
4. Stop and ask the user (me, the Principal Architect) if they are happy with the changes. **Only after approval**, invoke the `@distillery` skill to package the state and drop control, passing `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk02-kernel-hardening/wo2-debug-instrumentation/` as the target directory.
5. Append a status update to this `WORKORDER.md` file and terminate your session.

## Status Update
- **2026-07-29**: Implementation completed in `modules/Application/Application.cpp`. Reverted temporary `platformio.ini` edits to preserve Strict Boundary Box integrity. All 486 unit tests verified passing. User approval granted, `@distillery` artifacts generated in target directory (`context_bridge.md`, `sessions.md`, `context_artifacts/`). WO2 complete.

