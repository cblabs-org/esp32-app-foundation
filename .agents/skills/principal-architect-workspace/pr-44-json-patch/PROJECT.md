# PR-44 — RFC 6902 JSON Patch for ConfigManager

**Issue:** [#28](https://github.com/matthewmcneill/esp32-app-foundation/issues/28)
**Base branch:** `main` (`e9de86a`)
**Stack tip:** `pr-44-json-patch/wo5-build-verification` (when complete)
**Human approval:** Granted 2026-07-21 09:20 (implementation plan approved in conversation `3f6fb84f`)

## Objective

Add element-level config writes via a `patch_config` MCP tool using an RFC 6902 JSON Patch subset. Includes `_configMutex` for thread safety and `saveModuleConfig()` serialize/persist split.

## Design Authority

All design decisions were agreed in [Issue #28 comment thread](https://github.com/matthewmcneill/esp32-app-foundation/issues/28). See implementation plan for the decision table.

## Work Order Ledger

- **[x] [wo1-patch-engine](wo1-patch-engine/WORKORDER.md):** JSON Pointer resolver, patch engine, ConfigManager `patchNamespaceConfig()`, mutex, `saveModuleConfig()` refactor
- **[x] [wo2-mcp-tool](wo2-mcp-tool/WORKORDER.md):** `patch_config` MCP tool registration in `MCPBuiltinTools.cpp`
- **[x] [wo3-unit-tests](wo3-unit-tests/WORKORDER.md):** 39 test cases (30 standalone + 10 integration) — 401/401 passing
- **[x] [wo4-docs](wo4-docs/WORKORDER.md):** Framework-expert skill docs update
- **[x] [wo5-build-verification](wo5-build-verification/WORKORDER.md):** 3×3 build matrix — 416/416 tests, all firmware green
