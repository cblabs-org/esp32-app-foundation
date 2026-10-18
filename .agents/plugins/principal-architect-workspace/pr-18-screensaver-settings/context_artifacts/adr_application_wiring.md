# ADR: Application.cpp MCP Contributor Wiring

## Context

WO2 added `IMCPContributor` inheritance to `U8G2DisplayModule` and implemented `registerMCPTools()` with `display_status` and `display_control` tools. However, the WO2 boundary box correctly prohibited touching `Application.cpp`.

## Problem

Without `_mcpContributors.push_back(&_display)` in `Application::begin()`, the display tools would never appear in `list_tools` on device — `_mcpServer.begin(_mcpContributors, ...)` at line 198 consumes the vector before the tools would be registered.

## Decision

The PA added a single line after WO2 merge:
```cpp
#ifndef NATIVE_BUILD
    _webContributors.push_back(&_display);
    _mcpContributors.push_back(&_display);  // ← added
#endif
```

This mirrors the existing `_webContributors` pattern and must be placed before `_mcpServer.begin()` at line 198.

## Commit

`f25057c fix(app): wire U8G2DisplayModule as MCP contributor for display_status/display_control tools`
