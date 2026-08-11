#!/usr/bin/env python3
"""
ESP32 App Foundation — Device Diagnostics via MCP Server.

Queries system_info, MCP server status/config, device config, and tool list.
Uses only stdlib (no pip dependencies).
"""
import sys
import json
import argparse
import urllib.request
import urllib.error
import ssl
import os


def mcp_request(ip, tool_name, arguments=None, token=None):
    """Send a JSON-RPC 2.0 tools/call request to the device MCP server."""
    protocol = "https" if token else "http"
    url = f"{protocol}://{ip}/api/mcp"
    payload = {
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {"name": tool_name, "arguments": arguments or {}},
        "id": 1
    }
    data = json.dumps(payload).encode("utf-8")
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"
        
    req = urllib.request.Request(url, data=data, headers=headers, method="POST")
    ctx = ssl._create_unverified_context() if token else None
    
    try:
        with urllib.request.urlopen(req, timeout=5, context=ctx) as resp:
            body = json.loads(resp.read().decode("utf-8"))
    except urllib.error.URLError as e:
        return {"error": f"Connection failed: {e}"}

    if "error" in body:
        return {"error": body["error"]}
    try:
        text = body["result"]["content"][0]["text"]
        return json.loads(text)
    except (json.JSONDecodeError, TypeError, KeyError):
        return {"raw": text if "text" in dir() else str(body)}


def http_get_json(ip, path, token=None):
    """Simple GET returning parsed JSON."""
    try:
        protocol = "https" if token else "http"
        url = f"{protocol}://{ip}{path}"
        req = urllib.request.Request(url)
        if token:
            req.add_header("Authorization", f"Bearer {token}")
        ctx = ssl._create_unverified_context() if token else None
        with urllib.request.urlopen(req, timeout=5, context=ctx) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        return {"error": str(e)}


def fmt_bytes(b):
    if b >= 1048576:
        return f"{b / 1048576:.1f} MB"
    if b >= 1024:
        return f"{b / 1024:.1f} KB"
    return f"{b} B"


def pct(value, total):
    if total == 0:
        return "N/A"
    return f"{value * 100 / total:.0f}%"


def run_diagnostics(ip, token=None):
    sep = "=" * 50
    print(f"\n{sep}")
    print(f"  DEVICE DIAGNOSTICS: {ip}")
    print(f"{sep}\n")

    # --- 1. MCP Server Status ---
    print(">>> MCP SERVER STATUS")
    status = http_get_json(ip, "/api/mcp/status", token)
    if "error" in status:
        print(f"    [FAIL] {status['error']}\n")
    else:
        enabled = status.get("enabled", False)
        read_only = status.get("readOnly", True)
        init = status.get("initialized", False)
        tools = status.get("toolCount", 0)
        print(f"    Enabled:     {'YES' if enabled else 'NO'}")
        print(f"    Read-Only:   {'YES' if read_only else 'NO'}")
        print(f"    Initialized: {'YES' if init else 'NO'}")
        print(f"    Tools:       {tools}\n")

    # --- 2. System Info ---
    print(">>> SYSTEM INFO (system_info)")
    info = mcp_request(ip, "system_info", token=token)
    if "error" in info:
        print(f"    [FAIL] {info['error']}\n")
    elif "raw" in info:
        print(f"    {info['raw']}\n")
    else:
        # Memory
        heap_free = info.get("heap_free", 0)
        heap_total = info.get("heap_total", 0)
        heap_min = info.get("heap_min_free", 0)
        heap_max_blk = info.get("heap_max_alloc", 0)
        psram_free = info.get("psram_free", 0)
        psram_total = info.get("psram_total", 0)
        stor_used = info.get("storage_used", 0)
        stor_total = info.get("storage_total", 0)

        print(f"    --- Memory ---")
        print(f"    DRAM Free:       {fmt_bytes(heap_free)} / {fmt_bytes(heap_total)} ({pct(heap_free, heap_total)})")
        print(f"    DRAM Min Free:   {fmt_bytes(heap_min)} ({pct(heap_min, heap_total)})")
        print(f"    Largest Block:   {fmt_bytes(heap_max_blk)}")
        if psram_total > 0:
            print(f"    PSRAM Free:      {fmt_bytes(psram_free)} / {fmt_bytes(psram_total)} ({pct(psram_free, psram_total)})")
        else:
            print(f"    PSRAM:           Not available")
        stor_free = stor_total - stor_used
        print(f"    Storage Free:    {fmt_bytes(stor_free)} / {fmt_bytes(stor_total)} ({pct(stor_free, stor_total)})")

        # Device
        print(f"\n    --- Device ---")
        print(f"    Chip:            {info.get('chip_model', 'N/A')} rev {info.get('chip_revision', '?')} ({info.get('chip_cores', '?')} cores)")
        print(f"    Flash:           {fmt_bytes(info.get('flash_size', 0))}")
        print(f"    Temperature:     {info.get('chip_temp', 0):.1f}°C")
        print(f"    Uptime:          {info.get('uptime_seconds', 0)}s")
        print(f"    Partition:       {info.get('partition', 'N/A')}")

        # Firmware
        print(f"\n    --- Firmware ---")
        print(f"    Version:         {info.get('firmware_version', 'N/A')}")
        print(f"    Build:           {info.get('build_serial', 'N/A')}")
        print(f"    SDK:             {info.get('sdk_version', 'N/A')}")

        # Network
        print(f"\n    --- Network ---")
        ap = info.get("ap_mode", False)
        print(f"    Mode:            {'AP' if ap else 'Station'}")
        print(f"    SSID:            {info.get('wifi_ssid', 'N/A')}")
        print(f"    IP:              {info.get('wifi_ip', 'N/A')}")
        print(f"    MAC:             {info.get('wifi_mac', 'N/A')}")
        rssi = info.get("wifi_rssi", 0)
        if not ap and rssi != 0:
            if rssi >= -50:
                quality = "Excellent"
            elif rssi >= -60:
                quality = "Good"
            elif rssi >= -70:
                quality = "Fair"
            else:
                quality = "Weak"
            print(f"    RSSI:            {rssi} dBm ({quality})")
        print()

    # --- 3. Device Configuration ---
    print(">>> DEVICE CONFIGURATION (get_config)")
    config = mcp_request(ip, "get_config", token=token)
    if "error" in config:
        print(f"    [FAIL] {config['error']}\n")
    elif "raw" in config:
        print(f"    {config['raw']}\n")
    else:
        for ns, values in config.items():
            if isinstance(values, dict):
                print(f"    [{ns}]")
                for k, v in values.items():
                    print(f"      {k}: {v}")
            else:
                print(f"    {ns}: {values}")
        print()

    # --- 4. Registered Tools ---
    print(">>> REGISTERED TOOLS (list_tools)")
    tools_resp = mcp_request(ip, "list_tools", token=token)
    if "error" in tools_resp:
        print(f"    [FAIL] {tools_resp['error']}\n")
    elif "raw" in tools_resp:
        print(f"    {tools_resp['raw']}\n")
    else:
        tool_list = tools_resp.get("tools", tools_resp if isinstance(tools_resp, list) else [])
        count = tools_resp.get("count", len(tool_list))
        for t in tool_list:
            print(f"    - {t.get('name', '?')}: {t.get('description', '')}")
        print(f"    ({count} tools registered)\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ESP32 App Foundation — Device Diagnostics")
    parser.add_argument("ip", help="Device IP address")
    parser.add_argument("--token", help="Bearer token for TLS authentication")
    args = parser.parse_args()
    run_diagnostics(args.ip, args.token)
