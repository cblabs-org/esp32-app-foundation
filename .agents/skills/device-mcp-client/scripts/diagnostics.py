#!/usr/bin/env python3
"""
Queries ESP32 device diagnostics via the Device MCP Server.

Invokes the system_info and get_config built-in MCP tools to retrieve
telemetry, configuration, and system state.
"""
import sys
import json
import argparse
import urllib.request
import urllib.error


def mcp_request(ip_address, method_name, arguments=None):
    """Send a JSON-RPC 2.0 request to the device MCP server."""
    url = f"http://{ip_address}/api/mcp"
    payload = {
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {"name": method_name, "arguments": arguments or {}},
        "id": 1
    }

    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')

    try:
        with urllib.request.urlopen(req, timeout=5) as response:
            res_body = response.read().decode('utf-8')
    except urllib.error.URLError as e:
        return {"error": f"Connection Failed: {e}"}

    try:
        res_json = json.loads(res_body)
        if "error" in res_json:
            return {"error": res_json['error']}

        inner_text = res_json['result']['content'][0]['text']
        # Telemetry and config endpoints return stringified JSON in the text field
        try:
            return json.loads(inner_text)
        except (json.JSONDecodeError, TypeError):
            return {"raw": inner_text}
    except Exception as e:
        return {"error": f"Parse Error: {e}\nRaw Response: {res_body[:200]}"}


def run_diagnostics(ip_address):
    print(f"==========================================")
    print(f" HARDWARE DIAGNOSTICS: {ip_address}")
    print(f"==========================================\n")

    # 1. System Info
    print(">>> 1. SYSTEM INFO (system_info)")
    sys_info = mcp_request(ip_address, "system_info")
    if "error" in sys_info:
        print(f"    [FAIL] {sys_info['error']}\n")
    elif "raw" in sys_info:
        print(f"    {sys_info['raw']}\n")
    else:
        print(f"    - Free Heap: {sys_info.get('freeHeap', sys_info.get('heap', 0)) / 1024:.2f} KB")
        print(f"    - Min Free Heap: {sys_info.get('minFreeHeap', sys_info.get('min_free', 0)) / 1024:.2f} KB")
        print(f"    - Uptime: {sys_info.get('uptimeMs', sys_info.get('uptime', 0))} ms")
        print(f"    - SDK Version: {sys_info.get('sdkVersion', 'N/A')}")
        print(f"    - App Version: {sys_info.get('appVersion', 'N/A')}")
        print(f"    - Chip Model: {sys_info.get('chipModel', 'N/A')}")
        print(f"    - CPU Freq: {sys_info.get('cpuFreqMhz', 'N/A')} MHz\n")

    # 2. Configuration
    print(">>> 2. DEVICE CONFIGURATION (get_config)")
    config = mcp_request(ip_address, "get_config")
    if "error" in config:
        print(f"    [FAIL] {config['error']}\n")
    elif "raw" in config:
        print(f"    {config['raw']}\n")
    else:
        # Pretty-print the full configuration tree
        for key, value in config.items():
            if isinstance(value, dict):
                print(f"    [{key}]")
                for k, v in value.items():
                    print(f"      - {k}: {v}")
            else:
                print(f"    - {key}: {value}")
        print()

    # 3. Available Tools (list what the device exposes)
    print(">>> 3. AVAILABLE MCP TOOLS (list_tools)")
    tools = mcp_request(ip_address, "list_tools")
    if "error" in tools:
        print(f"    [FAIL] {tools['error']}\n")
    elif "raw" in tools:
        print(f"    {tools['raw']}\n")
    else:
        if isinstance(tools, list):
            for tool in tools:
                name = tool.get('name', 'unknown')
                desc = tool.get('description', '')
                print(f"    - {name}: {desc}")
        else:
            print(f"    {json.dumps(tools, indent=4)}")
        print()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Query ESP32 diagnostic tree via MCP")
    parser.add_argument("ip", help="Target ESP32 IP Address")
    args = parser.parse_args()

    run_diagnostics(args.ip)
