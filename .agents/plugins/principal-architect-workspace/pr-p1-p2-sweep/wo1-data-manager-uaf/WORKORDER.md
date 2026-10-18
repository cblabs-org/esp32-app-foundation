---
name: p1-p2-sweep-wo1
description: Handoff instructions for Work Order 1 (DataManager UAF fix)
complexity: standard
---

# WORKORDER: p1-p2-sweep-wo1

### Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

### Objective
Fix the P1 bug (Issue #175) in `DataManager.cpp` where a stack-local `DataSourceHealthPayload` is incorrectly published to the asynchronous `EventBus` from the worker task.

### Strict Boundary Box
- DO NOT touch files outside of `modules/DataManager/`.
- DO NOT modify the fundamental architecture of `EventBus`.

### Tooling Constraints
- Use the `platformio` MCP tools (e.g., `build_project`, `run_tests`) to compile and test, per project guidelines.
- Target `crowpanel-128-rotary` or `esp32dev` for a hardware build check.
- `pio test -e native` to ensure no regressions.

### Line-Item Instructions
- [x] In `include/modules/DataManager/DataManager.hpp`, add a new private member `DataSourceHealthPayload _publishPayload;` to the `DataManager` class.
- [x] In `src/modules/DataManager/DataManager.cpp`, inside `DataManager::_processOneFetch()` (around line 725), replace `DataSourceHealthPayload localPayload{};` with the new class member `_publishPayload`. Update references accordingly.
- [x] Verify that the pointer passed to `_eventBus->publish()` points to `_publishPayload` and not a stack-local variable.

### Verification Requirement
Verify the project compiles and the native tests pass.
**CRITICAL:** Before committing, you MUST ask the user if they are happy with the changes.

### Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
- `modules/DataManager/DataManager.hpp`: Added private member `DataSourceHealthPayload _publishPayload;` to `DataManager`.
- `modules/DataManager/DataManager.cpp`: In `executeLoop()`, replaced stack-local `localPayload` with member `_publishPayload` and passed `&_publishPayload` to `_eventBus->publish(ApplicationEvents::DATA_SOURCE_HEALTH, &_publishPayload)` to prevent UAF.

### Decisions Made
- Replaced stack allocation with member variable storage on `DataManager` instance per Issue #175 resolution instructions.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.

### Blockers
- None.

