---
name: pr-25-wo1
description: Handoff instructions for Work Order 1 — registerModule<T>() + ITickable signature + begin() migration
complexity: architect
---

# WO1 — Architectural Refinements

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

## Objective

Implement three tightly-coupled interface changes to the ESP32 App Foundation:

1. Add a `template<typename T> bool registerModule(T*)` method to `Application`
2. Change `ITickable::tick()` to `tick(uint32_t currentMillis)`
3. Migrate `Application::begin()` to use `registerModule()` for all internal modules

These changes make the framework's registration API uniform and testable.

---

## Strict Boundary Box

- **DO** modify: `Application.hpp`, `Application.cpp`, `ITickable.hpp`, and
  every file that implements `ITickable::tick()` or is registered in `begin()`.
- **DO** modify: all native unit test files that use `tick()` or mock `ITickable`.
- **DO NOT** modify any files in `docs/`, `template/`, `.agents/`, or `CONTRIBUTING.md`.
- **DO NOT** modify the EventBus (that is WO2).
- **DO NOT** create branches or make git commits — the PA handles branching.

---

## Line-Item Checklist

### Part A — `registerModule<T>()` Template Method

- [ ] Open `modules/Application/Application.hpp`
- [ ] Add the following `#include` statements near the top (if not already present):
  ```cpp
  #include <type_traits>
  ```
- [ ] Add the following template method in the **public** section of the `Application` class.
  It MUST be in the header (not `.cpp`) because it is a template:

  ```cpp
  /**
   * @brief Universal module registration — auto-detects all framework interfaces.
   *
   * Uses compile-time if constexpr to check which interfaces T implements,
   * and registers the module with every applicable subsystem in a single call.
   * This is the primary, recommended registration API for both framework
   * internals and downstream application modules.
   *
   * @tparam T  Concrete module type (deduced from pointer).
   * @param module  Pointer to the module instance. Must not be null.
   * @return true if at least one interface was detected and registered.
   */
  template<typename T>
  bool registerModule(T* module) {
      if (!module) return false;
      bool any = false;
      if constexpr (std::is_base_of_v<ITickable, T>)        { registerTickable(module);              any = true; }
      if constexpr (std::is_base_of_v<IConfigurable, T>)    { _configManager.registerModule(module); any = true; }
      if constexpr (std::is_base_of_v<IWebContributor, T>)  { registerWebContributor(module);        any = true; }
      if constexpr (std::is_base_of_v<IMCPContributor, T>)  { registerMcpContributor(module);        any = true; }
      if constexpr (std::is_base_of_v<IDataSource, T>)      { registerDataSource(module);            any = true; }
      if constexpr (std::is_base_of_v<IConfigGate, T>)      { registerConfigGate(module);            any = true; }
      if constexpr (std::is_base_of_v<IStatusReporter, T>)  { registerStatusReporter(module);        any = true; }
      if (!any) {
          ESP_LOGW(TAG, "registerModule: type implements no known framework interface");
      }
      return any;
  }
  ```

- [ ] Verify that `Application.hpp` already includes the interface headers it needs
  (`ITickable.hpp`, `IConfigurable.hpp`, `IWebContributor.hpp`, `IMCPContributor.hpp`,
  `IDataSource.hpp`, `IConfigGate.hpp`, `IStatusReporter.hpp`). Add any missing includes.

> **NOTE on IDisplayBoard**: Do NOT include `IDisplayBoard` in the template.
> Display boards are registered via the display carousel sub-object API
> (`display.carousel.add()`), not via Application. This is by design.

### Part B — `ITickable::tick(uint32_t currentMillis)` Signature Change

- [ ] Open `include/interfaces/ITickable.hpp`
- [ ] Change the pure virtual method signature from:
  ```cpp
  virtual void tick() = 0;
  ```
  to:
  ```cpp
  virtual void tick(uint32_t currentMillis) = 0;
  ```
- [ ] Update the Doxygen comment to document the `currentMillis` parameter:
  ```
  @param currentMillis  Milliseconds since boot, captured once per Application::tick() loop.
  ```

- [ ] Find every class that overrides `tick()`. Use this grep to find them all:
  ```bash
  grep -rn "void tick() override" modules/ template/ test/ --include="*.hpp" --include="*.cpp"
  ```
  For EACH result, update the signature to `void tick(uint32_t currentMillis) override`.

  **Expected files** (verify with grep — there may be more):
  - `modules/WifiManager/WifiManager.hpp` and `.cpp`
  - `modules/TimeManager/TimeManager.hpp` and `.cpp`
  - `modules/WebServer/WebServerManager.hpp` and `.cpp`
  - `modules/OTAUpdateManager/OTAUpdateManager.hpp` and `.cpp`
  - `modules/U8G2Display/U8G2DisplayModule.hpp` and `.cpp`
  - `template/src/myApp.hpp` and `.cpp`
  - `test/test_unit/test_application/test_application.cpp` (mock class)

- [ ] In each `.cpp` implementation of `tick()`:
  - Update the function signature to accept `uint32_t currentMillis`
  - If the function currently calls `millis()` directly, replace those calls
    with `currentMillis` where appropriate. If the function uses `millis()` for
    purposes other than "current loop time" (e.g., measuring elapsed durations
    relative to a stored timestamp), keep `millis()` for the stored-timestamp
    capture and use `currentMillis` for the "now" comparisons.

### Part C — `Application::tick()` — Capture and Inject `currentMillis`

- [ ] Open `modules/Application/Application.cpp`
- [ ] Find the `Application::tick()` method
- [ ] At the top of `tick()`, capture the current time once:
  ```cpp
  #ifndef NATIVE_BUILD
      uint32_t currentMillis = millis();
  #else
      uint32_t currentMillis = 0;  // Native builds use test-injected time
  #endif
  ```
- [ ] Update the loop that calls `tickable->tick()` to pass `currentMillis`:
  ```cpp
  for (auto* tickable : _tickables) {
      tickable->tick(currentMillis);
  }
  ```
  (The exact loop structure may differ — find it and update it.)

### Part D — `Application::begin()` Migration to `registerModule()`

This is the most important part. The framework must eat its own dog food.

- [ ] Open `modules/Application/Application.cpp`, find `Application::begin()`
- [ ] Locate **Step 7** (ConfigManager registration, approximately lines 128–149).
  It currently has individual `_configManager.registerModule()` calls. **Remove
  those individual calls.** The dependency injection calls (`setConfigManager`,
  `setKeyManager`, `_authManager.begin()`, `_otaManager = new ...`) STAY.

- [ ] Locate **Step 14** (web contributor + tickable registration, approximately
  lines 183–205). It currently has raw `_webContributors.push_back()` and
  `_tickables.push_back()` calls. **Remove all of those.**

- [ ] Locate **Step 16** (config gate registration, approximately line 228).
  It currently has `_configGates.push_back(&_wifiGate)`. **Remove it.**

- [ ] Replace all removed registration lines with uniform `registerModule()` calls.
  Place them AFTER the dependency injection block but BEFORE `_configManager.beginAll()`.
  The exact calls are:

  ```cpp
  // --- Module registration (auto-detects all interfaces per module) ---
  registerModule(&_wifiManager);        // ITickable + IConfigurable + IWebContributor
  registerModule(&_timeManager);        // ITickable + IConfigurable + IWebContributor
  registerModule(&_authManager);        // IConfigurable + IWebContributor
  registerModule(&_configManager);      // IWebContributor
  registerModule(&_keyManager);         // IWebContributor
  registerModule(&_mcpServer);          // IConfigurable + IWebContributor
  registerModule(_otaManager);          // ITickable + IConfigurable + IWebContributor
  registerModule(&_webServerManager);   // ITickable
  registerModule(&_appTabContributor);  // IWebContributor

  #ifndef NATIVE_BUILD
  registerModule(&_display);                    // ITickable + IStatusReporter + IConfigurable + IWebContributor + IMCPContributor
  registerModule(_display.settingsTabStub());   // IWebContributor
  registerModule(_display.screenshotTabStub()); // IWebContributor
  #endif

  registerModule(&_wifiGate);           // IConfigGate
  ```

- [ ] **CRITICAL**: The `_mcpContributors.push_back(&_display)` line in the
  current Step 14 — this is also handled by `registerModule()` now (via the
  `IMCPContributor` detection). Remove the explicit push_back.

- [ ] Verify the `_statusReporter = &_display` assignment in `begin()` (around
  line 83). This is a **singleton pointer assignment** that happens early in boot
  (before the registration block). It must remain as-is — `registerModule()` also
  calls `registerStatusReporter()` which is fine (it will set the same pointer again).
  No changes needed here, but verify it doesn't conflict.

- [ ] Update Step comments/numbering to reflect the simplified flow. Steps 7 and 14
  are now merged into a single registration block.

### Part E — Unit Tests

- [ ] Open `test/test_unit/test_application/test_application.cpp`
- [ ] Update any mock `ITickable` classes to use `tick(uint32_t currentMillis)` signature
- [ ] Update any test code that calls `tick()` to pass a value (e.g., `tick(1000)`)
- [ ] Add at least one new test that verifies `registerModule()` correctly detects
  a multi-interface type. Example:
  ```cpp
  class MultiModule : public ITickable, public IWebContributor {
  public:
      void tick(uint32_t currentMillis) override { tickCount++; }
      void registerRoutes(AsyncWebServer& server) override {}
      const char* getTabName() const override { return "Test"; }
      int getTabOrder() const override { return 99; }
      int tickCount = 0;
  };

  TEST(Application, registerModule_detects_multiple_interfaces) {
      Application app;
      MultiModule mod;
      bool result = app.registerModule(&mod);
      ASSERT_TRUE(result);
      // Verify it was registered as both tickable and web contributor
  }
  ```
  Adapt to the existing test framework style (Unity or GoogleTest — check
  which the file uses).

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
1. `pio run -e native` compiles cleanly
2. `pio test -e native` passes all tests
3. `pio run -e esp32dev` compiles cleanly
4. No raw `push_back()` calls remain in `begin()` for tickables, web contributors,
   MCP contributors, or config gates

**STOP** after verification and send a message back to the Principal Architect
reporting your results. Do not distill or commit until the PA approves.
