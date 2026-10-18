---
name: pr-66-wo4
description: Handoff instructions for Work Order 4 — Consumer cleanup
complexity: mechanical
---

# WO4: Consumer Cleanup — Delete Catch-Up Blocks + AP Subscriptions

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Delete the now-superseded post-begin catch-up blocks and `WIFI_AP_STARTED` EventBus subscriptions from all three consumer `main.cpp` files. Add `GateNames::WIFI_STATION` as a dependency on `BoardRegistry` in the departures-board consumer. The framework's gate-transition re-publishing (WO1) and display module dispatch (WO2) make these manual workarounds unnecessary.

## Parent Project

`pr-66-configuring-orchestration` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- ✅ ALLOWED: `src/main.cpp`, `sample-app/src/main.cpp`, `template-app/src/main.cpp`
- ✅ ALLOWED: The departures-board consumer's `BoardRegistry.hpp` (for adding WiFi dependency) — located in the consumer repo at `src/app/modules/BoardRegistry/BoardRegistry.hpp`
- ❌ DO NOT modify any framework module files (Application, WifiManager, U8G2Display, LVGLDisplay)
- ❌ DO NOT modify any interface files

## Important Context

The catch-up blocks and AP subscriptions to delete follow this pattern in ALL THREE `main.cpp` files:

**Pattern 1 — Post-begin catch-up block (~16 lines):**
```cpp
if (app.getState() == AppState::Configuring) {
    displayModule.carousel.suspend();
    displayModule.showBase(&displayModule.boards.configuringStatus);
    for (auto* gate : app.getConfigGates()) {
        if (!gate->isSatisfied() && ...) {
            displayModule.boards.wifiWizard.configure(...);
            displayModule.showModal(&displayModule.boards.wifiWizard);
            break;
        }
    }
}
```

**Pattern 2 — WIFI_AP_STARTED subscription (~6 lines):**
```cpp
app.getEventBus().subscribe(WifiEvents::WIFI_AP_STARTED,
    [](const char*, const void*, void* ctx) {
        auto& c = *static_cast<...>(ctx);
        c.dm->boards.wifiWizard.configure(...);
        c.dm->showModal(&c.dm->boards.wifiWizard);
    }, &callbackContext);
```

**KEEP** the `setGates()` call — `displayModule.boards.configuringStatus.setGates(...)` is still needed for the base board.

## Tooling Constraints

- Use PlatformIO MCP tools (read `.agents/skills/pio-manager/SKILL.md`)
- Verify compilation with: `pio run -e esp32s3nano`
- Run unit tests with: `pio test -e native`

## Checklist

### 1. `src/main.cpp` (framework default app)

- [ ] Delete the `if (app.getState() == AppState::Configuring)` catch-up block (~L154–170)
- [ ] Delete the `WIFI_AP_STARTED` subscription (~L177–183)
- [ ] KEEP the `displayModule.boards.configuringStatus.setGates()` call
- [ ] Verify no dangling references to `wifiWizard` remain outside event-driven code

### 2. `sample-app/src/main.cpp`

- [ ] Delete the `if (app.getState() == AppState::Configuring)` catch-up block (~L163–175)
- [ ] Delete the `WIFI_AP_STARTED` subscription (~L184–190)
- [ ] KEEP the `setGates()` call

### 3. `template-app/src/main.cpp`

- [ ] Delete the `if (app.getState() == AppState::Configuring)` catch-up block (~L152–164)
- [ ] Delete the `WIFI_AP_STARTED` subscription (~L173–179)
- [ ] KEEP the `setGates()` call

### 4. `BoardRegistry.hpp` — Add WiFi Dependency (Consumer Repo)

> [!NOTE]
> This file is in the departures-board CONSUMER repo, not the framework repo.
> Path: `../../src/app/modules/BoardRegistry/BoardRegistry.hpp` (relative to framework root)
> Or absolute: the departures-board-v3 workspace root → `src/app/modules/BoardRegistry/BoardRegistry.hpp`

- [ ] Add WiFi dependency using the gate name constant:
  ```cpp
  #include "interfaces/IConfigGate.hpp"  // for GateNames

  const char* const* getDependencies(uint8_t& count) const override {
      static const char* deps[] = {GateNames::WIFI_STATION};
      count = 1;
      return deps;
  }
  ```

## Verification

1. `pio test -e native` — all tests pass
2. `pio run -e esp32s3nano` — hardware target compiles
3. Verify in each `main.cpp` that no `AppState::Configuring` check or `WIFI_AP_STARTED` subscription remains
4. Verify `setGates()` call is preserved in each `main.cpp`

## Completion Protocol

When verified, **STOP and send a message to the PA**. Do NOT distill or commit until approved.
