# PR-34: Module-Specific MCP Tools

**Goal:** Turn the ESP32 device into a highly introspectable, agent-friendly environment by implementing domain-specific MCP diagnostic and control tools directly within the core modules (`WifiManager`, `OTAUpdateManager`, `U8G2DisplayModule`, `ConfigManager`), using the `IMCPContributor` contract.

## Work Order Ledger

- **[x] [wo1-wifi-mcp](wo1-wifi-mcp/WORKORDER.md):** `wifi_get_status`, `wifi_scan_networks`, `wifi_forget` — merged `e9086d7`
- **[x] [wo2-ota-mcp](wo2-ota-mcp/WORKORDER.md):** `ota_get_status`, `ota_check_now`, `ota_trigger_update` — merged `1e12428`
- **[x] [wo3-display-mcp](wo3-display-mcp/WORKORDER.md):** Already Satisfied — existing `display_status`/`display_control` conform to architecture. `capture_display` tech debt documented.
- **[x] [wo4-config-mcp](wo4-config-mcp/WORKORDER.md):** `config_get_schema_version`, `config_factory_reset` — merged `6bf6953`

## Summary

**8 new MCP tools** added across 3 modules (WifiManager, OTAUpdateManager, ConfigManager). 1 module (U8G2DisplayModule) already satisfied. All builds verified green on `esp32dev` and/or `esp32s3nano`. Hardware-verified on device at `v0.0.11-dev.15` — 9/9 read-only tools passing.

## Distillation

Full architect distillation: [context_bridge.md](./context_bridge.md)

