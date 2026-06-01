---
name: device-mcp-client
description: Interacts with the ESP32 Device MCP Server through JSON-RPC to query telemetry, configure parameters, and capture real-time display buffers to image files.
---

# `device-mcp-client`

This skill provides native utilities to interact with a flashed ESP32 running the JSON-RPC Device MCP server via HTTP.

## Overview
Because the agent cannot visually "see" the physical ESP32, this skill provides a suite of Python scripts that bridge the gap. It automatically discovers the dynamic IP of the board from hardware logs, safely pings its telemetry endpoints, and can download and decode the raw U8G2 XBM framebuffer into a visual `PNG` artifact for debugging UI layouts.

## MCP Server Details
- **Endpoint:** `http://<DEVICE_IP>/api/mcp` (HTTP POST, JSON-RPC 2.0)
- **Built-in tools:** `system_info`, `get_config`, `set_config`, `capture_display`, `reboot`, `list_tools`, `get_logs`
- **Display hardware:** SSD1322 256×64 monochrome (U8G2 XBM format)

## Scripts

### `scripts/discovery.py`
Parses the `.pio-mcp-workspace/logs/monitor/` directory to locate the dynamically assigned DHCP IP address from the ESP32's boot sequence. Reads the latest log file (sorted by filename timestamp) and searches for `Connected successfully. IP:` or `sta ip:` patterns.
**Usage:** `python3 .agents/skills/device-mcp-client/scripts/discovery.py`

### `scripts/captureU8g2.py`
Fetches the `capture_display` frame from the target IP, decodes the Base64 U8G2 XBM monochrome buffer, and converts it to a standard PNG. **Use this for the current hardware revision (SSD1322 256×64).**
**Usage:** `python3 .agents/skills/device-mcp-client/scripts/captureU8g2.py [IP_ADDRESS] [OUTPUT_PATH]`

### `scripts/capture_display.py`
Fetches the `capture_display` frame and decodes the raw RGB565 binary into a PNG. Use this for colour displays.
**Usage:** `python3 .agents/skills/device-mcp-client/scripts/capture_display.py [IP_ADDRESS] [OUTPUT_PATH]`

### `scripts/diagnostics.py`
Queries the device's current system info and configuration by invoking the `system_info` and `get_config` MCP tools.
**Usage:** `python3 .agents/skills/device-mcp-client/scripts/diagnostics.py [IP_ADDRESS]`
