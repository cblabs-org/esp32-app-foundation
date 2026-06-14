# Contributing to ESP32 App Foundation

Thank you for your interest in contributing! This framework thrives on reusable, cleanly separated modules.

## Module Authoring Guide

All features in the App Foundation should be packaged as distinct **Modules**. We enforce a strict separation between hardware interactions and high-level application logic.

### 1. Identify Your Module Interface
Depending on what your module needs to do, you should implement one or more of the core interfaces from `include/interfaces/`:
- **`ITickable`:** For modules that require periodic execution. Implement `tick()` to execute non-blocking logic. Must complete within 10ms.
- **`iConfigurable`:** For modules that load or persist configuration to LittleFS.
- **`IWebContributor`:** For modules that register REST API endpoints.
- **`IMcpContributor`:** For modules that expose functions via the MCP JSON-RPC protocol.
- **`iDataSource`:** For background data-fetching workers managed by the `DataManager`.
- **`IStatusReporter`:** For display adapters that render boot-phase status overlays.

### 2. Implementing the Interface
Create your class in the appropriate directory (`lib/` for generic features, or your project's local `src/` directory for app-specific logic). 

**Example Tickable Module (`MyLogicModule.hpp`):**
```cpp
#pragma once
#include "interfaces/ITickable.hpp"
#include <Arduino.h>

class MyLogicModule : public ITickable {
public:
    void tick() override {
        // Non-blocking loop logic here
        // E.g., check millis() to perform a task every 1000ms
    }
};
```

### 3. Registration
Modules must be registered with the `Application` instance during the boot phase, BEFORE `app.begin()` is called. 

```cpp
#include "application/Application.hpp"
#include "MyLogicModule.hpp"

Application app;
MyLogicModule myLogic;

void setup() {
    // Register the module so it receives tick() events
    app.registerTickable(&myLogic);
    
    FrameworkConfig config;
    // populate config ...
    app.begin(config);
}

void loop() {
    app.tick();
}
```

### 4. House Style and Standards
- **Non-blocking Code:** Never use `delay()` in a module's `tick()` function. Use timestamp checking (`millis()`) or FreeRTOS timers.
- **Memory Management:** Prefer stack allocation or static allocation. If dynamic allocation is necessary, do it during registration or in `begin()`, and avoid runtime allocation in `tick()` to prevent heap fragmentation.
- **Two-Tier Registration:** Modules are pushed to `std::vector` instances which are subsequently locked (via `shrink_to_fit()`). No registrations are allowed once locked.
- **Documentation:** All modules must include Doxygen-style or house-style comments for their public APIs. Ensure you include a module header with the standard license header.
- **Error Handling:** Avoid halting the entire application when a non-critical module fails.

### 5. Pull Requests
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/my-new-module`).
3. Commit your changes.
4. Push to the branch (`git push origin feature/my-new-module`).
5. Open a Pull Request.

Ensure all unit tests pass and that your code compiles for both `arduino` and `espidf` frameworks before submitting.

## Agent Harness Configuration

Agent context is kept single-source to avoid drift across coding assistants. The
canonical store is `.agents/` (skills, workflows, rules, harness-adapters), and
all harness entry points resolve into it:

- **`AGENTS.md`** — the canonical, harness-neutral bootstrap. Edit this. It is
  read natively by Antigravity (which uses the plural `.agents/` directory) and
  Cursor, and imported by Claude Code.
- **`CLAUDE.md`** — thin stub that imports `AGENTS.md` (`@AGENTS.md`) plus a few
  Claude Code-specific notes. Claude Code also reaches the store via the
  `.claude → .agents` symlink (`.claude/skills`, `.claude/commands`,
  `.claude/rules`).
- **`GEMINI.md`** — thin stub for Antigravity-specific overrides (highest
  precedence in Antigravity) plus a backstop that re-states the mandatory
  rules-load step, since Antigravity's rules-directory auto-load is unreliable.

**Rule of thumb:** put shared content in `AGENTS.md`; only add to `CLAUDE.md` or
`GEMINI.md` when the behaviour is genuinely specific to that harness.
