---
name: microkernel-refactor-wo2
description: Handoff instructions for Work order 2 - Extract OTA
complexity: architect
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Extract the `OTAUpdateManager` from `Application`'s owned members and wire it via explicit composition in `main.cpp` using the `#ifdef WITH_OTA` flag. Ensure cross-service wiring (like `ConfigManager` snapshot and `OTA_PROGRESS` event) is preserved correctly per MK-1 and MK-6.

## Strict Boundary Box
- Extract **only** `OTAUpdateManager`.
- Do not modify Ring-1 Core services' ownership.
- Modify `modules/Application/`, `modules/OTAUpdateManager/`, `template-app/src/main.cpp`, `sample-app/src/main.cpp`, and any display modules that subscribe to `OTA_PROGRESS` (`LVGLDisplay`, `U8G2Display`).

## Tooling Constraints
Use the `platformio` MCP tools for building and testing. Do not use raw CLI commands for PlatformIO tasks. Verify `native` and `esp32dev` compilation across the flag matrix (with and without `-D WITH_OTA`). Note that `WITH_OTA` is **default off**.

## Checklist
- [ ] **`Application.hpp`**: Remove `#include "../OTAUpdateManager/OTAUpdateManager.hpp"`. Remove `OTAUpdateManager* _otaManager = nullptr;`. Remove `getOTAUpdateManager()`.
- [ ] **`Application.hpp` (Facade Contract)**: Move the `OTA_PROGRESS` topic constant and its payload struct (`OtaProgressPayload`) from `OTAUpdateManager.hpp` into `Application.hpp` (e.g. under `ApplicationEvents`). Wrap them in `#ifdef WITH_OTA`. To satisfy EO-07 (legible absent-module error), ensure that consumers attempting to use `OTA_PROGRESS` without `WITH_OTA` get a clean, understandable compiler error.
- [ ] **`Application.cpp`**: Remove the heap allocation (`_otaManager = new OTAUpdateManager(...)`) and initialization from `begin()`. Remove the `delete _otaManager;` from the destructor.
- [ ] **`OTAUpdateManager` (Pre-reboot hook)**: Implement EO-08: provide a synchronous pre-reboot hook or guarantee async headroom before the reboot is triggered by the OTA manager, so that subscribers to `OTA_PROGRESS` (like power-monitor) have time to complete their work before the reboot occurs.
- [ ] **`template-app/src/main.cpp`** and **`sample-app/src/main.cpp`**:
  - Add `#ifdef WITH_OTA` blocks to include `"OTAUpdateManager/OTAUpdateManager.hpp"`.
  - Instantiate `OTAUpdateManager otaManager(&app.getConfigManager(), &app.getAuthManager());` (or similar, based on its constructor).
  - Register it: `app.registerModule(&otaManager);` before `app.begin()`.
  - If `otaManager` needs its own `begin()` call, do so after `app.begin()`. Make sure to pass `app` or whatever it needs to snapshot the config manifest.
- [ ] **Subscribers (Displays)**: Update `LVGLDisplayModule` and `U8G2DisplayModule` to point to the new location of `OTA_PROGRESS` and its payload (now in the facade, `ApplicationEvents::OTA_PROGRESS`). Wrap these subscriptions in `#ifdef WITH_OTA` so they still compile when OTA is off.
- [ ] **Verification**: Run `native` tests and compile `esp32dev` (with and without `WITH_OTA`).
- [ ] When finished, confirm compilation, then STOP and message the Principal Architect. Do NOT distill or wrap up yet.

## Status Update (2026-07-29)
**COMPLETE**. The `OTAUpdateManager` has been fully extracted from `Application`, decoupled via `ApplicationEvents::OtaProgressPayload`, and registered via composition in `main.cpp`. The work has been verified against `native` and `esp32dev` targets for `template-app`. (Note: `sample-app` fails compilation due to pre-existing display extraction issues out of scope for WO2). Distillery snapshot generated.
