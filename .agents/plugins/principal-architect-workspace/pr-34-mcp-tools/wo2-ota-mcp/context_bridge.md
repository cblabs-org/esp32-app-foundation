---
title: "WO2: OTAUpdateManager IMCPContributor Implementation"
distilled_at: "2026-06-20T07:36:00+01:00"
original_plan_id: pr-34-mcp-tools
artifacts:
  - context_artifacts/state_files_changed.md
  - context_artifacts/adr_ota_mcp_design.md
---

# WO2 — OTA MCP Tools: Context Bridge

## Executive Summary

Implemented `IMCPContributor` on `OTAUpdateManager`, registering three MCP tools (`ota_get_status`, `ota_check_now`, `ota_trigger_update`) using the static function pointer + `void* context` pattern. Build passed on first attempt with zero errors.

## What Was Done

- Added `IMCPContributor` to `OTAUpdateManager` class inheritance
- Declared and implemented `registerMCPTools()` with 3 tools behind `#ifndef NATIVE_BUILD`
- Added `MCPToolRegistry.hpp` include in the platform-conditional block
- Updated `Application.cpp` registration comment to reflect the new interface

## Files Modified

- `modules/OTAUpdateManager/OTAUpdateManager.hpp` — Interface, include, forward-decl, declaration
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — Include + registerMCPTools implementation
- `modules/Application/Application.cpp` — Registration comment update (line 149)

## Next Steps

No further work required for WO2. If resuming this PR:

1. Proceed to WO3 (U8G2DisplayModule MCP tools) or WO4 (ConfigManager MCP tools)
2. The pattern is now well-established — follow the WifiManager (WO1) and OTAUpdateManager (WO2) implementations

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/state_files_changed.md` - Exact summary of all lines changed in each file
- `context_artifacts/adr_ota_mcp_design.md` - Design decisions for the three OTA MCP tools
