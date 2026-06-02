---
name: device-mcp-client
description: Interacts with a flashed ESP32 running the App Foundation MCP server via JSON-RPC over HTTP. Use this skill whenever you need to query device telemetry, read or write configuration, capture the display, check MCP server status, or toggle read-only mode. Trigger on any mention of device diagnostics, device IP, system_info, capture_display, MCP config, or remote ESP32 interaction.
---

# device-mcp-client

Bridge the gap between the agent and a physical ESP32 running the App Foundation framework. The agent cannot see the device — these scripts discover its IP, query telemetry, capture the display as PNG, and manage MCP server state.

## Quick Reference

| Item | Detail |
|------|--------|
| **Endpoint** | `POST http://<IP>/api/mcp` (JSON-RPC 2.0) |
| **Status** | `GET /api/mcp/status` → `{enabled, readOnly, initialized, toolCount}` |
| **Config** | `GET /api/mcp/config` → `{enabled, readOnly}` |
| **Toggle** | `POST /api/mcp/config` with `{"enabled": bool}` or `{"readOnly": bool}` |
| **Default mode** | `readOnly: true` — mutating tools are blocked until explicitly enabled |

## Built-in MCP Tools

| Tool | Description | Mutates |
|------|-------------|---------|
| `system_info` | Full device telemetry (22 fields: heap, PSRAM, storage, WiFi, temp, chip, partition, uptime, firmware) | No |
| `get_config` | Read all module configs (display, wifi, mcp, etc.) | No |
| `set_config` | Update config by namespace: `{"config":{"wifi":{"ssid":"X"}}}` | **Yes** |
| `reboot` | Restart device (1s delay for response flush) | **Yes** |
| `get_logs` | Log buffer (placeholder — not yet implemented) | No |
| `list_tools` | Enumerate all registered tools with descriptions | No |
| `capture_display` | Framebuffer as base64 XBM (U8G2 vertical-byte format) | No |

### Read-Only Mode

When `readOnly: true` (the default), tools marked as mutating (`set_config`, `reboot`) return an error without executing. Always check server status before attempting writes. To enable writes:

```bash
curl -X POST http://<IP>/api/mcp/config -H 'Content-Type: application/json' -d '{"readOnly": false}'
```

Re-enable read-only mode after making changes.

## `system_info` Response Fields

The tool returns all fields that `/api/system` provides:

```
heap_free, heap_total, heap_min_free, heap_max_alloc   (Internal DRAM)
psram_free, psram_total                                 (PSRAM, 0 if absent)
heap_default_free, heap_default_total                   (DRAM + PSRAM combined)
storage_used, storage_total                             (LittleFS)
chip_temp                                               (°C, internal sensor)
uptime_seconds
build_serial, sdk_version, firmware_version
chip_model, chip_revision, chip_cores, flash_size
ap_mode, wifi_ssid, wifi_ip, wifi_mac, wifi_rssi
partition                                               (active OTA partition)
```

## Scripts

All scripts use only Python stdlib (no pip dependencies).

### `scripts/discovery.py`
Discover the device IP from PlatformIO monitor logs.
```bash
python3 .agents/skills/device-mcp-client/scripts/discovery.py
```
Searches `.pio-mcp-workspace/logs/monitor/` for `Connected successfully. IP:` or `sta ip:` patterns.

### `scripts/diagnostics.py`
Full device diagnostic report: system_info, MCP server status, config, and tool list.
```bash
python3 .agents/skills/device-mcp-client/scripts/diagnostics.py <IP_ADDRESS>
```

### `scripts/captureU8g2.py`
Capture display as PNG (U8G2 XBM vertical-byte monochrome format).
```bash
python3 .agents/skills/device-mcp-client/scripts/captureU8g2.py <IP_ADDRESS> [OUTPUT_PATH]
```

### `scripts/capture_display.py`
Capture display as PNG (RGB565 format, for colour displays).
```bash
python3 .agents/skills/device-mcp-client/scripts/capture_display.py <IP_ADDRESS> [OUTPUT_PATH]
```

## Workflow

1. **Discover** → run `discovery.py` to find the IP (or the user provides it)
2. **Check status** → `GET /api/mcp/status` to confirm server is enabled
3. **Diagnose** → run `diagnostics.py` for full telemetry snapshot
4. **Capture** → run `captureU8g2.py` to see the display
5. **Configure** → disable read-only, call `set_config`, re-enable read-only
