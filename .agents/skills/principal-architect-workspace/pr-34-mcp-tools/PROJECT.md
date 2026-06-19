# PR-34: Module-Specific MCP Tools

**Goal:** Turn the ESP32 device into a highly introspectable, agent-friendly environment by implementing domain-specific MCP diagnostic and control tools directly within the core modules (`WifiManager`, `OTAUpdateManager`, `U8G2DisplayModule`, `ConfigManager`), using the `IMCPContributor` contract.

## Work Order Ledger

- **[ ] [wo1-wifi-mcp](wo1-wifi-mcp/WORKORDER.md):** Implement `wifi_get_status`, `wifi_scan_networks`, and `wifi_forget`.
- **[ ] [wo2-ota-mcp](wo2-ota-mcp/WORKORDER.md):** Implement `ota_get_status`, `ota_check_now`, and `ota_trigger_update`.
- **[ ] [wo3-display-mcp](wo3-display-mcp/WORKORDER.md):** Implement `display_get_state`, `display_get_active_board`, and `display_set_screensaver`.
- **[ ] [wo4-config-mcp](wo4-config-mcp/WORKORDER.md):** Implement `config_get_schema_version` and `config_factory_reset`.
