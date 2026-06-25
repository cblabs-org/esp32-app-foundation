---
name: docs-excellence-wo1
description: Handoff instructions for Work Order 1 — Template-app critical fixes
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Fix all critical house-style violations in the template-app. The template is the first code new developers copy — it must be an exemplar of documentation standards.

# Strict Boundary Box

- **ONLY** modify files under `template-app/`
- Do NOT touch any framework files (`modules/`, `lib/`, `include/`, `src/`)
- Do NOT touch `sample-app/`
- Do NOT create git branches or make commits — work on the current branch
- Do NOT modify any logic or behavior — this is documentation and naming only (except removing the dead `registerMcpTools` method)

# Tooling Constraints

- Use `platformio` MCP tools for builds: `build_project` for compilation verification
- Build the template-app to verify your changes compile: build with project path `template-app/`

# House-Style Reference

Read `.agents/skills/house-style-docs/references/cpp-embedded.md` for the complete documentation standards before starting work.

# Line-Item Instructions

## 1. Rename files (PascalCase alignment rule)

- [ ] Rename `template-app/src/myApp.hpp` → `template-app/src/MyApp.hpp`
- [ ] Rename `template-app/src/myApp.cpp` → `template-app/src/MyApp.cpp`
- [ ] Update `#include "myApp.hpp"` → `#include "MyApp.hpp"` in `template-app/src/main.cpp`
- [ ] Update module path in the header blocks to reflect new filenames

## 2. Remove legacy dead code

- [ ] Remove the `registerMcpTools(MCPServer& mcp)` declaration from `MyApp.hpp` (currently L65 in myApp.hpp)
- [ ] Remove the `registerMcpTools(MCPServer& mcp)` stub implementation from `MyApp.cpp` (currently L157-159 in myApp.cpp)
- [ ] Remove the `class MCPServer;` forward declaration if it's no longer needed after removing the legacy method

## 3. Interface-grouped export list

- [ ] Rewrite the "Exported Classes" section in `MyApp.hpp` header to use the interface-grouped format:
```
 * Exported Classes:
 * - MyApp: Template application module demonstrating 5 primary framework interfaces.
 *
 *   Own API:
 *   - setConfigManager():    Wire the ConfigManager pointer for save operations.
 *
 *   ITickable:
 *   - tick():                Periodic update hook called by the Application hub.
 *
 *   IConfigurable ("myapp"):
 *   - getConfigNamespace():  Returns the config namespace identifier.
 *   - getSchemaVersion():    Returns the current schema version.
 *   - applyConfig():         Loads persisted settings from ConfigManager.
 *   - serializeConfig():     Serializes current configuration to JSON.
 *   - resetConfig():         Restores factory defaults.
 *
 *   IWebContributor:
 *   - registerRoutes():      Registers custom REST endpoints with the web server.
 *   - getTabName():          Returns the UI tab name for this module.
 *   - getTabOrder():         Returns the tab sort order.
 *
 *   IMCPContributor:
 *   - registerMCPTools():    Exposes tools to the MCP server.
 *
 *   IConfigGate:
 *   - getGateName():         Returns the human-readable gate name.
 *   - isSatisfied():         Checks if the gate condition is met.
```

## 4. Doxygen on ALL public methods

Add `/** @brief ... */` (minimum) to every public method declaration in `MyApp.hpp`. For methods with parameters, add `@param` and `@return` as appropriate.

Add Doxygen blocks to every method implementation in `MyApp.cpp` — all 12+ methods currently have zero documentation.

## 5. TODO markers at customization points

Add `// TODO:` comments at these locations:
- [ ] Above the class declaration: `// TODO: Rename this class to match your application`
- [ ] At `getConfigNamespace()` return: `// TODO: Change "myapp" to your app's config namespace`
- [ ] At `getTabName()` return: `// TODO: Change "MyApp" to your tab display name`
- [ ] At `getGateName()` return: `// TODO: Change "MyApp Ready" to your gate name`
- [ ] At MCP tool name in `registerMCPTools()`: `// TODO: Rename "myapp_status" to your tool name`
- [ ] At API route paths in `registerRoutes()`: `// TODO: Change "/api/myapp/..." to your API paths`
- [ ] At app name in `main.cpp`: `// TODO: Change "My Application" to your app name`

## 6. Additional improvements

- [ ] Add commented-out examples for optional `IWebContributor` overrides in `MyApp.hpp`:
  ```cpp
  // Optional overrides — uncomment to customize:
  // const char* getParentTab() const override { return "System"; }
  // const char* getTabIcon() const override { return "⚙️"; }
  // const char* getHelpText() const override { return "Help text for this tab"; }
  ```
- [ ] Add inline comment at `IConfigGate` section explaining: "The framework blocks transition from CONFIGURING to RUNNING state until all gates report isSatisfied() == true. In a real app, check API key readiness, hardware initialization, etc."
- [ ] Replace C-style cast `(uint32_t)_refreshIntervalMs` with `static_cast<uint32_t>(_refreshIntervalMs)` in `MyApp.cpp`
- [ ] Add inline doc comments to global variables in `main.cpp`: `Application app;` → `Application app; ///< Framework hub instance` and similar for `myApp`

## 7. README update

- [ ] Add `data/www/myapp/tab.html` and `data/www/myapp/tab.js` to the file table in `README.md`
- [ ] Add note about `IDataSource` and `IDisplayBoard` interfaces: "The sample-app demonstrates two additional interfaces not shown here..."
- [ ] Note about `sdkconfig.defaults` if referenced by platformio.ini

# Verification

- [ ] Build the template-app: `pio run -e esp32dev` in `template-app/` — must succeed
- [ ] If native env exists, run: `pio test -e native` in `template-app/`
- [ ] Visually verify all files have complete house-style headers
- [ ] **STOP and ask the user if they are happy before distilling**
