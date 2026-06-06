#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Script: scripts/generate_manifest.py
Description: Generates a manifest.json for OTA firmware distribution.
             For each board, it computes the SHA-256 hash and file size of the
             firmware binary, then assembles a manifest matching the project's
             OTA manifest schema.

Usage:
    python3 scripts/generate_manifest.py \
        --version v3.1.0 \
        --channel stable \
        --security-version 1 \
        --boards esp32dev,esp32s3nano \
        --firmware-dir ./release/

    # Write to a file instead of stdout:
    python3 scripts/generate_manifest.py \
        --version v3.1.0 \
        --channel stable \
        --security-version 1 \
        --boards esp32dev,esp32s3nano \
        --firmware-dir ./release/ \
        --output manifest.json

Prerequisites: Python 3.8+
"""

import argparse
import hashlib
import json
import os
import sys


def sha256_file(path: str) -> str:
    """Compute the SHA-256 hex digest of a file."""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def build_manifest(
    version: str,
    channel: str,
    security_version: int,
    boards: list[str],
    firmware_dir: str,
) -> dict:
    """Build the manifest dictionary for the given boards."""
    assets = []
    for board in boards:
        bin_name = f"firmware-{board}.bin"
        bin_path = os.path.join(firmware_dir, bin_name)

        if not os.path.isfile(bin_path):
            print(f"ERROR: firmware binary not found: {bin_path}", file=sys.stderr)
            sys.exit(1)

        size = os.path.getsize(bin_path)
        sha256 = sha256_file(bin_path)

        assets.append(
            {
                "board": board,
                "firmware": bin_name,
                "signature": f"{bin_name}.sig",
                "size": size,
                "sha256": sha256,
            }
        )

    return {
        "version": version,
        "channel": channel,
        "securityVersion": security_version,
        "assets": assets,
    }


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate OTA firmware manifest.json"
    )
    parser.add_argument(
        "--version",
        required=True,
        help="Release version string (e.g. v3.1.0)",
    )
    parser.add_argument(
        "--channel",
        required=True,
        choices=["stable", "beta", "dev", "rc"],
        help="Release channel",
    )
    parser.add_argument(
        "--security-version",
        required=True,
        type=int,
        help="Anti-rollback security version counter",
    )
    parser.add_argument(
        "--boards",
        required=True,
        help="Comma-separated list of board names (e.g. esp32dev,esp32s3nano)",
    )
    parser.add_argument(
        "--firmware-dir",
        required=True,
        help="Directory containing firmware-{board}.bin files",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output file path (default: stdout)",
    )
    return parser.parse_args()


def main() -> None:
    """Entry point."""
    args = parse_args()

    boards = [b.strip() for b in args.boards.split(",") if b.strip()]
    if not boards:
        print("ERROR: --boards must specify at least one board", file=sys.stderr)
        sys.exit(1)

    manifest = build_manifest(
        version=args.version,
        channel=args.channel,
        security_version=args.security_version,
        boards=boards,
        firmware_dir=args.firmware_dir,
    )

    manifest_json = json.dumps(manifest, indent=2) + "\n"

    if args.output:
        with open(args.output, "w") as f:
            f.write(manifest_json)
        print(f"Manifest written to {args.output}", file=sys.stderr)
    else:
        sys.stdout.write(manifest_json)


if __name__ == "__main__":
    main()
