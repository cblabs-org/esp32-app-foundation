# Tool Catalog — v0.0.11-dev.15 (17 tools)

## Built-in Tools (MCPBuiltinTools)

| Tool | Mutates | Description |
|------|---------|-------------|
| `system_info` | No | Device telemetry (heap, PSRAM, storage, WiFi, temp, chip, uptime, firmware) |
| `get_config` | No | All module configs with _meta envelope |
| `set_config` | **Yes** | Update config by namespace |
| `reboot` | **Yes** | Restart device (1s delay) |
| `get_logs` | No | Placeholder — ring-buffer logger planned |
| `list_tools` | No | Enumerate all tools with descriptions/schemas |
| `capture_display` | No | Framebuffer as base64 XBM |

## Module-Contributed Tools (IMCPContributor)

| Module | Tool | Mutates | Description |
|--------|------|---------|-------------|
| WifiManager | `wifi_get_status` | No | Connected, IP, gateway, subnet, RSSI, BSSID, ap_mode |
| WifiManager | `wifi_scan_networks` | **Yes** | Force scan, return discovered networks |
| WifiManager | `wifi_forget` | **Yes** | Erase credentials, restart to AP mode |
| OTAUpdateManager | `ota_get_status` | No | State, version, channel, last_check_ms, auto_check |
| OTAUpdateManager | `ota_check_now` | **Yes** | Force immediate GitHub release check |
| OTAUpdateManager | `ota_trigger_update` | **Yes** | Download, verify, flash pipeline |
| ConfigManager | `config_get_schema_version` | No | Backup envelope version (currently 1) |
| ConfigManager | `config_factory_reset` | **Yes** | Wipe /mod/*.json + manifest, reboot |
| U8G2DisplayModule | `display_status` | No | Brightness, flip, activeBoard, screensaver, carousel |
| U8G2DisplayModule | `display_control` | **Yes** | Wake/sleep, brightness, screensaver, timeouts |
