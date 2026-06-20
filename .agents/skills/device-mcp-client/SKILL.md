---
name: device-mcp-client
description: Interacts with a flashed ESP32 running the App Foundation MCP server via JSON-RPC over HTTP. Use this skill whenever you need to query device telemetry, read or write configuration, capture the display, check MCP server status, toggle read-only mode, or call any module-contributed MCP tool. Trigger on any mention of device diagnostics, device IP, system_info, capture_display, MCP config, remote ESP32 interaction, wifi_get_status, ota_get_status, display_status, config_factory_reset, or "call a tool on the device". The tool set is dynamic — always discover available tools via `list_tools` rather than assuming a fixed set.
---

# device-mcp-client

Bridge the gap between the agent and a physical ESP32 running the App Foundation framework. The device exposes a JSON-RPC 2.0 MCP server whose tool set is **dynamic** — core modules register domain-specific tools at boot via the `IMCPContributor` interface, and downstream applications can add their own. Never assume a fixed tool catalog; always discover what's available.

## Quick Reference

| Item | Detail |
|------|--------|
| **Endpoint** | `POST https://<IP>/api/mcp` (JSON-RPC 2.0) |
| **Status** | `GET /api/mcp/status` → `{enabled, readOnly, initialized, toolCount}` |
| **Config** | `GET /api/mcp/config` → `{enabled, readOnly}` |
| **Toggle** | `POST /api/mcp/config` with `{"enabled": bool}` or `{"readOnly": bool}` |
| **Default mode** | `readOnly: true` — mutating tools are blocked until explicitly enabled |

## Discovery-First Workflow

The tool set changes with every firmware build. **Always start by discovering what's available:**

1. **Discover IP** → run `discovery.py` (or the user provides it)
2. **Check status** → `GET /api/mcp/status` to confirm server is enabled and note `toolCount`
3. **Enumerate tools** → call `list_tools` to get the full catalog with descriptions and schemas
4. **Diagnose** → run `diagnostics.py` for a comprehensive snapshot
5. **Capture display** → run `captureU8g2.py` to see what's on screen
6. **Configure** → disable read-only, call mutating tools, re-enable read-only

### Calling `list_tools`

```bash
curl -sk https://<IP>/api/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"list_tools","arguments":{}}}'
```

The response includes every registered tool with its name, description, and parameter schema. Parse this to understand what's available before calling any specific tool.

## Tool Architecture

Tools come from two sources:

### 1. Built-in Tools (always present)
Registered by `MCPBuiltinTools` — these are framework-level and available regardless of what application runs on the device:

| Tool | Description | Mutates |
|------|-------------|---------|
| `system_info` | Device telemetry: heap, PSRAM, storage, WiFi, temp, chip, partition, uptime, firmware | No |
| `get_config` | Read all module configs (display, wifi, mcp, ota, etc.) | No |
| `set_config` | Update config by namespace: `{"config":{"wifi":{"ssid":"X"}}}` | **Yes** |
| `reboot` | Restart device (1s delay for response flush) | **Yes** |
| `get_logs` | Log buffer (placeholder — ring-buffer logger planned) | No |
| `list_tools` | Enumerate all registered tools with descriptions and schemas | No |
| `capture_display` | Framebuffer as base64 XBM (U8G2 vertical-byte format) | No |

### 2. Module-Contributed Tools (dynamic)
Any module implementing `IMCPContributor` registers tools via `registerMCPTools()` at boot. These are discovered automatically — you don't need to know what modules are loaded. The current framework modules contribute:

| Module | Read-Only Tools | Mutating Tools |
|--------|----------------|----------------|
| **WifiManager** | `wifi_get_status` | `wifi_scan_networks`, `wifi_forget` |
| **OTAUpdateManager** | `ota_get_status` | `ota_check_now`, `ota_trigger_update` |
| **ConfigManager** | `config_get_schema_version` | `config_factory_reset` |
| **U8G2DisplayModule** | `display_status` | `display_control` |

**This table will grow.** Downstream applications built on the framework will add their own tools (e.g., a weather station might add `weather_get_readings`). Always use `list_tools` to discover the actual set.

### Read-Only Mode

When `readOnly: true` (the default), tools marked as mutating return an error without executing. Always check server status before attempting writes:

```bash
# Disable read-only mode
curl -sk -X POST https://<IP>/api/mcp/config \
  -H 'Content-Type: application/json' \
  -d '{"readOnly": false}'

# ... call mutating tools ...

# Re-enable read-only mode
curl -sk -X POST https://<IP>/api/mcp/config \
  -H 'Content-Type: application/json' \
  -d '{"readOnly": true}'
```

## Calling Any Tool

All tools use the same JSON-RPC 2.0 envelope:

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

The response always has this shape:
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

## `system_info` Response Fields

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

## Adding New Tools (for developers)

Any module can contribute MCP tools by implementing the `IMCPContributor` interface:

```cpp
#include "interfaces/IMCPContributor.hpp"

class MyModule : public ITickable, public IMCPContributor {
    void registerMCPTools(MCPToolRegistry& registry) override {
        registry.addTool("my_tool", "Description of what it does",
            [](void* ctx, JsonObject& args, JsonObject& result) {
                auto* self = static_cast<MyModule*>(ctx);
                result["value"] = self->getSomeValue();
            }, this, false);  // false = read-only, true = mutates state
    }
};
```

Key rules:
- Use **stateless lambdas** (no captures) that decay to function pointers — `std::function` is prohibited on this platform (heap allocation risk).
- Pass `this` as the `void* context` parameter; cast back with `static_cast` inside the handler.
- Set `mutatesState = true` for any tool that changes device state — read-only mode will block it otherwise.
- The `Application` module auto-discovers `IMCPContributor` at registration time via `std::is_base_of_v<>`.
- Tools registered this way appear automatically in `list_tools` output — no manual catalog updates needed.
