---
name: issue-sweep-aug-wo1
description: Handoff instructions for Work Order 1 — Fix WITH_OTA compilation defects
complexity: standard
---

# WO1 — Fix WITH_OTA Compilation Defects

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix two compilation defects that prevent consumer apps from enabling `-D WITH_OTA`. Both are small, surgical fixes. Closes **#120**.

## Strict Boundary Box
- **ONLY** modify the two files listed below.
- Do NOT refactor surrounding code, add features, or change any other OTA behaviour.
- Do NOT touch `OTAUpdateManager.hpp`, `DisplayServiceHelper`, or any other files.

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (rule `04-mcp-tooling.md`).
- Verify with: `build_project` targeting `crowpanel-128-rotary` with `-D WITH_OTA` in build flags.
- Run native tests: `run_tests` with environment `native`.

## Line-Item Instructions

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp`** (line 1517–1518)
  Fix the lambda capture context. The `ota_get_status` MCP tool lambda at line 1512 uses `[](void* context, ...)` (non-capturing) and casts `context` to `OTAUpdateManager* self` at line 1513. But lines 1517–1518 reference `_lastCheckTimer.getLastFire()` without the `self->` prefix.

  **Change:**
  ```cpp
  // FROM (line 1517-1518):
  result["last_check_ms"] = (_lastCheckTimer.getLastFire() > 0)
                           ? static_cast<long>(millis() - _lastCheckTimer.getLastFire())
  // TO:
  result["last_check_ms"] = (self->_lastCheckTimer.getLastFire() > 0)
                           ? static_cast<long>(millis() - self->_lastCheckTimer.getLastFire())
  ```

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (line 926–928)
  Fix the non-static method call. `OTAUpdateManager::getState()` is invoked as if static, but it's a non-static member. The `_serviceHelper.setOtaPredicate()` lambda needs a reference to the OTA manager instance.

  **Change:** The `LVGLDisplayModule` needs access to the OTA manager to call `getState()`. Look at how `_serviceHelper` is already wired — it likely receives a reference during `begin()`. Capture a pointer to the OTA manager and call `->getState()` on it instead of the static-style call. If no OTA reference exists in the module, add one via the `begin(Application&)` path (Application provides access to registered modules).

  ```cpp
  // FROM (line 926-928):
  _serviceHelper.setOtaPredicate([]() {
      return OTAUpdateManager::getState() != OTAState::Idle;
  });
  // TO (capture OTA manager pointer):
  auto* otaManager = &app.getModule<OTAUpdateManager>();  // or however the OTA ref is obtained
  _serviceHelper.setOtaPredicate([otaManager]() {
      return otaManager->getState() != OTAState::Idle;
  });
  ```
  Investigate how other modules obtain the OTA manager reference and follow that pattern.

## Verification Requirement
1. `pio run -e crowpanel-128-rotary` with `-D WITH_OTA` — must compile cleanly.
2. Native test suite must still pass (`pio test -e native`).
3. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
_(pending)_

### Changes Made
_(pending)_

### Decisions Made
_(pending)_

### Dead Ends & Warnings
_(pending)_

### Out-of-Scope Findings
_(pending)_

### Blockers
_(pending)_
