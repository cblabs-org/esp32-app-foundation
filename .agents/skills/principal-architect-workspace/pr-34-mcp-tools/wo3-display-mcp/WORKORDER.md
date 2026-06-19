---
name: pr-34-wo3
description: Handoff instructions for Work order 3 (U8G2DisplayModule MCP Tools)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the `IMCPContributor` contract on `U8G2DisplayModule` to expose three domain-specific MCP tools: `display_get_state`, `display_get_active_board`, and `display_set_screensaver`.

# Strict Boundary Box
- DO NOT touch `WifiManager`, `OTAUpdateManager`, or `ConfigManager`.
- DO NOT modify the MCP server core (`MCPBuiltinTools.cpp` or `MCPServer.cpp`) beyond verifying they compile.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) to compile and verify your code instead of running raw `pio` CLI commands.

# Line-Item Instructions
- [ ] Verify `U8G2DisplayModule` already inherits from `IMCPContributor` (it should).
- [ ] In `modules/U8G2Display/U8G2DisplayModule.hpp`, declare `void registerMCPTools(MCPToolRegistry& registry) override;` if not already present.
- [ ] In `modules/U8G2Display/U8G2DisplayModule.cpp`, implement `registerMCPTools(MCPToolRegistry& registry)`.
- [ ] Inside `registerMCPTools`, call `registry.addTool(...)` to register `display_get_state` (`mutatesState = false`). It should return `screensaver_active` (bool), `blanked` (bool), and `brightness` (int).
- [ ] Register `display_get_active_board` (`mutatesState = false`). It should return `{"board_name": string}` representing the currently active UI board on the carousel.
- [ ] Register `display_set_screensaver` (`mutatesState = true`). It should accept `{"active": bool}`, force the screensaver on or off, and return `{"success": true}`.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool to verify your changes. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
