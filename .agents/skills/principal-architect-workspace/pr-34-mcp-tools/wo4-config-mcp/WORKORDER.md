---
name: pr-34-wo4
description: Handoff instructions for Work order 4 (ConfigManager MCP Tools)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the `IMCPContributor` contract on `ConfigManager` to expose two domain-specific MCP tools: `config_get_schema_version` and `config_factory_reset`.

# Strict Boundary Box
- DO NOT touch `WifiManager`, `OTAUpdateManager`, or `U8G2DisplayModule`.
- DO NOT modify the MCP server core (`MCPBuiltinTools.cpp` or `MCPServer.cpp`) beyond verifying they compile.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) to compile and verify your code instead of running raw `pio` CLI commands.

# Line-Item Instructions
- [ ] In `modules/ConfigManager/ConfigManager.hpp`, add `public IMCPContributor` to the class inheritance if not already present.
- [ ] Add `#include <interfaces/IMCPContributor.hpp>` if necessary.
- [ ] Declare `void registerMCPTools(MCPToolRegistry& registry) override;` in the public section of `ConfigManager`.
- [ ] In `modules/ConfigManager/ConfigManager.cpp`, implement `registerMCPTools(MCPToolRegistry& registry)`.
- [ ] Inside `registerMCPTools`, call `registry.addTool(...)` to register `config_get_schema_version` (`mutatesState = false`). It should return `{"version": int}` representing the current config schema version.
- [ ] Register `config_factory_reset` (`mutatesState = true`). It should wipe the NVS partition (factory reset) and initiate a reboot. Return `{"success": true, "message": "NVS wiped, restarting"}`.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool to verify your changes. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
