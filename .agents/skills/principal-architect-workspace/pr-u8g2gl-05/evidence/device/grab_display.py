#!/usr/bin/env python3
"""Fetch the live U8G2 frame from a running foundation device as a PNG.

Usage: grab_display.py <device-ip> <output.png>

The device's MCP endpoint (POST /api/mcp, tool `capture_display`) returns the
frame as a base64 PNG inside a JSON-RPC text result. The framework-expert skill's
captureU8g2.py still expects the older raw tile buffer and fails on it.

The capture reads the framebuffer while the carousel may be redrawing it, so an
occasional frame comes back torn at 8-row tile boundaries. The physical panel
does not tear (owner, 2026-10-10). Take several captures and use a whole one.
"""
import base64
import json
import sys
import urllib.request

from PIL import Image


def main():
    ip, out = sys.argv[1], sys.argv[2]
    payload = {"jsonrpc": "2.0", "method": "tools/call",
               "params": {"name": "capture_display", "arguments": {}}, "id": 1}
    req = urllib.request.Request(f"http://{ip}/api/mcp", data=json.dumps(payload).encode(),
                                 headers={"Content-Type": "application/json"}, method="POST")
    body = json.loads(urllib.request.urlopen(req, timeout=10).read())
    inner = json.loads(body["result"]["content"][0]["text"])
    with open(out, "wb") as f:
        f.write(base64.b64decode(inner["image"]))
    im = Image.open(out).convert("1")
    lit = sum(1 for y in range(im.height) for x in range(im.width) if im.getpixel((x, y)))
    print(f"{out} {im.size[0]}x{im.size[1]} lit={lit}")


if __name__ == "__main__":
    main()
