---
name: pr-25-wo3
description: Handoff instructions for Work Order 3 — Template expansion (all extension points) + CONTRIBUTING.md fix
complexity: standard
---

# WO3 — Template Expansion + CONTRIBUTING.md Fix

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

---

## Step 0 — Mandatory Rules Load (DO THIS FIRST)

**Before doing anything else**, read every file inside `.agents/rules/` in
filename order using file-read tools. These are unconditional operational
constraints. Do not skip this step. Do not assume they were auto-loaded.

---

## Prerequisite Context

WO1 and WO2 have already landed. This means:
- `app.registerModule(&myModule)` is the primary registration API
- `ITickable::tick(uint32_t currentMillis)` is the interface signature
- EventBus uses `std::vector` and modules declare their own topic constants

Read these files to understand the current API before making changes:
- `modules/Application/Application.hpp` — for `registerModule<T>()` template
- `include/interfaces/ITickable.hpp` — for `tick(uint32_t currentMillis)` signature
- `include/interfaces/IConfigurable.hpp` — for the config interface contract
- `include/interfaces/IMCPContributor.hpp` — for the MCP interface contract
- `include/interfaces/IConfigGate.hpp` — for the config gate contract (read the
  worked example in the header comments — it's the best documentation in the codebase)
- `modules/Application/Application.cpp` — for the current `begin()` registration
  pattern (to match style)

---

## Objective

Expand the template app (`template/`) to demonstrate ALL primary extension points,
and fix the include path in `CONTRIBUTING.md`. The template is the definitive
worked example — the docs (WO4) will walk through it line by line.

---

## Strict Boundary Box

- **DO** modify: `template/src/myApp.hpp`, `template/src/myApp.cpp`,
  `template/src/main.cpp`, `CONTRIBUTING.md`
- **DO** create: `template/data/www/myapp/tab.html`, `template/data/www/myapp/tab.js`
- **DO NOT** modify any files in `modules/`, `include/interfaces/`, `lib/`, or `src/`.
- **DO NOT** modify any files in `docs/` or `.agents/`.
- **DO NOT** create branches or make git commits — the PA handles branching.

---

## Line-Item Checklist

### Part A — Expand `myApp.hpp`

- [ ] Open `template/src/myApp.hpp`
- [ ] Read the current file to understand the existing structure
- [ ] Update the class declaration to implement ALL of these interfaces:
  ```cpp
  class MyApp : public ITickable,
                public IConfigurable,
                public IWebContributor,
                public IMCPContributor,
                public IConfigGate {
  ```
- [ ] Add the required `#include` statements for any new interfaces.
  Use the pattern already in the file for existing includes.
- [ ] Add the following public method declarations:

  **ITickable** (already exists — just verify signature matches):
  ```cpp
  void tick(uint32_t currentMillis) override;
  ```

  **IConfigurable** (NEW — add all five methods):
  ```cpp
  // IConfigurable
  const char* getConfigNamespace() const override;
  int getSchemaVersion() const override;
  void applyConfig(const JsonObject& config) override;
  void serializeConfig(JsonObject& config) const override;
  void resetConfig() override;
  ```

  **IWebContributor** (already exists — verify):
  ```cpp
  // IWebContributor
  void registerRoutes(AsyncWebServer& server) override;
  const char* getTabName() const override;
  int getTabOrder() const override;
  ```

  **IMCPContributor** (NEW):
  ```cpp
  // IMCPContributor
  void registerMcpTools(MCPServer& mcp) override;
  ```

  **IConfigGate** (NEW):
  ```cpp
  // IConfigGate
  const char* gateName() const override;
  bool isOpen() const override;
  ```

- [ ] Add private member variables to support the config and gate examples:
  ```cpp
  private:
      // Configuration state (persisted via IConfigurable)
      int _refreshIntervalMs = 5000;     // Example configurable parameter
      bool _featureEnabled = true;        // Example boolean toggle

      // Config gate state
      bool _gateOpen = false;

      // Tick timing
      uint32_t _lastActionMs = 0;
  ```

- [ ] Add the `#include` for ArduinoJson if implementing IConfigurable:
  ```cpp
  #include <ArduinoJson.h>
  ```

### Part B — Expand `myApp.cpp`

- [ ] Open `template/src/myApp.cpp`
- [ ] Read the current file to understand the existing implementation
- [ ] Update `tick(uint32_t currentMillis)` to accept the parameter and use it:
  ```cpp
  void MyApp::tick(uint32_t currentMillis) {
      // Example: perform an action every _refreshIntervalMs
      if (currentMillis - _lastActionMs >= (uint32_t)_refreshIntervalMs) {
          _lastActionMs = currentMillis;
          // Your periodic logic here
      }
  }
  ```

- [ ] Implement the IConfigurable methods:
  ```cpp
  const char* MyApp::getConfigNamespace() const {
      return "myapp";  // Config stored at /mod/myapp.json
  }

  int MyApp::getSchemaVersion() const {
      return 1;
  }

  void MyApp::applyConfig(const JsonObject& config) {
      _refreshIntervalMs = config["refreshMs"] | 5000;
      _featureEnabled = config["enabled"] | true;
  }

  void MyApp::serializeConfig(JsonObject& config) const {
      config["refreshMs"] = _refreshIntervalMs;
      config["enabled"] = _featureEnabled;
  }

  void MyApp::resetConfig() {
      _refreshIntervalMs = 5000;
      _featureEnabled = true;
  }
  ```

- [ ] Implement the IMCPContributor method:
  ```cpp
  void MyApp::registerMcpTools(MCPServer& mcp) {
      // Example: register one MCP tool that returns app status
      // Adapt this to the MCPServer::registerTool() API — read the
      // MCPServer header to see the exact signature.
  }
  ```
  **IMPORTANT**: Read `modules/MCPServer/MCPServer.hpp` to find the exact
  `registerTool()` API. Match the signature and callback pattern used by
  existing modules (e.g., the display module's MCP registration).

- [ ] Implement the IConfigGate methods:
  ```cpp
  const char* MyApp::gateName() const {
      return "MyApp Ready";
  }

  bool MyApp::isOpen() const {
      return _gateOpen;
  }
  ```
  Add logic in `tick()` to set `_gateOpen = true` after a condition is met
  (e.g., after the first successful tick cycle, or unconditionally for the
  template). The purpose is to demonstrate the gate pattern.

- [ ] Preserve ALL existing code comments and documentation. Add house-style
  comments to new methods following the pattern in the existing file.

### Part C — Update `main.cpp` to Use `registerModule()`

- [ ] Open `template/src/main.cpp`
- [ ] Replace the existing individual `registerTickable()` / `registerWebContributor()`
  calls with a single `registerModule()` call:
  ```cpp
  app.registerModule(&myApp);  // Auto-registers for tick, config, web, MCP, config gate
  ```
- [ ] Remove any other registration calls that `registerModule()` now handles.
- [ ] Keep the `FrameworkConfig` setup and `app.begin(config)` call as-is.
- [ ] Update comments to explain what `registerModule()` does.

### Part D — Create Web Portal Tab Fragment

- [ ] Create directory `template/data/www/myapp/`
- [ ] Create `template/data/www/myapp/tab.html` with a skeleton tab fragment:
  ```html
  <!-- MyApp tab content fragment -->
  <!-- Loaded by the SPA shell (app.js) when the user clicks the MyApp tab -->
  <div class="tab-content" id="myapp-tab">
      <h2>My Application</h2>
      <div class="card">
          <h3>Status</h3>
          <div id="myapp-status">Loading...</div>
      </div>
      <div class="card">
          <h3>Configuration</h3>
          <label>
              Refresh Interval (ms):
              <input type="number" id="myapp-refresh-ms" value="5000" min="1000" max="60000">
          </label>
          <label>
              <input type="checkbox" id="myapp-enabled" checked>
              Feature Enabled
          </label>
          <button id="myapp-save-btn">Save</button>
      </div>
  </div>
  ```

- [ ] Create `template/data/www/myapp/tab.js` with a skeleton JS file:
  ```javascript
  // MyApp tab JavaScript
  // Fetched and executed by the SPA shell when the tab is activated.

  (function() {
      'use strict';

      // Fetch current status from the app's API endpoint
      async function loadStatus() {
          try {
              const response = await fetch('/api/myapp/status');
              if (!response.ok) throw new Error('Failed to fetch status');
              const data = await response.json();
              document.getElementById('myapp-status').textContent =
                  JSON.stringify(data, null, 2);
          } catch (err) {
              document.getElementById('myapp-status').textContent =
                  'Error: ' + err.message;
          }
      }

      // Save configuration
      async function saveConfig() {
          const refreshMs = document.getElementById('myapp-refresh-ms').value;
          const enabled = document.getElementById('myapp-enabled').checked;
          try {
              await fetch('/api/myapp/config', {
                  method: 'POST',
                  headers: { 'Content-Type': 'application/json' },
                  body: JSON.stringify({ refreshMs: parseInt(refreshMs), enabled })
              });
              alert('Configuration saved');
          } catch (err) {
              alert('Save failed: ' + err.message);
          }
      }

      // Wire up event listeners
      document.getElementById('myapp-save-btn')
          ?.addEventListener('click', saveConfig);

      // Initial load
      loadStatus();
  })();
  ```

### Part E — Fix CONTRIBUTING.md

- [ ] Open `CONTRIBUTING.md`
- [ ] Find the include path `#include "application/Application.hpp"` and replace
  it with `#include "AppFoundation.h"`
- [ ] Update the registration example code to use `registerModule()`:
  ```cpp
  #include "AppFoundation.h"
  #include "MyLogicModule.hpp"

  Application app;
  MyLogicModule myLogic;

  void setup() {
      app.registerModule(&myLogic);

      FrameworkConfig config;
      // populate config ...
      app.begin(config);
  }

  void loop() {
      app.tick();
  }
  ```
- [ ] Update any other examples to use the new `tick(uint32_t currentMillis)`
  signature. The `ITickable` example class should show:
  ```cpp
  class MyLogicModule : public ITickable {
  public:
      void tick(uint32_t currentMillis) override {
          // Non-blocking loop logic here
      }
  };
  ```
- [ ] Add a brief mention of `IConfigurable` in the Interface list section
  (it's currently listed but the example only shows ITickable).
- [ ] Preserve all existing content that isn't being corrected.

### Part F — Build Verification

Use the PlatformIO MCP server tools — do NOT use `pio` CLI commands directly.
The project directory is `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.

- [ ] Build for `native` environment using MCP:
  ```
  call_mcp_tool(ServerName="platformio", ToolName="build_project",
    Arguments={"projectDir": "/Users/mcneillm/Documents/Projects/esp32-app-foundation", "environment": "native"})
  ```
- [ ] Run native unit tests using MCP:
  ```
  call_mcp_tool(ServerName="platformio", ToolName="run_tests",
    Arguments={"projectDir": "/Users/mcneillm/Documents/Projects/esp32-app-foundation", "environment": "native"})
  ```
- [ ] Build for `esp32dev` environment using MCP:
  ```
  call_mcp_tool(ServerName="platformio", ToolName="build_project",
    Arguments={"projectDir": "/Users/mcneillm/Documents/Projects/esp32-app-foundation", "environment": "esp32dev"})
  ```
- [ ] ALL builds and tests must pass with zero errors.

---

## Verification Requirement

After completing all checklist items, verify:
1. Native build compiles cleanly
2. All native tests pass
3. esp32dev build compiles cleanly
4. `template/src/main.cpp` uses `app.registerModule(&myApp)` — NOT individual
   `registerTickable()`/`registerWebContributor()` calls
5. `CONTRIBUTING.md` uses `#include "AppFoundation.h"` — NOT `"application/Application.hpp"`
6. Template `myApp` implements all five interfaces listed above

**STOP** after verification and send a message back to the Principal Architect
reporting your results. Do not distill or commit until the PA approves.
