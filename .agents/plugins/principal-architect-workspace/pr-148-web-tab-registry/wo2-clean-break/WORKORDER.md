---
name: 148-web-tab-registry-wo2
description: Handoff instructions for Work Order 2 — Complete clean break, all single-tab modules, WebServerManager, test updates
complexity: standard
---

# WO2 — Complete Clean Break: All Modules + WebServerManager

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Complete the `IWebContributor` clean break: remove all scalar tab getters (`getTabName`, `getParentTab`, `getTabOrder`, `getTabIcon`, `getTabScript`), make `registerTabs()` pure virtual, migrate every remaining `IWebContributor` implementor, make `WebServerManager` implement `IWebContributor` for its "Status" tab, refactor the tab collection loop, and update all tests. Zero technical debt — template-app and sample-app ship best-practice `registerTabs()` examples.

## Context
This is WO2 of PR #150, stacked on WO1 (`148-web-tab-registry/wo1-types-multi-tab`). The branch for this WO is `148-web-tab-registry/wo2-clean-break` based on `148-web-tab-registry/wo1-types-multi-tab`.

After WO1:
- `TabRegistry` exists in `include/utils/TabRegistry.hpp`.
- `IWebContributor` has `registerTabs(TabRegistry&)` with a backward-compat default.
- Multi-tab modules (`AuthManager`, `LVGLDisplayModule`, `U8G2DisplayModule`) already override `registerTabs()` and their inner stubs are deleted.
- Single-tab modules still work via the backward-compat default reading scalar getters.

**Key invariant:** `getApiPrefix()` and `getMaxPostBody()` are **RETAINED** on `IWebContributor`. They serve the body-limit middleware, not tab metadata. They are 1:1 per contributor and don't suffer from the multi-registration problem. Do NOT remove them.

## Strict Boundary Box
- **DO NOT** remove `getApiPrefix()` or `getMaxPostBody()` from `IWebContributor`.
- **DO NOT** touch `TabRegistry.hpp` — it is complete from WO1.
- **DO NOT** modify display rendering code, PreconditionEngine, EventBus, or any module logic beyond tab registration.
- **DO NOT** create or modify any branches beyond what was pre-created by the PA.
- **DO NOT** change any tab names, parent relationships, ordering, icons, or scripts — the refactored output must be byte-identical to the pre-refactor `/api/tabs` JSON.

## Tooling Constraints
- Use `platformio` MCP tools (`build_project`, `run_tests`) for build verification — not raw `pio` CLI commands.
- Use `check_project` or `agent_build_diagnose` if build issues arise.
- Commit messages should follow conventional commits: `refactor(web):`, `feat(web):`, etc.

## Line-Item Instructions

### 1. Make `registerTabs()` pure virtual and remove scalar getters
- [ ] Modify `include/interfaces/IWebContributor.hpp`
- **Remove** the default implementation body from `registerTabs()` — make it pure virtual:
  ```cpp
  virtual void registerTabs(TabRegistry& registry) = 0;
  ```
- **Remove** the following scalar getter methods entirely:
  - `getTabName()` — the virtual method declaration and its default `{ return nullptr; }` body
  - `getParentTab()` — same
  - `getTabOrder()` — same (default was `{ return 100; }`)
  - `getTabIcon()` — same
  - `getTabScript()` — same
- **Retain**: `registerRoutes()`, `getApiPrefix()`, `getMaxPostBody()`.
- Update the class Doxygen comment to reflect the new interface shape.

### 2. Migrate framework single-tab modules (7 modules)

For each module below, add a `registerTabs(TabRegistry&)` override. Read the module's existing scalar getters to determine the exact values — they must match precisely. Remove any scalar getter overrides that are now dead code (since the base class methods no longer exist).

- [ ] `modules/ConfigManager/ConfigManager.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Backup", "System", 40);
  }
  ```

- [ ] `modules/KeyManager/KeyManager.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Keys", "System", 20);
  }
  ```

- [ ] `modules/WifiManager/WifiManager.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("WiFi", nullptr, 10, /* existing WiFi SVG icon */);
  }
  ```
  Find the existing WiFi SVG icon string from the current `getTabIcon()` override.

- [ ] `modules/OTAUpdateManager/OTAUpdateManager.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("OTA", "System", 50, nullptr, "/ota.js");
  }
  ```

- [ ] `modules/TimeManager/TimeManager.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Time", "System", 15, nullptr, "/time.js");
  }
  ```

- [ ] `modules/HomeAssistant/HomeAssistantModule.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("HA", nullptr, 40, /* existing HA SVG icon */, "/homeassistant.js");
  }
  ```
  Find the existing HA SVG icon string from the current `getTabIcon()` override.

- [ ] `modules/MCPServer/MCPServer.hpp`
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("MCP", "System", 30, nullptr, "/mcp.js");
  }
  ```

For each module: also remove any now-dead scalar getter overrides (`getTabName()`, `getParentTab()`, `getTabOrder()`, `getTabIcon()`, `getTabScript()`) — they will cause compile errors since the base class no longer declares them.

### 3. Migrate Application internal contributors

- [ ] Modify `modules/Application/Application.hpp`
- **`AppTabContributor`** (line ~527): remove scalar getter overrides, add:
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("App", nullptr, 30);
  }
  ```
- **`PreconditionWebContributor`** (line ~541): this contributor has **no visible tab** — only API routes. Add an empty override:
  ```cpp
  void registerTabs(TabRegistry& /*registry*/) override {}
  ```
  Remove any scalar getter overrides (it currently has none for tab name, so this is mainly adding the required pure virtual override).

### 4. Make `WebServerManager` implement `IWebContributor`

- [ ] Modify `modules/WebServer/WebServerManager.hpp`
- Add `IWebContributor` to the class inheritance list (it may already inherit other interfaces — add to the existing list).
- Add `#include "interfaces/IWebContributor.hpp"` if not already present.
- Declare `registerRoutes()` and `registerTabs()` overrides.
- For `registerRoutes()`: if `WebServerManager` currently registers its system routes (like `/api/system`, `/api/reboot`) inline in `begin()`, you have two options:
  - **Option A (preferred):** Leave system route registration in `begin()` and have `registerRoutes()` be a no-op `{}`. The routes are framework-internal and already registered before contributors.
  - **Option B:** Move system route registration into `registerRoutes()`. Only do this if it's clean — don't break the initialization order.
- For `registerTabs()`:
  ```cpp
  void registerTabs(TabRegistry& registry) override {
      registry.addTab("Status", "System", 10);
  }
  ```

- [ ] Modify `modules/Application/Application.cpp` (`begin()`)
- Ensure `registerModule(&_webServerManager)` auto-detects `IWebContributor` now that `WebServerManager` implements it. The existing `registerModule<T>` template with `std::is_base_of_v<IWebContributor, T>` handles this automatically — no change needed if `registerModule(&_webServerManager)` is already called.

### 5. Refactor `WebServerManager::begin()` tab collection

- [ ] Modify `modules/WebServer/WebServerManager.cpp`
- Create a concrete `TabRegistryImpl` class (can be a private inner class in the `.cpp` or in the `.hpp`):
  ```cpp
  class TabRegistryImpl : public TabRegistry {
  public:
      std::vector<TabEntry> entries;
      void addTab(const char* name, const char* parent, int order,
                  const char* icon, const char* script) override {
          entries.push_back({name, parent, order, icon, script});
      }
  };
  ```
- In `begin()`, replace the tab-collection loop (lines ~213–238) that reads scalar getters:
  ```cpp
  // OLD: for each contributor, read getTabName(), getParentTab(), etc.
  // NEW: for each contributor, call registerTabs(tabRegistry)
  TabRegistryImpl tabRegistry;
  for (auto* contributor : contributors) {
      if (contributor != nullptr) {
          contributor->registerRoutes(*activeServer);
          contributor->registerTabs(tabRegistry);
      }
  }
  // Build JSON from tabRegistry.entries
  for (const auto& tab : tabRegistry.entries) {
      JsonObject tabObj = tabsArr.add<JsonObject>();
      tabObj["name"] = tab.name;
      tabObj["id"]   = tab.name;
      if (tab.parent) tabObj["parent"] = tab.parent;
      tabObj["order"] = tab.order;
      if (tab.icon)   tabObj["icon"]   = tab.icon;
      if (tab.script && tab.script[0] == '/') tabObj["script"] = tab.script;
  }
  ```
- **Remove** the hardcoded "Status" tab JSON injection (line ~280):
  ```cpp
  // DELETE these lines:
  JsonObject statusTab = tabsArr.add<JsonObject>();
  statusTab["name"]   = "Status";
  statusTab["id"]     = "Status";
  statusTab["parent"] = "System";
  statusTab["order"]  = 10;
  ```
  This is now handled by `WebServerManager::registerTabs()`.

### 6. Migrate consumer apps

- [ ] Modify `sample-app/src/WeatherModule.hpp`
  - Remove scalar tab getter overrides.
  - Add `registerTabs()` override:
    ```cpp
    void registerTabs(TabRegistry& registry) override {
        registry.addTab("Weather", nullptr, 50, /* existing cloud SVG icon */, "/weather.js");
    }
    ```
  - Find the existing cloud/weather SVG icon from `getTabIcon()`.

- [ ] Modify `template-app/src/MyApp.hpp`
  - Remove scalar tab getter overrides.
  - Add `registerTabs()` override:
    ```cpp
    void registerTabs(TabRegistry& registry) override {
        registry.addTab("MyApp", nullptr, 50);
    }
    ```

- [ ] Check `sample-app/src/main.cpp` and `template-app/src/main.cpp` — verify there are no `registerModule(xxx.xxxTabStub())` calls. If any exist, remove them.

### 7. Update test mocks and stubs

- [ ] Modify `test/test_unit/test_application/test_application.cpp`
  - Update `MockWebContributor` — remove scalar getter overrides, add `registerTabs()` override.
  - Update `MultiModule` if it implements `IWebContributor` — same treatment.
  - Remove any test cases that test stub registration patterns.
  - Add a test verifying that `registerTabs()` is called and produces correct entries (use a local `TabRegistryImpl` to capture output).

- [ ] Modify `test/test_unit/test_webServer/test_webServer.cpp`
  - Update `MockContributor` — remove scalar getter overrides, add `registerTabs()` override.
  - Update `TabMockContributor` — same treatment.

- [ ] Verify `test/test_unit/test_u8g2Display/u8g2_module_native_stubs.cpp` — ensure no remaining references to deleted scalar getters.
- [ ] Verify `test/test_unit/test_mcpServer/u8g2_module_mcp_stubs.cpp` — same.

### 8. Final build and test verification
- [ ] Run `build_project` for all hardware environments — must compile cleanly.
- [ ] Run `run_tests` for the `native` environment — all tests must pass (expect ≥ 828).
- [ ] Verify the refactored code produces identical tab hierarchy: same tab names, parents, orders, icons, and scripts as before the refactor.

## Verification Requirement
The worker must verify the project compiles and all tests pass using the `platformio` MCP tools. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
✅ **COMPLETE** — All 8 checklist items executed. 31/31 test suites pass (824/824 test cases). Committed as `93706f68`.

### Changes Made
**28 files changed, 450 insertions, 383 deletions.**

1. **`IWebContributor.hpp`**: Made `registerTabs(TabRegistry&)` pure virtual. Removed 5 scalar getters (`getTabName`, `getParentTab`, `getTabOrder`, `getTabIcon`, `getTabScript`). Retained `registerRoutes()`, `getApiPrefix()`, `getMaxPostBody()`.

2. **7 Framework modules** (ConfigManager, KeyManager, WifiManager, OTAUpdateManager, TimeManager, HomeAssistantModule, MCPServer): Replaced scalar getter overrides with `registerTabs()` inline override calling `registry.addTab(...)`.

3. **Application.hpp**: `AppTabContributor` — replaced `getTabName`/`getTabOrder` with `registerTabs()`. `PreconditionWebContributor` — added empty `registerTabs()` override (API-only, no visible tab).

4. **WebServerManager**: Added `IWebContributor` to inheritance chain, declared `registerRoutes()` (no-op) and `registerTabs()` (registers Status sub-pill at order 10).

5. **WebServerManager.cpp**: Created `TabRegistryImpl` local struct to capture tabs into JsonArray. Replaced scalar-getter loop with `contributor->registerTabs(tabRegistry)` calls. Removed hardcoded Status tab injection. Preserved body-limit collection code.

6. **Consumer apps**: WeatherModule (sample-app) and MyApp (template-app) migrated to `registerTabs()`. Removed dead `TabStub()` registration calls from both `sample-app/src/main.cpp` and `template-app/src/main.cpp`.

7. **WO1 multi-tab module cleanup**: AuthManager, LVGLDisplayModule, U8G2DisplayModule — removed dead scalar getter overrides that would fail to compile. Changed `getTabIcon()` from `override` to non-virtual helper (called by their `registerTabs()` implementations).

8. **Test updates** (7 test files): Updated mocks (MockWebContributor, MultiModule, MockContributor, TabMockContributor) with `registerTabs()` override. Replaced scalar getter assertions with `registerTabs()` collector-based tests in: test_application, test_webServer, test_authManager, test_lvglDisplay, test_weather, test_wifiManager, test_configManager.

### Decisions Made
- `getTabIcon()` retained as a **non-virtual helper** on AuthManager, HomeAssistantModule, LVGLDisplayModule, U8G2DisplayModule, WeatherModule — called from within their `registerTabs()` implementations. This avoids duplicating large SVG string literals.
- `WebServerManager::registerRoutes()` is a no-op since its routes are registered inline in `begin()`. The `registerTabs()` method registers the Status sub-pill.
- Dead `TabStub()` calls in `sample-app/src/main.cpp` and `template-app/src/main.cpp` removed — these referenced methods already deleted in WO1 from the display modules.

### Dead Ends & Warnings
- **Missed test files on first pass**: `test_wifiManager.cpp` and `test_configManager.cpp` both called removed scalar getters (`getTabName()`, `getParentTab()`, `getTabOrder()`), causing ERRORED status on the initial test run. These were caught during parent review and fixed with registerTabs() collector tests. Root cause: initial test sweep searched for `->getTabName()` pattern (pointer dereference) but missed the `wifiManager.getTabName()` pattern (dot access on stack object).

### Out-of-Scope Findings
- None.

### Blockers
- None.
