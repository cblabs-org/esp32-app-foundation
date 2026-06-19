---
name: pr-34-wo1
description: Handoff instructions for Work order 1 (WifiManager MCP Tools)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the `IMCPContributor` contract on `WifiManager` to expose three domain-specific MCP tools: `wifi_get_status`, `wifi_scan_networks`, and `wifi_forget`.

# Strict Boundary Box
- DO NOT touch `OTAUpdateManager`, `U8G2DisplayModule`, or `ConfigManager`.
- DO NOT modify the MCP server core (`MCPBuiltinTools.cpp` or `MCPServer.cpp`) beyond verifying they compile.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) to compile and verify your code instead of running raw `pio` CLI commands.

# Line-Item Instructions
- [ ] In `modules/WifiManager/WifiManager.hpp`, add `public IMCPContributor` to the class inheritance if not already present.
- [ ] Add `#include <interfaces/IMCPContributor.hpp>` if necessary.
- [ ] Declare `void registerMCPTools(MCPToolRegistry& registry) override;` in the public section of `WifiManager`.
- [ ] In `modules/WifiManager/WifiManager.cpp`, implement `registerMCPTools(MCPToolRegistry& registry)`.
- [ ] Inside `registerMCPTools`, call `registry.addTool(...)` to register `wifi_get_status` (`mutatesState = false`). It should return a JSON object with `connected` (bool), `ip` (string), `gateway` (string), `subnet` (string), `rssi` (int), and `bssid` (string).
- [ ] Register `wifi_scan_networks` (`mutatesState = true`). It should force a WiFi scan (`WiFi.scanNetworks()`) and return an array of networks (`ssid`, `rssi`, `secure`).
- [ ] Register `wifi_forget` (`mutatesState = true`). It should call `WiFi.disconnect(true, true)` or wipe the stored credentials, and return `{"success": true}`.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool to verify your changes. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
