---
id: pr-18
title: "Screensaver & Display Settings"
status: "📋 PLANNING"
branch: "pr-18-screensaver-settings"
depends_on: []
design_spec: "docs/design_specification.md#78-display-module-object-model-target-architecture"
decisions: ""
---

# PR-18: Screensaver & Display Settings

## Goal

Add comprehensive screensaver control and display settings across all interfaces:
MCP tools, web portal, REST API, and persistent config. Currently the screensaver
exists with a rich internal API (`ScreensaverController`) but no way for users or
agents to inspect or control it beyond the single `sleepTimeout` config key.

## Current State

### What Exists
- `ScreensaverController` with full dim/blank/wake state machine
- `POST /api/display/sleep` — accepts `{"action": "sleep|wake"}` and `{"timeout": ms}`
- `GET /api/display/status` — returns `sleeping`, `sleepTimeout` fields
- `sleepTimeout` persisted in `/mod/display.json` via ConfigManager
- `dimTimeout` hardcoded to `sleepTimeout / 2` (not independently configurable)
- No screensaver enable/disable toggle
- No separate dim delay vs blank delay in config
- No screensaver-related MCP tools
- No web portal UI for display/screensaver settings (only the App tab clock font)

### What's Needed
1. **MCP `display_status` tool** — runtime screensaver state (active, dimmed, blanked, idle time)
2. **MCP `display_control` tool** — wake, sleep, set brightness, set timeout
3. **Config expansion** — `screensaverEnabled`, `dimTimeout`, `blankTimeout` as independent settings
4. **Web portal Display tab** — brightness slider, flip toggle, screensaver on/off, dim/blank delays
5. **Web portal tests** — Playwright coverage for the new Display tab

## Architectural Constraints

1. All new config keys go through the existing `U8G2DisplayModule` IConfigurable namespace (`display`)
2. MCP tools are registered via `IMCPContributor::registerMCPTools()` on U8G2DisplayModule
3. The web portal follows the existing tab pattern (see `wifi.js`, `time.js`, `appconfig.js`)
4. Config changes must be persisted via `ConfigManager::saveModuleConfig()`
5. All REST routes follow the existing `/api/display/*` prefix
6. The `/web-testing-strategy` workflow must be followed for web portal changes

## Work Order Ledger

- **[ ] [wo1-config-expansion](wo1-config-expansion/WORKORDER.md):** Expand display config schema with independent screensaver settings (screensaverEnabled, dimTimeout, blankTimeout) and update applyConfig/serializeConfig/resetConfig
- **[ ] [wo2-mcp-tools](wo2-mcp-tools/WORKORDER.md):** Add `display_status` and `display_control` MCP tools for runtime screensaver inspection and control
- **[ ] [wo3-display-tab](wo3-display-tab/WORKORDER.md):** Create web portal Display tab with brightness slider, flip toggle, screensaver settings controls, and Playwright tests

## Sequencing

```
WO1 (config) ──→ WO2 (MCP tools) ──→ WO3 (web portal)
```

WO1 must go first: it establishes the config schema that WO2 and WO3 depend on.
WO2 and WO3 could theoretically run in parallel, but they both touch `U8G2DisplayModule.cpp`
(registerMCPTools vs registerRoutes), so serializing is safer.

## Pre-Reading for All Workers

1. `modules/U8G2Display/U8G2DisplayModule.hpp` — full module header
2. `modules/U8G2Display/U8G2DisplayModule.cpp` — config methods (applyConfig, serializeConfig, resetConfig, registerRoutes, registerMCPTools)
3. `modules/U8G2Display/ScreensaverController.hpp` — screensaver API surface
4. `web/portal/appconfig.js` — existing App tab pattern for portal JS
5. `web/portal/index.html` — portal shell and tab registration
6. `.agents/skills/house-style-docs/references/cpp-embedded.md` — house style
7. `.agents/workflows/web-testing-strategy.md` — mandatory 3-phase web testing
