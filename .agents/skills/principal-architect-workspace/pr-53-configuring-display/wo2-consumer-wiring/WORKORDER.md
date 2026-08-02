---
name: configuring-display-wo2
description: Handoff instructions for Work Order 2 — Consumer Wiring (displayModule.begin + gate binding + initial state)
complexity: standard
---

# WO2 — Consumer Wiring

## Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

## Objective

Wire the display module to the Application lifecycle in all three `main.cpp`
consumer files: call `displayModule.begin()` to subscribe to events, bind the
gate list for `ConfiguringStatusBoard`, and handle the missed initial
`CONFIGURING` event that fires before subscription.

## Parent Project

`pr-53-configuring-display` — GitHub Issue [#60](https://github.com/matthewmcneill/esp32-app-foundation/issues/60).
Human approval of the decomposition is recorded in `PROJECT.md` status log.

## Strict Boundary Box

You may **ONLY** modify these files:

- `src/main.cpp`
- `sample-app/src/main.cpp`
- `template-app/src/main.cpp`

You **MUST NOT** touch:

- Any framework module file (those were WO1's scope)
- Any branch management — you are on `configuring-display/wo2-consumer-wiring`
  already
- Any other header, test, or configuration file
- Any architectural decisions beyond what is specified here

## Tooling Constraints

- Use the `platformio` MCP server tools for all build/compile operations:
  - `build_project` for compilation
- Do NOT use raw `pio` CLI commands

## Context: Current Display Setup Block

All three `main.cpp` files have an identical pattern in the display setup block
(after `app.begin()` returns):

```cpp
#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    displayModule.setConfigManager(&app.getConfigManager());
#ifdef WITH_DISPLAY_U8G2
    diagnosticsBoard.configure(&app.getWifiManager());
    displayModule.carousel.add(&diagnosticsBoard);
    // ... (sample-app has extra weatherBoard lines)
#ifdef WITH_MCP
    mcpServer.setDisplayInfo(displayModule.getDisplayInfo());
#endif
#endif
#endif
```

The changes below must be inserted into this existing block in each file.

## Line-Item Instructions

### For each of the three `main.cpp` files, apply the following changes:

#### Step 1: Add `displayModule.begin()` call

Inside the `#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)` block,
**immediately after** `displayModule.setConfigManager(&app.getConfigManager());`,
add:

```cpp
    displayModule.begin(&app.getEventBus());
```

This subscribes the display module to `APP_CONFIGURING` and `APP_RUNNING` events.

- [ ] `src/main.cpp` — add `displayModule.begin(&app.getEventBus());`
- [ ] `sample-app/src/main.cpp` — add `displayModule.begin(&app.getEventBus());`
- [ ] `template-app/src/main.cpp` — add `displayModule.begin(&app.getEventBus());`

#### Step 2: Bind ConfiguringStatusBoard gate list

Inside the `#ifdef WITH_DISPLAY_U8G2` sub-block, **after** the
`diagnosticsBoard.configure(&app.getWifiManager());` line, add:

```cpp
    displayModule.boards.configuringStatus.setGates(&app.getConfigGates());
```

This passes a pointer to the Application's gate vector. The board reads it each
frame to render gate-by-gate progress.

**API:** `void setGates(const std::vector<IConfigGate*>* gates)` — takes a
**pointer** to the vector (note: `&app.getConfigGates()` takes address of the
returned const reference).

- [ ] `src/main.cpp` — add `setGates()` call
- [ ] `sample-app/src/main.cpp` — add `setGates()` call
- [ ] `template-app/src/main.cpp` — add `setGates()` call

#### Step 3: Handle missed initial CONFIGURING event

After the carousel `.add()` calls (and after any app-specific board wiring like
weatherBoard), but still inside the `#ifdef WITH_DISPLAY_U8G2` block and
**before** the `#ifdef WITH_MCP` block, add the initial state check:

```cpp
    // Handle missed initial CONFIGURING event (fires before subscription)
    if (app.getState() == AppState::Configuring) {
        displayModule.carousel.suspend();
        displayModule.showBase(&displayModule.boards.configuringStatus);
        if (app.getWifiManager().getAPMode()) {
            displayModule.boards.wifiWizard.configure(
                app.getWifiManager().getHostname(), "192.168.4.1");
            displayModule.showModal(&displayModule.boards.wifiWizard);
        }
    }
```

You will need to add `#include "AppState.hpp"` at the top of each file if not
already included (check — `AppState` may already be transitively included via
`Application.hpp`).

- [ ] `src/main.cpp` — add initial state check
- [ ] `sample-app/src/main.cpp` — add initial state check
- [ ] `template-app/src/main.cpp` — add initial state check

### Final state of the display block (reference — `src/main.cpp`):

```cpp
#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)
    displayModule.setConfigManager(&app.getConfigManager());
    displayModule.begin(&app.getEventBus());                              // NEW
#ifdef WITH_DISPLAY_U8G2
    diagnosticsBoard.configure(&app.getWifiManager());
    displayModule.boards.configuringStatus.setGates(&app.getConfigGates()); // NEW
    displayModule.carousel.add(&diagnosticsBoard);

    // Handle missed initial CONFIGURING event (fires before subscription)     // NEW
    if (app.getState() == AppState::Configuring) {                             // NEW
        displayModule.carousel.suspend();                                      // NEW
        displayModule.showBase(&displayModule.boards.configuringStatus);       // NEW
        if (app.getWifiManager().getAPMode()) {                                // NEW
            displayModule.boards.wifiWizard.configure(                         // NEW
                app.getWifiManager().getHostname(), "192.168.4.1");            // NEW
            displayModule.showModal(&displayModule.boards.wifiWizard);         // NEW
        }                                                                      // NEW
    }                                                                          // NEW

#ifdef WITH_MCP
    mcpServer.setDisplayInfo(displayModule.getDisplayInfo());
#endif
#endif
#endif
```

For `sample-app/src/main.cpp`, the initial state check goes **after** the
`weatherBoard.wireEventBus(app.getEventBus());` line and before `#ifdef WITH_MCP`.

## Verification Requirement

1. Build using `platformio` MCP `build_project` for the `esp32s3nano` environment
2. Build using `platformio` MCP `build_project` for the `native` environment
3. If either build fails, diagnose and fix within the boundary box
4. **CRITICAL:** Before distilling or committing, you MUST ask the user if they
   are happy. Only after approval should you invoke the `@distillery` skill.
