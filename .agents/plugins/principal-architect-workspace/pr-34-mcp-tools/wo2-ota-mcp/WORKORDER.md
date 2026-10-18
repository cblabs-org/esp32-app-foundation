---
name: pr-34-wo2
description: Handoff instructions for Work order 2 (OTAUpdateManager MCP Tools)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement the `IMCPContributor` contract on `OTAUpdateManager` to expose three domain-specific MCP tools: `ota_get_status`, `ota_check_now`, and `ota_trigger_update`.

# Strict Boundary Box
- DO NOT touch `WifiManager`, `U8G2DisplayModule`, or `ConfigManager`.
- DO NOT modify the MCP server core (`MCPBuiltinTools.cpp` or `MCPServer.cpp`) beyond verifying they compile.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) to compile and verify your code instead of running raw `pio` CLI commands.

# Line-Item Instructions
- [ ] In `modules/OTAUpdateManager/OTAUpdateManager.hpp`, add `public IMCPContributor` to the class inheritance if not already present.
- [ ] Add `#include <interfaces/IMCPContributor.hpp>` if necessary.
- [ ] Declare `void registerMCPTools(MCPToolRegistry& registry) override;` in the public section of `OTAUpdateManager`.
- [ ] In `modules/OTAUpdateManager/OTAUpdateManager.cpp`, implement `registerMCPTools(MCPToolRegistry& registry)`.
- [ ] Inside `registerMCPTools`, call `registry.addTool(...)` to register `ota_get_status` (`mutatesState = false`). It should return `state` (e.g., "Idle", "Downloading", "Verifying"), `version`, `channel`, and `last_check_ms`.
- [ ] Register `ota_check_now` (`mutatesState = true`). It should force the manager to check the GitHub API and return `{"update_available": bool, "latest_version": string}`.
- [ ] Register `ota_trigger_update` (`mutatesState = true`). It should optionally accept a `version` string, force the manager to begin the OTA sequence, and return `{"status": "Update initiated"}`.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool to verify your changes. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

---

# Execution Status

| Field | Value |
|---|---|
| **Status** | ✅ COMPLETE |
| **Worker Session** | `cb4b0b66-0ec6-4724-8772-4bc9ed63a84b` |
| **Completed** | 2026-06-20T07:36:00+01:00 |
| **Build Result** | SUCCESS (47.6s, esp32dev) |
| **Iterations** | 0 (first-attempt success) |

## Checklist (all items verified)

- [x] In `modules/OTAUpdateManager/OTAUpdateManager.hpp`, add `public IMCPContributor` to the class inheritance.
- [x] Add `#include <interfaces/IMCPContributor.hpp>` if necessary.
- [x] Declare `void registerMCPTools(MCPToolRegistry& registry) override;` in the public section.
- [x] In `modules/OTAUpdateManager/OTAUpdateManager.cpp`, implement `registerMCPTools(MCPToolRegistry& registry)`.
- [x] Register `ota_get_status` (`mutatesState = false`) — returns state, version, channel, last_check_ms.
- [x] Register `ota_check_now` (`mutatesState = true`) — forces GitHub check, returns update_available + latest_version.
- [x] Register `ota_trigger_update` (`mutatesState = true`) — accepts optional version, returns status.

