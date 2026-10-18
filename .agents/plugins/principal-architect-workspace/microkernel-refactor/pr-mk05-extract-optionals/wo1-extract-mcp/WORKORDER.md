---
name: microkernel-refactor-wo1
description: Handoff instructions for Work order 1 - Extract MCP
complexity: standard
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Extract the `MCPServer` from `Application`'s owned members and wire it via explicit composition in `main.cpp` using the `#ifdef WITH_MCP` flag. This satisfies MK-1 and MK-6 of the microkernel design.

## Strict Boundary Box
- Extract **only** `MCPServer`. Do not extract OTA or Time yet.
- Do not modify Ring-1 Core services.
- Modify `modules/Application/`, `modules/MCPServer/`, `template-app/src/main.cpp`, and `sample-app/src/main.cpp`.

## Tooling Constraints
Use the `platformio` MCP tools for building and testing. Do not use raw CLI commands for PlatformIO tasks. Verify `native` and `esp32dev` compilation across the flag matrix (with and without `-D WITH_MCP`). Note that `WITH_MCP` is **default off**.

## Checklist
- [x] **`Application.hpp`**: Remove `#include "../MCPServer/MCPServer.hpp"`.
- [x] **`Application.hpp`**: Remove the `MCPServer _mcpServer;` member.
- [x] **`Application.hpp`**: Remove `MCPServer& getMCPServer();`.
- [x] **`Application.hpp`**: Add a getter to access registered MCP contributors: `const std::vector<IMCPContributor*>& getMcpContributors() const { return _mcpContributors; }`
- [x] **`Application.cpp`**: Remove all references to `_mcpServer` (in `begin()`, `tick()`, etc.). The built-in MCP tools wiring (`_configManager`, `_timeManager`, `_wifiManager`, `_authManager`) should be removed from `Application::begin()`.
- [x] **`template-app/src/main.cpp`** and **`sample-app/src/main.cpp`**:
  - Add `#ifdef WITH_MCP` to include `"MCPServer.hpp"` and instantiate a static `MCPServer mcpServer;`.
  - In `setup()`, *before* `app.begin()`, register the module: `app.registerModule(&mcpServer);`.
  - *After* `app.begin()`, initialize the server: `mcpServer.begin(app.getMcpContributors(), &app.getConfigManager(), &app.getTimeManager(), &app.getWifiManager(), &app.getAuthManager());`.
  - Any display setup calls to the MCP server (e.g. `app.getMCPServer().setDisplayInfo(...)`) must be updated to use `mcpServer.setDisplayInfo(...)` inside `#ifdef WITH_MCP`.
- [x] **Verification**: Run `native` tests and compile `esp32dev` (with and without `WITH_MCP`).
- [x] When finished, confirm compilation, then STOP and message the Principal Architect.

## Status Log
- **2026-07-29**: ✅ **COMPLETE**. `MCPServer` extracted from `Application` owned members. Explicit composition wired in `template-app/src/main.cpp`, `sample-app/src/main.cpp`, and `src/main.cpp` under `#ifdef WITH_MCP`. Tested: 467/467 native unit tests passed; `esp32dev` builds succeeded both with and without `WITH_MCP` (~34.1 KiB flash saved when off). Output distilled to `context_bridge.md`.

