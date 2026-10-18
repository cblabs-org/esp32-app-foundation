---
name: configuring-display-wo1
description: Handoff instructions for Work Order 1 — Framework Event Plumbing (payload hostname + WiFi wizard modal)
complexity: standard
---

# WO1 — Framework Event Plumbing

## Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

## Objective

Add the device hostname to the `AppConfiguringPayload` struct, populate it in
`Application::_publishConfiguringEvent()`, and complete the WiFi wizard modal
logic in `U8G2DisplayModule::_onAppConfiguring()` so the display shows the
WiFi setup instructions when the blocking gate is "WiFi Station".

## Parent Project

`pr-53-configuring-display` — GitHub Issue [#60](https://github.com/matthewmcneill/esp32-app-foundation/issues/60).
Human approval of the decomposition is recorded in `PROJECT.md` status log.

## Strict Boundary Box

You may **ONLY** modify these files:

- `include/events/AppConfiguringPayload.hpp`
- `modules/Application/Application.cpp`
- `modules/U8G2Display/U8G2DisplayModule.cpp`

You **MUST NOT** touch:

- Any `main.cpp` file (that is WO2's scope)
- Any branch management — you are on `configuring-display/wo1-event-plumbing`
  already
- Any other module, header, or test file outside the boundary box
- Any architectural decisions beyond what is specified here

## Tooling Constraints

- Use the `platformio` MCP server tools for all build/compile operations:
  - `build_project` for compilation
- Do NOT use raw `pio` CLI commands

## Line-Item Instructions

### 1. Add `hostname` field to `AppConfiguringPayload`

**File:** `include/events/AppConfiguringPayload.hpp`

Add a `hostname` field to the struct. The struct currently has:
```cpp
struct AppConfiguringPayload {
    const char* gateName = nullptr;
    const char* helpText = nullptr;
    bool isRegression = false;
};
```

Add after `isRegression`:
```cpp
    const char* hostname = nullptr;   ///< Device hostname for WiFi wizard display (AP SSID)
```

- [x] Add the `hostname` field with appropriate doc comment
- [x] Preserve all existing comments and docstrings

### 2. Populate `hostname` in `_publishConfiguringEvent()`

**File:** `modules/Application/Application.cpp`

In `_publishConfiguringEvent()` (around L636-668), after the line that sets
`_configuringPayload.isRegression = isRegression;`, add:

```cpp
_configuringPayload.hostname = _wifiManager.getHostname();
```

This uses the `_wifiManager` member (Application.hpp L516) and `getHostname()`
(WifiManager.hpp L307, returns `const char*`).

- [x] Add hostname population line
- [x] Verify it compiles

### 3. Complete WiFi wizard modal in `_onAppConfiguring()`

**File:** `modules/U8G2Display/U8G2DisplayModule.cpp`

The current handler (around L1172-1184) ends with:
```cpp
    self->showBase(cfgBoard);
    // TODO: Restore WiFi Wizard modal logic
```

Replace the TODO comment with logic to push the WiFi wizard modal when the
blocking gate is WiFi. The handler should:

1. Check if `payload` is not null
2. Cast payload to `const AppConfiguringPayload*`
3. Check if `gateName` is "WiFi Station" using `strcmp`
4. If so, resolve the WiFi wizard board — check `IU8G2SystemScreens::wifiWizardBoard()`
   first (same pattern as the configuring board resolution above), fall back to
   the built-in `self->boards.wifiWizard`
5. Call `configure(p->hostname, "192.168.4.1")` on the resolved board (only if
   it's the built-in `WiFiWizardBoard` — a custom board may not have `configure()`)
6. Then call `self->showModal(wizardBoard)`

You will need to add `#include "events/AppConfiguringPayload.hpp"` if not
already included. Also add `#include <cstring>` for `strcmp` if not present.

**IMPORTANT — Extensibility pattern:** The existing code already checks
`_screens->configuringBoard()` for the base board. You MUST mirror that same
pattern for the WiFi wizard modal. This ensures downstream apps that implement
`IU8G2SystemScreens::wifiWizardBoard()` get their custom board used.

Reference implementation:
```cpp
    self->showBase(cfgBoard);

    // Push WiFi wizard modal if the blocking gate is WiFi
    if (payload != nullptr) {
        auto* p = static_cast<const AppConfiguringPayload*>(payload);
        if (p->gateName != nullptr && strcmp(p->gateName, "WiFi Station") == 0) {
            // Resolve wizard board: prefer IU8G2SystemScreens override, fall back to built-in
            IDisplayBoard* wizardBoard = (self->_screens != nullptr && self->_screens->wifiWizardBoard() != nullptr)
                                         ? self->_screens->wifiWizardBoard()
                                         : &self->boards.wifiWizard;

            // Configure built-in board with hostname + IP (custom boards manage their own state)
            if (wizardBoard == &self->boards.wifiWizard) {
                self->boards.wifiWizard.configure(p->hostname, "192.168.4.1");
            }
            self->showModal(wizardBoard);
        }
```

- [x] Remove the TODO comment
- [x] Add the WiFi wizard modal logic
- [x] Add necessary includes (`AppConfiguringPayload.hpp`, `<cstring>`)
- [x] Ensure the `payload` parameter name is restored (currently `/*payload*/`)

## Verification Requirement

1. Build using `platformio` MCP `build_project` for the `esp32s3nano` environment
2. Build using `platformio` MCP `build_project` for the `native` environment
3. If either build fails, diagnose and fix within the boundary box
4. **CRITICAL:** Before distilling or committing, you MUST ask the user if they
   are happy. Only after approval should you invoke the `@distillery` skill.

---

## Status Update

| Date | Status | Details |
|------|--------|---------|
| 2026-08-02 | ✅ Complete & Approved | Implementation finished, verified with `esp32s3nano` build and native test suite (368/368 passed). User approval received. Distillation package generated at `.agents/skills/principal-architect-workspace/pr-53-configuring-display/wo1-event-plumbing/`. |

