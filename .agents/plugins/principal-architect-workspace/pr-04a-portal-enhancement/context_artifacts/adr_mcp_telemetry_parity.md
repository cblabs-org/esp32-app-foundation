# ADR: MCP Telemetry Parity

## Context
The MCP `system_info` tool originally returned 12 fields (heap, uptime, chip basics). The portal's `/api/system` endpoint returned 22 fields. Agents using the MCP tool were blind to WiFi, storage, temperature, and partition info.

## Decision
Expanded `system_info` in `mcpBuiltinTools.cpp` to return all 22 fields, matching `/api/system` exactly. Both endpoints now use the same ESP-IDF calls.

## Added Fields
- `storage_used`, `storage_total` (LittleFS)
- `chip_temp` (internal sensor via `temperatureRead()`)
- `build_serial` (`__DATE__ " " __TIME__`)
- `sdk_version` (`ESP.getSdkVersion()`)
- `chip_revision` (`esp_chip_info().revision`)
- `ap_mode`, `wifi_ssid`, `wifi_ip`, `wifi_mac`, `wifi_rssi` (WiFi API)
- `partition` (`esp_ota_get_running_partition()->label`)

## Added Includes
`esp_chip_info.h`, `esp_partition.h`, `esp_ota_ops.h`, `LittleFS.h`, `WiFi.h`

## MCP Read-Only Mode
- Default: `readOnly: true` — mutating tools (`set_config`, `reboot`) blocked
- Toggle: `POST /api/mcp/config` with `{"readOnly": false}`
- Portal MCP tab shows tools greyed out (opacity 0.4) with BLOCKED label when unavailable
- Badge colours switch to grey (#999) when blocked

## device-mcp-client Skill
Rewritten for the new framework. `diagnostics.py` now parses all 22 fields with human-readable formatting. SKILL.md documents read-only mode workflow and all tool signatures.
