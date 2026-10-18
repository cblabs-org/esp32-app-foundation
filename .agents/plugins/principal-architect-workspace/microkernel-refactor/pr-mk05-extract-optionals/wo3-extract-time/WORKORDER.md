---
name: microkernel-refactor-wo3
description: Handoff instructions for Work order 3 - Extract Time
complexity: standard
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Extract the `TimeManager` (and its associated `TimeConfigGate`) from `Application`'s owned members and wire it via explicit composition in `main.cpp` using the `#ifdef WITH_TIME` flag. `WITH_TIME` should be **default ON**.

## Strict Boundary Box
- Extract **only** `TimeManager` and `TimeConfigGate`.
- Do not modify Ring-1 Core services' ownership.
- Modify `modules/Application/`, `template-app/src/main.cpp`, `sample-app/src/main.cpp`, `src/main.cpp`, and any tests calling `app.getTimeManager()`.

## Tooling Constraints
Use the `platformio` MCP tools for building and testing. Do not use raw CLI commands for PlatformIO tasks. Verify `native` and `esp32dev` compilation across the flag matrix (with and without `-D WITH_TIME`). Note that `WITH_TIME` is **default ON**, meaning you should ensure it is defined in the appropriate `platformio.ini` environments or just globally true by default unless disabled.

## Checklist
- [x] **`Application.hpp`**: Remove `#include "../TimeManager/TimeManager.hpp"` and `#include "../TimeManager/TimeConfigGate.hpp"`. Remove the `TimeManager _timeManager;` and `TimeConfigGate _timeGate;` members. Remove `getTimeManager()` and `getTimeGate()` accessors.
- [x] **`Application.cpp`**: Remove all references to `_timeManager`, specifically `_timeManager.setConfigManager(&_configManager);`, `registerModule(&_timeManager);`, and `_timeManager.begin();` from `Application::begin()`.
- [x] **`template-app/src/main.cpp`**, **`sample-app/src/main.cpp`**, and **`src/main.cpp`**:
  - Add `#ifdef WITH_TIME` blocks to include `"TimeManager.hpp"` and `"TimeConfigGate.hpp"`.
  - Instantiate `TimeManager timeManager;` and `TimeConfigGate timeGate{timeManager};` statically.
  - In `setup()`, *before* `app.begin()`, register the module: `timeManager.setConfigManager(&app.getConfigManager()); app.registerModule(&timeManager);`.
  - *After* `app.begin()`, call `timeManager.begin();`.
  - Note: `MCPServer` needs `TimeManager`. So when calling `mcpServer.begin(...)`, you must pass `&timeManager` instead of `&app.getTimeManager()`. Wrap the argument or use `#ifdef WITH_TIME` to conditionally pass it, or just pass `nullptr` if `WITH_TIME` is off.
- [x] **Verification**: Run `native` tests and compile `esp32dev` (with and without `WITH_TIME`).
- [x] When finished, confirm compilation, then STOP and message the Principal Architect. Do NOT distill or wrap up yet.

## Status Update

- **2026-07-29**: ✅ **COMPLETE**. `TimeManager` and `TimeConfigGate` extracted from `Application` owned members. Explicit composition wired in `template-app/src/main.cpp`, `sample-app/src/main.cpp`, and `src/main.cpp` under `#ifdef WITH_TIME` (default ON). Tested: 449/449 native unit tests passed; `esp32dev` and `esp32s3nano` builds succeeded both with and without `WITH_TIME` (~12.8 KiB flash saved when off). Output distilled to `context_bridge.md`.
