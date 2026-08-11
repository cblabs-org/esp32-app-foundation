#!/usr/bin/env python3
"""
Discovers the ESP32 device IP by parsing PlatformIO monitor logs.

Reads the latest log file from .pio-mcp-workspace/logs/monitor/ (sorted
by filename — newest timestamp last) and searches for WiFi connection
messages containing the assigned IP address.
"""
import glob
import os
import re
import sys


def discover_ip():
    """
    Parses the latest monitor log from .pio-mcp-workspace/logs/monitor/
    to locate the dynamically assigned DHCP IP address from the ESP32's
    boot sequence.
    """
    log_dir = ".pio-mcp-workspace/logs/monitor"

    if not os.path.isdir(log_dir):
        print(f"ERROR: Monitor log directory not found at {log_dir}", file=sys.stderr)
        sys.exit(1)

    # Find all monitor-*.log files and sort by name (timestamp in filename)
    log_files = sorted(glob.glob(os.path.join(log_dir, "monitor-*.log")))

    if not log_files:
        print(f"ERROR: No monitor log files found in {log_dir}", file=sys.stderr)
        sys.exit(1)

    # Use the latest file (last after sorting by timestamp-based filename)
    log_path = log_files[-1]
    print(f"Reading log: {os.path.basename(log_path)}", file=sys.stderr)

    with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    # Regex to match private IPv4 addresses
    private_ip_pattern = re.compile(
        r"(?:192\.168\.\d{1,3}\.\d{1,3}|10\.\d{1,3}\.\d{1,3}\.\d{1,3}"
        r"|172\.(?:1[6-9]|2\d|3[0-1])\.\d{1,3}\.\d{1,3})"
    )

    # Search backwards for the most recent IP assignment
    for line in reversed(lines):
        # Match "Connected successfully. IP: x.x.x.x" or "sta ip: x.x.x.x"
        if ("Connected successfully" in line and "IP:" in line) or "sta ip:" in line:
            matches = private_ip_pattern.findall(line)
            if matches:
                ip = matches[-1]
                # Skip AP-mode addresses (192.168.4.x)
                if not ip.startswith("192.168.4."):
                    print(ip)
                    return ip

    print("ERROR: No valid private IP address found in logs.", file=sys.stderr)
    sys.exit(1)


if __name__ == "__main__":
    discover_ip()
