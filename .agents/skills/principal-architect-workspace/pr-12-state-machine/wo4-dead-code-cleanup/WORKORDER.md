---
name: pr-12-wo4
description: Handoff instructions for Work Order 4 — Dead Code Cleanup
---

# WO4 — Dead Code Cleanup (ConfigManager Reload Scaffolding)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove the unused `requestReload()`, `checkAndClearReload()`, and `_reloadPending` scaffolding from ConfigManager. These methods have zero production callers — they are dead code that adds unnecessary API surface.

## Strict Boundary Box

- ✅ You MAY modify: `modules/configManager/configManager.hpp`, `modules/configManager/configManager.cpp`, `test/test_unit/test_configManager/test_configManager.cpp`
- ❌ You MUST NOT modify any other files
- ❌ You MUST NOT add new functionality — this is a removal-only operation

## Line-Item Checklist

- [ ] Read `modules/configManager/configManager.hpp` to locate the reload methods and member
- [ ] Read `modules/configManager/configManager.cpp` to locate the `checkAndClearReload()` implementation
- [ ] Read `test/test_unit/test_configManager/test_configManager.cpp` to locate the reload test

### configManager.hpp changes:
- [ ] Remove the "Reload Mechanism" section (lines ~117-131): `requestReload()` inline method and `checkAndClearReload()` declaration
- [ ] Remove `bool _reloadPending = false;` from the member variables section (line ~194)
- [ ] Remove the two lines from the module header doc block that reference `requestReload()` and `checkAndClearReload()` (lines ~26-27)

### configManager.cpp changes:
- [ ] Remove the entire "Reload Mechanism" section (lines ~329-341): the `checkAndClearReload()` implementation and its section header comment

### test_configManager.cpp changes:
- [ ] Remove the "Test Group 6: Reload Mechanism" section (lines ~384-395): the `RequestReload_SetsAndClearsFlag` test
- [ ] Update the module header description to remove mention of "reload mechanism"

### Verification:
- [ ] Run `pio test -e native --filter test_configManager` — all remaining tests pass
- [ ] Verify no other files in the codebase reference `requestReload`, `checkAndClearReload`, or `_reloadPending` (use grep to confirm)

## Verification Requirement

After completing all items, run the test suite to verify no regressions. Then **STOP and send a message back to the Principal Architect** reporting:
1. What files were modified
2. Whether tests pass
3. Confirmation that grep shows zero remaining references

Do NOT wrap up, distill, or commit until the Principal Architect explicitly approves.
