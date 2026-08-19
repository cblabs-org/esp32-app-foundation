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
✅ **Complete** — committed as `242aab11` on `issue-sweep-aug/wo1-ota-compilation-fixes`.

### Changes Made

#### `modules/OTAUpdateManager/OTAUpdateManager.cpp`
1. **Lambda `self->` prefix fix (lines 1517–1518):** Added `self->` to both `_lastCheckTimer.getLastFire()` calls in the `ota_get_status` MCP tool lambda. The lambda uses a void-context pattern (`[](void* context, ...)`) and casts to `OTAUpdateManager* self`, but these two accesses were missing the `self->` prefix.
2. **Pre-include workaround for `Periodic.hpp`:** Moved `#include "utils/Periodic.hpp"` before `#include "OTAUpdateManager.hpp"` to fix a pre-existing missing-include bug — the header declares `Periodic _lastCheckTimer` but never includes the Periodic header. Removed the now-duplicate include from its original location.

#### `modules/LVGLDisplay/LVGLDisplayModule.cpp`
1. **Non-static `getState()` fix (lines 926–928):** Replaced the invalid static-style call `OTAUpdateManager::getState() != OTAState::Idle` with `_pauseRendering.load()`, using the module's own `std::atomic<bool>` member that is already set during OTA progress events. Captures `this` in the lambda.
2. **Removed dead `#include "OTAUpdateManager.hpp"`** — no longer referenced after the predicate fix.

### Decisions Made
- **`_pauseRendering` over `getState()`:** The boundary box prohibits `.hpp` changes, so no `_otaManager` member could be added. The module's `_pauseRendering` atomic flag serves the identical purpose (gating screenshots during OTA) and is already maintained by the EventBus `_onOtaProgress` subscription. This is arguably cleaner than a cross-module pointer dependency.

### Dead Ends & Warnings
- Initially attempted to find an `Application&` path to obtain the OTA manager reference, as suggested in the work order. The module's `begin()` takes `EventBus*` (not `Application&`), `registerRoutes()` receives only `PsychicHttpServer&`, and there is no `getModule<T>()` template. All paths require header modification, which the boundary box prohibits.

### Out-of-Scope Findings
1. **`OTAUpdateManager.hpp` missing `#include "utils/Periodic.hpp"`:** The header declares `Periodic _lastCheckTimer` (line 401) but doesn't include the header. Introduced by commit `9b08d23a` ("migrate module throttles to Periodic helper"). The `.cpp` workaround is sufficient but the header should be made self-contained.
2. **`U8G2DisplayModule.cpp:787`** has the identical `OTAUpdateManager::getState()` static-call bug. Same fix pattern applies.
3. **Pre-existing Playwright web test failures (24/69):** Display, MCP, and OTA tab tests fail — unrelated to this WO.

### Blockers
None.

### Verification
- ✅ `pio run -e crowpanel-128-rotary` with `-D WITH_OTA` — compiled cleanly (RAM 16.4%, Flash 45.5%)
- ✅ `pio test -e native` — 748/748 test cases passed

