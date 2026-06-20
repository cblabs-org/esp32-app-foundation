---
title: "PR-34: Module-Specific MCP Tools — Architect Distillation"
distilled_at: "2026-06-20T07:18:00Z"
original_plan_id: pr-34-mcp-tools
artifacts:
  - context_artifacts/adr_mcp_tool_architecture.md
  - context_artifacts/adr_display_already_satisfied.md
  - context_artifacts/state_tool_catalog.md
  - context_artifacts/graveyard_ota_last_check.md
---

# PR-34: Module-Specific MCP Tools

## Executive Summary

Implemented 8 new domain-specific MCP tools across 3 core framework modules (WifiManager, OTAUpdateManager, ConfigManager) via the `IMCPContributor` interface. The 4th module (U8G2DisplayModule) was already satisfied by its existing `display_status`/`display_control` tools. Device now exposes 17 total MCP tools. All verified on hardware (`esp32dev`, `v0.0.11-dev.15`).

## Key Facts

- **Tag:** `v0.0.11-dev.15` — pushed and flashed
- **Branch:** `pr-34-mcp-tools` (merged to `main`)
- **Modules touched:** WifiManager, OTAUpdateManager, ConfigManager, Application (comment-only)
- **Modules NOT touched:** U8G2DisplayModule (already satisfied), MCPServer, MCPBuiltinTools
- **Skill updates:** `device-mcp-client` rewritten as discovery-first; `framework-expert` now embeds MCP server system docs and corrected `mcp-contributor.md` interface

## Next Steps

1. **Push `main` to origin** — all commits are local-only at session end
2. **Update `latest-dev` floating tag** — `git tag -f latest-dev && git push -f origin latest-dev`
3. **Tech debt: `capture_display` ownership** — still in `MCPBuiltinTools.cpp`, should be owned by `U8G2DisplayModule` via `registerMCPTools()`. Requires refactoring the `DisplayInfo` late-binding pattern. Tracked in WO3 status.
4. **`wifi_scan_networks` is a mutating read** — it triggers a WiFi scan (brief STA disconnect risk in AP mode). Architecturally correct as `mutatesState=true` but worth noting for agents that expect it to be read-only.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_mcp_tool_architecture.md` - Architecture decisions: stateless lambdas, void* context, NATIVE_BUILD guards, naming conventions
- `context_artifacts/adr_display_already_satisfied.md` - Why WO3 required no code changes and the capture_display tech debt
- `context_artifacts/state_tool_catalog.md` - Complete 17-tool catalog with module ownership and mutatesState flags
- `context_artifacts/graveyard_ota_last_check.md` - The `last_check_ms: 0` bug and the dual check-path fix
