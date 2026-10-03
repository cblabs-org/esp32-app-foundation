---
name: 232-device-identity-wo4
description: Handoff instructions for Work Order 4 — HomeAssistant identity auto-wiring
complexity: mechanical
---

# Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Wire `HomeAssistantModule` to auto-pull identity from `Application` using the same `begin(Application& app)` pattern that `BLEManager` already uses. Fix the hardcoded `"CB Labs"` manufacturer string.

## Context

`BLEManager` already auto-wires identity via `BLEManager::begin(Application& app)` (line 261 of `BLEManager.cpp`):
```cpp
void BLEManager::begin(Application& app) {
    // auto-wires app.getAppName(), app.getManufacturer()
}
```

Consumer `main.cpp` calls it as: `bleManager.begin(app);`

`HomeAssistantModule` currently requires manual wiring in consumer main.cpp:
```cpp
haModule.setAppName(app.getAppName());    // manual
haModule.setAppVersion(BUILD_VERSION);    // manual
// manufacturer is hardcoded to "CB Labs" in HomeAssistant.cpp:194
```

The fix applies the identical pattern: add a `begin(Application& app)` method that self-wires identity, and call it from consumer main.cpp.

## Key Implementation Details

- `HomeAssistantModule` is in `modules/HomeAssistant/HomeAssistant.hpp` and `.cpp`
- It implements `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`
- `_appName` is `std::string`, `setAppName()` takes `const std::string&`
- `_appVersion` is `std::string`, `setAppVersion()` takes `const std::string&`
- No existing `begin()` method — check if the class has one; if it does, add an overload or use a different name like `wireIdentity(Application& app)`
- `Application::getAppName()` returns `const char*`, `Application::getManufacturer()` returns `const char*`
- Build version macro is `BUILD_VERSION`
- Forward-declare `Application` in the header to avoid circular includes

## Strict Boundary Box

- **DO NOT** modify `Application::begin()` or any other module's wiring — those are complete.
- **DO NOT** modify `WifiManager`, `BLEManager`, `MCPServer`, `AuthManager`, `OTADownloadEngine`, LVGL XML, `auth.js`, or documentation.
- **DO NOT** change the existing `setAppName()` / `setAppVersion()` APIs — they must still work for consumers who prefer manual wiring.
- Changes are limited to: `HomeAssistant.hpp`, `HomeAssistant.cpp`, `src/main.cpp`, `sample-app/src/main.cpp`, and any relevant test file.

## Tooling Constraints

- Use the `platformio-mcp_platformio` MCP tools (`build_project`) for compilation verification — never raw `pio` CLI.

## Line-Item Instructions

- [ ] **`modules/HomeAssistant/HomeAssistant.hpp`**:
  - Add a forward declaration at the top: `class Application;`
  - Add a public method: `void begin(Application& app);`
  - Add a `setManufacturer()` setter if one doesn't exist: `void setManufacturer(const std::string& manufacturer) { _manufacturer = manufacturer; }`
  - Add a `_manufacturer` member field: `std::string _manufacturer;`

- [ ] **`modules/HomeAssistant/HomeAssistant.cpp`**:
  - Add `#include "Application.hpp"` (or the appropriate include path — check how BLEManager includes it)
  - Implement `begin(Application& app)`:
    ```cpp
    void HomeAssistantModule::begin(Application& app) {
        if (_appName.empty()) {
            _appName = app.getAppName();
        }
        if (_manufacturer.empty()) {
            _manufacturer = app.getManufacturer();
        }
        if (_appVersion.empty()) {
            _appVersion = BUILD_VERSION;
        }
    }
    ```
    The `empty()` guards ensure that explicit pre-`begin()` setter calls from consumers are not overwritten (same pattern as BLEManager).
  - Update line 194 — change hardcoded manufacturer:
    - **Current:** `device["manufacturer"] = "CB Labs";`
    - **Change to:** `device["manufacturer"] = _manufacturer.empty() ? "CB Labs" : _manufacturer;`
    This keeps `"CB Labs"` as the fallback default but allows it to be overridden.

- [ ] **`src/main.cpp`**:
  - After `app.registerModule(&haModule);` (line 134), add: `haModule.begin(app);`
  - Remove the now-redundant manual wiring lines:
    - Remove: `haModule.setAppName(app.getAppName());`
    - Remove: `haModule.setAppVersion(BUILD_VERSION);`
  - Keep the `setKeyManager`, `setEventBus`, `setConfigManager` calls — those wire framework services, not identity.

- [ ] **`sample-app/src/main.cpp`**:
  - Same changes as above — add `haModule.begin(app);`, remove manual `setAppName`/`setAppVersion` calls.

## Verification Requirement

1. Build the project using `build_project` MCP tool — must compile cleanly with zero errors on both environments.
2. Run native tests — all must pass.
3. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

