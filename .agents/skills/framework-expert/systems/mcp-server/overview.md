# MCP Server System

The ESP32 App Foundation includes a JSON-RPC 2.0 MCP (Model Context Protocol) server that runs on the device. It provides a dynamic, discoverable set of tools for AI agents to query telemetry, read/write configuration, capture the display, and interact with any module that implements `IMCPContributor`.

## Architecture

Tools come from two sources:

### Built-in Tools (always present)
Registered by `MCPBuiltinTools` — framework-level, available on every device:

| Tool | Description | Mutates |
|------|-------------|---------|
| `system_info` | Device telemetry: heap, PSRAM, storage, WiFi, temp, chip, partition, uptime, firmware | No |
| `get_config` | Read all module configs (display, wifi, mcp, ota, etc.) | No |
| `set_config` | Update config by namespace: `{"config":{"wifi":{"ssid":"X"}}}` | **Yes** |
| `patch_config` | Apply RFC 6902 JSON Patch ops to a config namespace | **Yes** |
| `reboot` | Restart device (1s delay for response flush) | **Yes** |
| `get_logs` | Log buffer (placeholder — ring-buffer logger planned) | No |
| `list_tools` | Enumerate all registered tools with descriptions and schemas | No |
| `capture_display` | Framebuffer as base64 XBM (U8G2 vertical-byte format) | No |

### Module-Contributed Tools (dynamic)
Any module implementing `IMCPContributor` registers tools via `registerMCPTools()` at boot. These are auto-discovered — you never need to update a catalog. Current framework modules contribute:

| Module | Read-Only Tools | Mutating Tools |
|--------|----------------|----------------|
| **WifiManager** | `wifi_get_status` | `wifi_scan_networks`, `wifi_forget` |
| **OTAUpdateManager** | `ota_get_status` | `ota_check_now`, `ota_trigger_update` |
| **ConfigManager** | `config_get_schema_version` | `config_factory_reset` |
| **U8G2DisplayModule** | `display_status` | `display_control` |
| **ButtonModule** (`WITH_BUTTONS`) | `button_status` | — |
| **EncoderModule** (`WITH_ENCODER`) | `encoder_status` | — |
| **KeyManager** | `key_list`, `key_get` | `key_add`, `key_delete` |

**This table grows with every PR.** Downstream applications add their own tools. Always call `list_tools` to discover the actual set.

## Talking to the Device

### Endpoints

| Item | Detail |
|------|--------|
| **Tool calls** | `POST https://<IP>/api/mcp` (JSON-RPC 2.0) |
| **Server status** | `GET /api/mcp/status` → `{enabled, readOnly, initialized, toolCount}` |
| **Server config** | `GET /api/mcp/config` → `{enabled, readOnly}` |
| **Toggle** | `POST /api/mcp/config` with `{"enabled": bool}` or `{"readOnly": bool}` |
| **Default mode** | `readOnly: true` — mutating tools are blocked until explicitly enabled |

### Discovery-First Workflow

1. **Discover IP** → run `discovery.py` or the user provides it
2. **Check status** → `GET /api/mcp/status` to confirm server is enabled and note `toolCount`
3. **Enumerate tools** → call `list_tools` to get the full catalog
4. **Call tools** → use the JSON-RPC 2.0 envelope below

### Recovering the IP When Discovery Fails

If `discovery.py` finds nothing and mDNS / `<hostname>.local` or subnet scans do
not resolve the device (common on isolated VLANs, captive networks, or right
after a WiFi change), read the IP directly from the **boot log over serial**:
start a serial monitor and reset the device — the network stack prints the
acquired IP during boot (the same value exposed as `wifi_ip` in `system_info`).
Serial access needs no network and no portal auth, so it is the reliable
last-resort channel for locating a device that has gone quiet on the network.

### Calling a Tool

```bash
curl -sk https://<IP>/api/mcp \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
      "name": "<TOOL_NAME>",
      "arguments": { <TOOL_ARGS> }
    }
  }'
```

Response shape:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{"type": "text", "text": "<JSON_STRING>"}],
    "isError": false
  }
}
```

Parse `result.content[0].text` as JSON to get the tool's actual response.

### Read-Only Mode

When `readOnly: true` (the default), tools marked as mutating return an error without executing. To enable writes:

```bash
# Disable read-only
curl -sk -X POST https://<IP>/api/mcp/config \
  -H 'Content-Type: application/json' -d '{"readOnly": false}'

# ... call mutating tools ...

# Re-enable read-only
curl -sk -X POST https://<IP>/api/mcp/config \
  -H 'Content-Type: application/json' -d '{"readOnly": true}'
```

## Device Interaction Scripts

All scripts use only Python stdlib (no pip dependencies). Located in `.agents/skills/device-mcp-client/scripts/`.

| Script | Purpose | Usage |
|--------|---------|-------|
| `discovery.py` | Find device IP from PlatformIO monitor logs | `python3 .agents/skills/device-mcp-client/scripts/discovery.py` |
| `diagnostics.py` | Full report: system_info, MCP status, config, tool list | `python3 .agents/skills/device-mcp-client/scripts/diagnostics.py <IP>` |
| `captureU8g2.py` | Capture display as PNG (monochrome XBM) | `python3 .agents/skills/device-mcp-client/scripts/captureU8g2.py <IP> [OUTPUT]` |
| `capture_display.py` | Capture display as PNG (RGB565 colour) | `python3 .agents/skills/device-mcp-client/scripts/capture_display.py <IP> [OUTPUT]` |

### `system_info` Response Fields

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
time_configured, time_synced, time_local                (NTP state)
```
