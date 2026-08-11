#!/usr/bin/env python3
"""
Captures the ESP32 U8G2 monochrome display buffer via the Device MCP Server
and saves it as a PNG image.

The MCP server exposes capture_display at /api/mcp (JSON-RPC 2.0).
The response contains a U8G2 tile-format monochrome framebuffer (base64-encoded)
along with width, height, and format metadata.
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


def decode_u8g2_monochrome(data, width, height):
    """
    Decodes a U8G2 tile-based monochrome buffer into a Pillow Image.
    Each tile is 8x8 pixels.
    Each byte in a tile is one column (8 vertical pixels).
    Bit 0 is the top pixel of the column.
    Layout: Tiles are stored row-major (Left-to-Right, then Top-to-Bottom).
    """
    img = Image.new('1', (width, height), 0)  # '1' is 1-bit monochrome
    pixels = img.load()

    tiles_x = width // 8
    tiles_y = height // 8

    idx = 0
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            for col in range(8):
                if idx >= len(data):
                    break
                byte = data[idx]
                idx += 1

                for row_bit in range(8):
                    pixel_val = (byte >> row_bit) & 0x01
                    px = (tx * 8) + col
                    py = (ty * 8) + row_bit

                    if px < width and py < height:
                        pixels[px, py] = pixel_val

    return img


def decode_xbm_monochrome(data, width, height):
    """
    Decodes an XBM-format monochrome buffer into a Pillow Image.
    XBM stores pixels row-by-row, LSB first within each byte.
    """
    img = Image.new('1', (width, height), 0)
    pixels = img.load()

    bytes_per_row = (width + 7) // 8
    idx = 0
    for y in range(height):
        for byte_idx in range(bytes_per_row):
            if idx >= len(data):
                break
            byte = data[idx]
            idx += 1
            for bit in range(8):
                x = byte_idx * 8 + bit
                if x < width:
                    pixels[x, y] = (byte >> bit) & 0x01

    return img


def capture_u8g2(ip_address, output_path):
    url = f"http://{ip_address}/api/mcp"

    payload = {
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {"name": "capture_display", "arguments": {}},
        "id": 1
    }

    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='POST')

    print(f"Requesting U8G2 framebuffer from {url}...")
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

        # The response text field is a JSON string with width, height, format, data
        inner_text = res_json['result']['content'][0]['text']
        display_data = json.loads(inner_text)

        width = display_data.get('width', 256)
        height = display_data.get('height', 64)
        fmt = display_data.get('format', 'u8g2-tile')
        b64_str = display_data['data']
    except Exception as e:
        print(f"ERROR: Failed to parse MCP response. Response snippet: {res_body[:200]}", file=sys.stderr)
        sys.exit(1)

    print(f"Decoding {len(b64_str)} chars of Base64 (format={fmt}, {width}x{height})...")
    raw_buffer = base64.b64decode(b64_str)

    buffer_len = len(raw_buffer)
    print(f"Decoded binary framebuffer size: {buffer_len} bytes")
    print(f"Dimensions: {width}x{height}, Format: {fmt}")

    if fmt == "xbm-rowmajor":
        img = decode_xbm_monochrome(raw_buffer, width, height)
    else:
        # U8G2 tile format — default for all U8G2 monochrome displays.
        # Handles both the new "u8g2-tile" discriminator and the legacy
        # "xbm" value from pre-fix firmware.
        img = decode_u8g2_monochrome(raw_buffer, width, height)

    img.save(output_path)
    print(f"Successfully saved U8G2 display capture to: {output_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Capture ESP32 U8G2 Display buffer via MCP")
    parser.add_argument("ip", help="Target ESP32 IP Address")
    parser.add_argument("output", help="Output PNG path", default="display_capture_u8g2.png", nargs="?")
    args = parser.parse_args()

    capture_u8g2(args.ip, args.output)
