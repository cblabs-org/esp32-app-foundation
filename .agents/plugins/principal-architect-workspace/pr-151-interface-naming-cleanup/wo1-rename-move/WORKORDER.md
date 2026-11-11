---
name: 151-interface-naming-wo1
description: Handoff instructions for Work Order 1 — rename and relocate PreconditionRegistry and TabRegistry
complexity: standard   # mechanical | standard | architect
---

# WO1: Rename & Relocate PreconditionRegistry → IPreconditionRegistry, TabRegistry → ITabRegistry

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Rename two abstract interfaces to follow the project's `I`-prefix convention and relocate them from `include/utils/` to `include/interfaces/`. Update every includer, reference, and documentation file. Verify zero old-name references remain.

**Parent project:** PR #151 — Interface Naming Convention Cleanup  
**Parent PROJECT.md:** `.agents/plugins/principal-architect-workspace/pr-151-interface-naming-cleanup/PROJECT.md`

## Strict Boundary Box

You MUST NOT:
- Modify any logic or behavior — this is a **pure naming/location refactor**
- Change the `TabEntry` struct name (it's a data struct, not an interface)
- Create additional branches beyond what is already checked out
- Modify files unrelated to the `PreconditionRegistry` → `IPreconditionRegistry` or `TabRegistry` → `ITabRegistry` rename
- Make architectural decisions — all decisions are pre-made in this checklist

## Tooling Constraints

- Use the `platformio` MCP tools for build and test verification:
  - `build_project` for compilation checks (environments: `native`, `esp32dev`, `crowpanel-128-rotary`)
  - `run_tests` for test execution (environment: `native`)
- Do NOT use raw `pio` CLI commands

## Line-Item Instructions

### Part A: `PreconditionRegistry` → `IPreconditionRegistry`

- [x] **Create** `include/interfaces/IPreconditionRegistry.hpp`:
  - Copy content from `include/utils/PreconditionRegistry.hpp`
  - Rename class `PreconditionRegistry` → `IPreconditionRegistry`
  - Update the include guard / `#pragma once` (already uses `#pragma once` — just verify)
  - Update the file header comment to reflect the new path: `include/interfaces/IPreconditionRegistry.hpp`

- [x] **Delete** `include/utils/PreconditionRegistry.hpp`

- [x] **Update** `include/interfaces/IPreconditionContributor.hpp`:
  - Change `#include "utils/PreconditionRegistry.hpp"` → `#include "interfaces/IPreconditionRegistry.hpp"`
  - Update any forward declarations or type references: `PreconditionRegistry` → `IPreconditionRegistry`

- [x] **Update** `modules/PreconditionEngine/PreconditionEngine.hpp`:
  - Change include path
  - Update base class name: `PreconditionRegistry` → `IPreconditionRegistry`

- [x] **Update** `modules/WifiManager/WifiPreconditionContributor.hpp`:
  - Change include path if present
  - Update parameter type: `PreconditionRegistry&` → `IPreconditionRegistry&`

- [x] **Update** `modules/TimeManager/TimePreconditionContributor.hpp`:
  - Change include path if present
  - Update parameter type: `PreconditionRegistry&` → `IPreconditionRegistry&`

- [x] **Update** `sample-app/src/WeatherModule.hpp` and `sample-app/src/WeatherModule.cpp`:
  - Change include path if present
  - Update parameter type: `PreconditionRegistry&` → `IPreconditionRegistry&`

- [x] **Update** `sample-app/test/test_unit/test_weather/test_main.cpp`:
  - Update any type references to `PreconditionRegistry` → `IPreconditionRegistry`

- [x] **Update** `template-app/src/MyApp.hpp` and `template-app/src/MyApp.cpp`:
  - Change include path if present
  - Update parameter type: `PreconditionRegistry&` → `IPreconditionRegistry&`

### Part B: `TabRegistry` → `ITabRegistry`

- [x] **Create** `include/interfaces/ITabRegistry.hpp`:
  - Copy content from `include/utils/TabRegistry.hpp`
  - Rename class `TabRegistry` → `ITabRegistry`
  - Keep the `TabEntry` struct name **unchanged** (it's a data struct, not an interface)
  - Update the file header comment to reflect the new path: `include/interfaces/ITabRegistry.hpp`

- [x] **Delete** `include/utils/TabRegistry.hpp`

- [x] **Update** `include/interfaces/IWebContributor.hpp`:
  - Change `#include "utils/TabRegistry.hpp"` → `#include "interfaces/ITabRegistry.hpp"`
  - Update parameter type in `registerTabs()`: `TabRegistry&` → `ITabRegistry&`

- [x] **Update** `modules/WebServer/WebServerManager.hpp` and `modules/WebServer/WebServerManager.cpp`:
  - Change include path
  - Update concrete implementation base class: `TabRegistry` → `ITabRegistry`
  - Update all type references

- [x] **Update** the following module headers — change include paths and update all `TabRegistry` → `ITabRegistry` type references in each:
  - `modules/Application/Application.hpp`
  - `modules/AuthManager/AuthManager.hpp` and `.cpp`
  - `modules/ConfigManager/ConfigManager.hpp`
  - `modules/HomeAssistant/HomeAssistantModule.hpp`
  - `modules/KeyManager/KeyManager.hpp`
  - `modules/LVGLDisplay/LVGLDisplayModule.hpp` and `.cpp`
  - `modules/MCPServer/MCPServer.hpp`
  - `modules/OTAUpdateManager/OTAUpdateManager.hpp`
  - `modules/TimeManager/TimeManager.hpp`
  - `modules/U8G2Display/U8G2DisplayModule.hpp` and `.cpp`
  - `modules/WifiManager/WifiManager.hpp`

- [x] **Update** `sample-app/src/WeatherModule.hpp` (and `.cpp` if referenced):
  - `TabRegistry&` → `ITabRegistry&`

- [x] **Update** `sample-app/test/test_unit/test_weather/test_main.cpp`:
  - Update type references

- [x] **Update** `template-app/src/MyApp.hpp` and `template-app/src/MyApp.cpp`:
  - `TabRegistry&` → `ITabRegistry&`

### Part C: Umbrella Header & Documentation

- [x] **Check** `include/AppFoundation.h`:
  - If it includes `utils/PreconditionRegistry.hpp`, update to `interfaces/IPreconditionRegistry.hpp`
  - If it includes `utils/TabRegistry.hpp`, update to `interfaces/ITabRegistry.hpp`
  - If it documents or references either old name, update those references

- [x] **Update** `.agents/skills/framework-expert/SKILL.md`:
  - In the interface table: add rows for `IPreconditionRegistry` and `ITabRegistry` if not already present
  - In the trigger keywords: add `IPreconditionRegistry`, `ITabRegistry`
  - Update any existing references to the old names

- [x] **Update** `.agents/skills/framework-expert/interfaces/precondition-contributor.md`:
  - All references to `PreconditionRegistry` → `IPreconditionRegistry`

- [x] **Update** `.agents/skills/framework-expert/references/architecture.md`:
  - If it references `PreconditionRegistry` or `TabRegistry`, update to new names

- [x] **Update** `.agents/skills/framework-expert/references/registration.md`:
  - If it references `PreconditionRegistry` or `TabRegistry`, update to new names

- [x] **Update any test stubs/mocks** that subclass `PreconditionRegistry` or `TabRegistry`:
  - Search test directories for mock/stub classes derived from either old name
  - Update base class names to `IPreconditionRegistry` / `ITabRegistry`

### Part D: Verification

- [x] **Grep check — old names must be gone:**
  ```
  grep -r "PreconditionRegistry" --include='*.hpp' --include='*.cpp' --include='*.h' include/ modules/ sample-app/ template-app/ lib/
  ```
  Must return ZERO hits for the bare name `PreconditionRegistry` (only `IPreconditionRegistry` allowed).

- [x] **Grep check — TabRegistry:**
  ```
  grep -r "TabRegistry" --include='*.hpp' --include='*.cpp' --include='*.h' include/ modules/ sample-app/ template-app/ lib/
  ```
  Must return ZERO hits for `TabRegistry` (only `ITabRegistry` and `TabEntry` allowed).

- [x] **Run tests** via `platformio` MCP `run_tests` (environment: `native`) — all tests pass.

- [x] **Build** via `platformio` MCP `build_project`:
  - Environment: `esp32dev` — builds clean
  - Environment: `crowpanel-128-rotary` — builds clean

### Commit

Use this exact commit message:
```
refactor(interfaces)!: rename PreconditionRegistry → IPreconditionRegistry, TabRegistry → ITabRegistry

Move both abstract registration interfaces from include/utils/ to
include/interfaces/ with I-prefix to match the project's naming
convention for pure abstract classes.

BREAKING CHANGE: Consumers referencing PreconditionRegistry or
TabRegistry must update to IPreconditionRegistry / ITabRegistry.
```

## Verification Requirement

All of Part D must pass before committing. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
- Created `include/interfaces/IPreconditionRegistry.hpp` defining pure abstract interface `IPreconditionRegistry` with `add()`.
- Deleted `include/utils/PreconditionRegistry.hpp`.
- Created `include/interfaces/ITabRegistry.hpp` defining `struct TabEntry` and pure abstract interface `ITabRegistry` with `addTab()`.
- Deleted `include/utils/TabRegistry.hpp`.
- Updated `include/interfaces/IPreconditionContributor.hpp` and `include/interfaces/IWebContributor.hpp` with new interface includes and signatures.
- Updated `modules/PreconditionEngine/PreconditionEngine.hpp` and `.cpp` to implement `IPreconditionRegistry`.
- Updated `modules/WebServer/WebServerManager.hpp` and `.cpp` to implement `ITabRegistry`.
- Updated all module headers/implementations implementing `IPreconditionContributor` (`WifiManager`, `TimeManager`, `WeatherModule`, `MyApp`).
- Updated all module headers/implementations implementing `IWebContributor` (`Application`, `AuthManager`, `ConfigManager`, `HomeAssistant`, `KeyManager`, `LVGLDisplay`, `MCPServer`, `OTAUpdateManager`, `TimeManager`, `U8G2Display`, `WifiManager`, `WeatherModule`, `MyApp`).
- Updated test stubs and unit test mocks (`test_helpers/MockTabRegistry.hpp`, `test_application.cpp`, `test_webServer.cpp`, `test_weather/test_main.cpp`, display MCP/native stubs).
- Updated `include/AppFoundation.h` umbrella exports and includes.
- Updated `framework-expert` skill references and documentation files across `docs/`.

### Decisions Made
- Maintained `struct TabEntry` inside `include/interfaces/ITabRegistry.hpp` without an `I` prefix since it is a plain data struct (POD) rather than a pure virtual interface.
- Added `IPreconditionRegistry.hpp` and `ITabRegistry.hpp` to `include/AppFoundation.h` so consumer applications importing the single façade header have full type visibility for registration signatures.

### Dead Ends & Warnings
None. All renames were clean drop-in replacements.

### Out-of-Scope Findings
None.

### Blockers
None.
