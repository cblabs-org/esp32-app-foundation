---
name: mk03-wo1
description: Extract HomeAssistant from Application and establish the honest main.cpp composition mechanism
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the "honest main.cpp" composition mechanism and extract `HomeAssistantModule` out of `Application`'s owned members. After this WO, HomeAssistant is only built and instantiated if `WITH_HOMEASSISTANT` is defined in the build flags, and is registered from `main.cpp` via `registerModule`.

# Strict Boundary Box
- Do NOT extract MCP, OTA, Time, or display. Only extract HomeAssistant.
- Do NOT touch Ring-1 Core services (WebServer, Auth, WiFi, Config, Key, Crypto, DataManager, EventBus). They stay owned members of Application.
- Surface any findings about out-of-scope defects to the Principal Architect; do not file issues.

# Tooling Constraints
Use the `platformio` MCP tools for building and testing. Run tests with `run_tests` and build with `build_project`. Do not run raw `pio` CLI commands.

# Line-Item Instructions
- [x] In `platformio.ini`, under `[env:esp32dev]`, add `-D WITH_HOMEASSISTANT` to `build_flags`.
- [x] In `modules/Application/Application.hpp`:
  - Remove `#include "../HomeAssistant/HomeAssistantModule.hpp"`.
  - Remove `HomeAssistantModule _haModule;` from the class members.
  - Remove `HomeAssistantModule* getHomeAssistant()` (if it exists).
  - Ensure `getEventBus()` is public and returns `EventBus&` or `EventBus*` so `main.cpp` can inject it into `HomeAssistantModule`. (`getConfigManager()` and `getKeyManager()` are already public).
- [x] In `modules/Application/Application.cpp`:
  - Remove the HomeAssistant wiring in `begin()` (e.g. `_haModule.setKeyManager...`, `_haModule.setConfigManager...`, `_haModule.setEventBus...`, `_configManager.registerModule(&_haModule);`, etc.)
- [x] In `template-app/src/main.cpp`, `sample-app/src/main.cpp`, and the root `src/main.cpp` (check which ones exist):
  - Add `#ifdef WITH_HOMEASSISTANT` guards.
  - Inside the guard, `#include "modules/HomeAssistant/HomeAssistantModule.hpp"`.
  - Instantiate a global `HomeAssistantModule haModule;`.
  - In `setup()`, wire it exactly as `Application::begin()` did: inject `KeyManager`, `ConfigManager`, `EventBus`, AppName, AppVersion. (e.g., `haModule.setKeyManager(&app.getKeyManager());`).
  - Call `app.registerModule(&haModule);` inside the `#ifdef`.
- [x] In `modules/HomeAssistant/HomeAssistantModule.cpp`:
  - Wrap the entire file's contents in `#ifdef WITH_HOMEASSISTANT` / `#endif` so it's excluded from compilation when the flag is missing. (This is essential since LDF will still try to compile it).
- [x] In `modules/HomeAssistant/HomeAssistantModule.hpp`:
  - You may also wrap the implementation details in `#ifdef WITH_HOMEASSISTANT` if necessary, but just wrapping the cpp file is usually enough. For completeness, it's safe to wrap the class declaration in `#ifdef WITH_HOMEASSISTANT` so any inclusion without the flag is a compile error, matching MK-6a fail-fast rule.

# Verification Requirement
Verify that `platformio` `build_project` succeeds for `esp32dev` with `-D WITH_HOMEASSISTANT` enabled. Verify `native` environment tests pass.
Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill to package the state and drop control.

# Status Update
- **Status**: ✅ COMPLETED & VALIDATED
- **Date**: 2026-07-29
- **Commit**: `f22d778` (`feat(mk03): extract HomeAssistant from Application owned members into main.cpp composition`)
- **Verification**: `esp32dev` PlatformIO build passed; 491/491 `native` unit tests passed.
- **Distillery**: Packaged at `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk03-module-composition/wo1-extract-ha/`

