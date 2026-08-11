#!/usr/bin/env python3
"""
Captures the ESP32 RGB565 colour display buffer via the Device MCP Server
and saves it as a PNG image.

The MCP server exposes capture_display at /api/mcp (JSON-RPC 2.0).
Use this for colour displays. For monochrome U8G2 displays, use captureU8g2.py.
"""
import sys
import json
import base64
import argparse
import urllib.request
import urllib.error

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow library is not installed. Please run: python3 -m pip install Pillow", file=sys.stderr)
    sys.exit(1)


def decode_rgb565(data, width, height):
    """
    Decodes a raw byte array of RGB565 data into a Pillow Image.
    RGB565 stores a pixel in 2 bytes (little-endian on ESP32):
    Low Byte first, High Byte second.
    """
    img = Image.new('RGB', (width, height))
    pixels = img.load()

    idx = 0
    for y in range(height):
        for x in range(width):
            if idx + 1 >= len(data):
                break

            # Little Endian for ESP32 framebuffer
            pixel = (data[idx + 1] << 8) | data[idx]

            # Extract RGB
            r = (pixel >> 11) & 0x1F
            g = (pixel >> 5) & 0x3F
            b = pixel & 0x1F

            # Scale to 8-bit (0-255)
            r = (r * 255) // 31
            g = (g * 255) // 63
            b = (b * 255) // 31

            pixels[x, y] = (r, g, b)
            idx += 2

    return img


def capture_display(ip_address, output_path):
    url = f"http://{ip_address}/api/mcp"

    payload = {
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {"name": "capture_display", "arguments": {}},
        "id": 1
    }

    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')

    print(f"Requesting framebuffer from {url}...")
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            res_body = response.read().decode('utf-8')
    except urllib.error.URLError as e:
        print(f"ERROR: Failed to connect to device: {e}", file=sys.stderr)
        sys.exit(1)

    try:
        res_json = json.loads(res_body)
        if "error" in res_json:
            print(f"RPC Error from device: {res_json['error']}", file=sys.stderr)
            sys.exit(1)

        inner_text = res_json['result']['content'][0]['text']
        display_data = json.loads(inner_text)

        width = display_data.get('width', 256)
        height = display_data.get('height', 64)
        b64_str = display_data['data']
    except Exception as e:
        print(f"ERROR: Failed to parse MCP response. Response snippet: {res_body[:200]}", file=sys.stderr)
        sys.exit(1)

    print(f"Decoding {len(b64_str)} chars of Base64...")
    raw_buffer = base64.b64decode(b64_str)

    buffer_len = len(raw_buffer)
    print(f"Decoded binary framebuffer size: {buffer_len} bytes")
    print(f"Dimensions: {width}x{height}")

    img = decode_rgb565(raw_buffer, width, height)
    img.save(output_path)
    print(f"Successfully saved display capture to: {output_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Capture ESP32 Display buffer via MCP")
    parser.add_argument("ip", help="Target ESP32 IP Address")
    parser.add_argument("output", help="Output PNG path", default="display_capture.png", nargs="?")
    args = parser.parse_args()

    capture_display(args.ip, args.output)
