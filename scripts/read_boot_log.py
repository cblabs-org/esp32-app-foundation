#!/usr/bin/env python3
"""Reset an ESP32 over its USB-serial DTR/RTS lines and print the boot log.

Usage:
    python3 scripts/read_boot_log.py /dev/cu.usbmodem1101 [--baud 115200] [--seconds 15]

Diagnostic helper for when the PlatformIO MCP serial tools are unavailable.
Do not run it while another tool holds the port.
"""
import argparse
import sys
import time

import serial


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("port", help="Serial port, e.g. /dev/cu.usbmodem1101 or COM3")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--seconds", type=float, default=15, help="How long to capture (default: 15)")
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    with ser:
        # Toggle DTR/RTS to reset the ESP32 (EN low, then release with IO0 high)
        ser.setDTR(False)
        ser.setRTS(True)
        time.sleep(0.1)
        ser.setDTR(True)
        ser.setRTS(False)
        time.sleep(0.1)

        deadline = time.time() + args.seconds
        while time.time() < deadline:
            line = ser.readline()
            if line:
                print(line.decode("utf-8", errors="replace"), end="")
    return 0


if __name__ == "__main__":
    sys.exit(main())
