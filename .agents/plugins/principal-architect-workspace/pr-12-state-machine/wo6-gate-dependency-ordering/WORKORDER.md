---
name: pr-12-wo6
description: Handoff instructions for Work Order 6 — Config Gate Dependency Ordering
---

# WO6 — Config Gate Dependency Ordering

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory.

## Objective

Add dependency-aware ordering to config gates so that the CONFIGURING phase presents gates in the correct sequence (e.g., "Connect WiFi" before "Set Timezone"). The ConfiguringStatusBoard shows only actionable gates — those whose own dependencies are already satisfied.

## Branch

```bash
git checkout pr-12-wo6-gate-deps
```

## Strict Boundary Box

- ✅ You MAY modify: `include/interfaces/IConfigGate.hpp`, `modules/application/Application.cpp`, `modules/u8g2Display/boards/configuringStatusBoard.cpp`, test files
- ❌ You MUST NOT modify: `AppState.hpp`, `DataManager`, `ConfigManager`, `WifiManager`

## Detailed Changes

### 1. IConfigGate.hpp — Add getDependencies()

Add this virtual method to the interface:

```cpp
/// Return the names of gates this gate depends on.
/// Default: no dependencies.
/// Example: a "Timezone" gate depends on "WiFi Station".
virtual const char* const* getDependencies(uint8_t& count) const {
    count = 0;
    return nullptr;
}
```

### 2. Application.cpp — Dependency-aware gate publishing

Update `_publishConfiguringEvent()` to find the first **actionable** gate (all deps satisfied, gate itself unsatisfied) rather than just the first unsatisfied gate:

```cpp
void Application::_publishConfiguringEvent(bool isRegression) {
    for (const auto* gate : _configGates) {
        if (gate->isSatisfied()) continue;
        
        // Check if all dependencies are satisfied
        uint8_t depCount = 0;
        const char* const* deps = gate->getDependencies(depCount);
        bool depsOk = true;
        for (uint8_t d = 0; d < depCount; d++) {
            // Find the dependency gate by name and check if satisfied
            bool found = false;
            for (const auto* other : _configGates) {
                if (strcmp(other->getGateName(), deps[d]) == 0) {
                    if (!other->isSatisfied()) depsOk = false;
                    found = true;
                    break;
                }
            }
            if (!found || !depsOk) { depsOk = false; break; }
        }
        
        if (!depsOk) continue; // Skip — deps not met
        
        // This gate is actionable
        AppConfiguringPayload payload;
        payload.gateName = gate->getGateName();
        payload.helpText = gate->getHelpText();
        payload.isRegression = isRegression;
        _eventBus.publish("app.configuring", &payload);
        ESP_LOGI(TAG, "Config gate unsatisfied: %s%s", gate->getGateName(),
                 isRegression ? " (regression)" : "");
        return;
    }
}
```

### 3. ConfiguringStatusBoard — Dependency-aware rendering

Update the board's `draw()` method to render gates grouped by dependency state:
- **Satisfied** gates at top (✓ icon)
- **Actionable** gates in middle (▶/⟳ icon) — unsatisfied but all deps met
- **Blocked** gates at bottom (○ icon) — unsatisfied with unmet deps

The board already has access to the gate list. Add dependency checking logic similar to what's in Application.cpp.

### 4. Tests

Add test cases for dependency ordering:
- Gate with unmet dependency is skipped (not published as the actionable gate)
- Gate with all deps satisfied is actionable
- Gate with no dependencies declared is always actionable
- Multiple chained dependencies resolve correctly

## Line-Item Checklist

- [ ] Read `include/interfaces/IConfigGate.hpp`
- [ ] Read `modules/application/Application.cpp` — focus on `_publishConfiguringEvent()`
- [ ] Read `modules/u8g2Display/boards/configuringStatusBoard.cpp` — understand rendering
- [ ] Modify `IConfigGate.hpp` — add `getDependencies()`
- [ ] Modify `Application.cpp` — dependency-aware gate publishing
- [ ] Modify `configuringStatusBoard.cpp` — dependency-aware rendering
- [ ] Add tests for dependency ordering logic
- [ ] Verify: `pio test -e native --filter "test_unit/test_application"` — all tests pass
- [ ] Verify: `pio test -e native --filter "test_unit/test_u8g2Display"` — all tests pass

## Verification Requirement

After completing all items, run tests. Then **STOP and send a message back to me** reporting results. Do NOT commit until I explicitly approve.
