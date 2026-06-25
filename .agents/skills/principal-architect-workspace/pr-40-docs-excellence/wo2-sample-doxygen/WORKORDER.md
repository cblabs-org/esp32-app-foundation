---
name: docs-excellence-wo2
description: Handoff instructions for Work Order 2 — Sample-app full Doxygen + interface-grouped headers
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Add comprehensive Doxygen documentation and interface-grouped export lists to all 6 sample-app source files. The sample-app must be a documentation exemplar — it's the primary teaching vehicle for the framework.

# Strict Boundary Box

- **ONLY** modify files under `sample-app/src/` (the 6 source files listed below)
- Do NOT touch `sample-app/README.md` or `sample-app/platformio.ini` (those are WO3)
- Do NOT touch any framework files or template-app files
- Do NOT modify any logic or behavior — documentation only
- Do NOT create git branches or make commits

# Tooling Constraints

- Use `platformio` MCP tools for builds: `build_project` for compilation verification
- Build the sample-app to verify your changes compile

# House-Style Reference

Read `.agents/skills/house-style-docs/references/cpp-embedded.md` BEFORE starting. Pay special attention to:
- §1 (Module Header Template) — especially "Interface-Grouped Exports"
- §2 (Function and Method Documentation) — Doxygen format
- §4 (Internal Functional Flow) — `// --- Step N ---` block comments

# Context

The sample-app WeatherModule implements 6 framework interfaces: ITickable, IConfigurable, IWebContributor, IDataSource, IMCPContributor, IConfigGate. Read the interface headers in `include/interfaces/` to understand each contract.

# Line-Item Instructions

## 1. WeatherModule.hpp — Header + Doxygen

- [ ] Upgrade header block to full house-style format:
  ```
  * ESP32 App Foundation (c) 2026 CB Labs
  *
  * https://github.com/matthewmcneill/esp32-app-foundation
  *
  * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
  * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
  ```
- [ ] Convert flat export list to **interface-grouped** format covering all 6 interfaces + JsonListener. List EVERY public method. Example structure:
  ```
  *   Lifecycle:
  *   - begin():               ADR-39 transient service locator wiring  [ITickable lifecycle]
  *
  *   Data Accessors:
  *   - getTemperature():      Thread-safe atomic getter
  *   - getCondition():        Thread-safe mutex-protected getter
  *   - getUnits():            Config accessor for units string
  *   - ... (list ALL getters)
  *
  *   ITickable:
  *   - tick():                Periodic update hook (intentionally empty — delegates to DataManager)
  *
  *   IConfigurable ("weather"):
  *   - getConfigNamespace():  ...
  *   - getSchemaVersion():    ...
  *   - applyConfig():         ...
  *   - serializeConfig():     ...
  *   - resetConfig():         ...
  *
  *   IWebContributor:
  *   - registerRoutes():      ...
  *   - getTabName():          ...
  *   - getTabOrder():         ...
  *
  *   IDataSource:
  *   - fetchUpdate():         ...
  *   - getSourceName():       ...
  *   - getPollingIntervalMs():...
  *   - getPriorityTier():     ...
  *
  *   IMCPContributor:
  *   - registerMCPTools():    ...
  *
  *   IConfigGate:
  *   - getGateName():         ...
  *   - isSatisfied():         ...
  *   - getHelpText():         ... (if implemented)
  ```
- [ ] Add `/** @brief ... */` to every public method declaration (~20+ methods)
- [ ] Add inline comments to each service pointer member (`_keyManager`, `_eventBus`, `_configManager`, `_dataManager`)
- [ ] Add inline comments to config fields (`_lat`, `_lon`, `_pollingIntervalMs`, `_units`)
- [ ] Add SRP disclaimer comment near class declaration: "This module is intentionally monolithic to demonstrate all framework interfaces in a single cohesive class. In production, consider separating the HTTP client, JSON parser, and config management."

## 2. WeatherModule.cpp — Doxygen + Flow Comments

- [ ] Upgrade header block: add repo URL, license, year/author ("CB Labs")
- [ ] Add Doxygen `@brief`/`@param`/`@return` to ALL method implementations (~15+ methods). Key ones:
  - `begin()`, `tick()`, `fetchUpdate()`, `endDocument()`, `value()`, `registerRoutes()`, `registerMCPTools()`, `applyConfig()`, `serializeConfig()`, `resetConfig()`, `getConfigNamespace()`, `getSchemaVersion()`, `getTabName()`, `getTabOrder()`, `getSourceName()`, `getPollingIntervalMs()`, `getPriorityTier()`, `getGateName()`, `isSatisfied()`
- [ ] Add `// --- Step N: Description ---` block comments to `fetchUpdate()` (~85 lines):
  - Step 1: Validate credentials (API key check)
  - Step 2: Build URL and allocate TLS client
  - Step 3: Execute HTTP GET request
  - Step 4: Stream and parse JSON response
  - Step 5: Cleanup and return status
- [ ] Add `// --- Step N: Description ---` to `endDocument()` (~73 lines):
  - Step 1: Parse scratch buffers to typed values
  - Step 2: Write to atomic/mutex-protected fields
  - Step 3: Publish EventBus notification + HA state

## 3. WeatherClockBoard.hpp — Header + Doxygen

- [ ] Upgrade header block: full house-style with repo URL, license, year/author
- [ ] Expand export list to include ALL methods: constructor, `render()`, `getBoardName()`, `isExpired()`, `wireEventBus()`, static callback
- [ ] Add Doxygen `@brief` to every method declaration

## 4. WeatherClockBoard.cpp — Doxygen

- [ ] Upgrade header block: full house-style
- [ ] Add Doxygen to: constructor, `wireEventBus()`, `onWeatherUpdated()` (static callback), `render()`

## 5. WeatherWidget.hpp — Header + Doxygen

- [ ] Upgrade header block: full house-style
- [ ] Add Doxygen to `draw()` method: `@brief`, `@param` for each parameter

## 6. WeatherWidget.cpp — Doxygen

- [ ] Upgrade header block: full house-style
- [ ] Add Doxygen to `draw()` implementation

# Verification

- [ ] Build the sample-app: `pio run -e esp32dev` in `sample-app/` — must succeed
- [ ] Build the native env if it exists: `pio test -e native` in `sample-app/`
- [ ] Visually verify all 6 files have:
  - Complete house-style headers (attribution, URL, license, module path, description, interface-grouped exports)
  - Doxygen on every public method
  - `// --- Step N ---` flow comments in long functions
- [ ] **STOP and ask the user if they are happy before distilling**
