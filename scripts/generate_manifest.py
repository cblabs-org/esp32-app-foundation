#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Script: scripts/generate_manifest.py
Description: Generates a v2 manifest.json for OTA firmware distribution.
             Produces the multi-channel manifest schema with absolute download
             URLs, manifestVersion, and self-referencing manifestUrl. For each
             board, it computes the SHA-256 hash and file size of the firmware
             binary, then assembles assets with absolute firmwareUrl and
             signatureUrl fields.

Usage:
    # Single channel:
    python3 scripts/generate_manifest.py \
        --version v3.1.0 \
        --channel stable \
        --security-version 1 \
        --boards esp32dev,esp32s3nano \
        --firmware-dir ./release/ \
        --base-url https://github.com/owner/repo/releases/download/v3.1.0 \
        --manifest-url https://github.com/owner/repo/releases/download/ota-manifest/manifest.json

    # Multiple channels (comma-separated --channels with matching --versions):
    python3 scripts/generate_manifest.py \
        --channels stable,dev \
        --versions v3.1.0,v3.2.0-dev.1 \
        --security-versions 1,1 \
        --boards esp32dev,esp32s3nano \
        --firmware-dirs ./release-stable/,./release-dev/ \
        --base-urls https://github.com/.../download/v3.1.0,https://github.com/.../download/v3.2.0-dev.1 \
        --manifest-url https://github.com/owner/repo/releases/download/ota-manifest/manifest.json

    # Write to a file instead of stdout:
    python3 scripts/generate_manifest.py \
        --version v3.1.0 \
        --channel stable \
        --security-version 1 \
        --boards esp32dev,esp32s3nano \
        --firmware-dir ./release/ \
        --base-url https://github.com/owner/repo/releases/download/v3.1.0 \
        --manifest-url https://github.com/owner/repo/releases/download/ota-manifest/manifest.json \
        --output manifest.json

    # Merge into an existing manifest (incremental multi-channel build):
    python3 scripts/generate_manifest.py \
        --version v3.2.0-dev.1 \
        --channel dev \
        --security-version 1 \
        --boards esp32dev,esp32s3nano \
        --firmware-dir ./release/ \
        --base-url https://github.com/owner/repo/releases/download/v3.2.0-dev.1 \
        --manifest-url https://github.com/owner/repo/releases/download/ota-manifest/manifest.json \
        --merge-into manifest.json \
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


def build_channel_entry(
    version: str,
    security_version: int,
    boards: list[str],
    base_url: str,
    firmware_dir: str = None,
) -> dict:
    """Build a single channel entry with absolute asset URLs.

    Args:
        version: Release version tag for this channel.
        security_version: Anti-rollback security version counter.
        boards: List of board identifiers (e.g. ["esp32dev", "esp32s3nano"]).
        base_url: URL prefix for firmware/signature download URLs.
        firmware_dir: Local directory containing firmware-{board}.bin files.
                      When None, size and sha256 are omitted from assets
                      (the device never reads these fields — ECDSA signature
                      provides integrity verification).

    Returns:
        Channel entry dict with version, securityVersion, and assets array.
    """
    # Ensure base_url has no trailing slash for clean URL joining
    base_url = base_url.rstrip("/")

    assets = []
    for board in boards:
        bin_name = f"firmware-{board}.bin"

        asset = {
            "board": board,
            "firmwareUrl": f"{base_url}/{bin_name}",
            "signatureUrl": f"{base_url}/{bin_name}.sig",
        }

        # Include size and sha256 only when firmware binaries are available
        if firmware_dir is not None:
            bin_path = os.path.join(firmware_dir, bin_name)
            if not os.path.isfile(bin_path):
                print(f"ERROR: firmware binary not found: {bin_path}", file=sys.stderr)
                sys.exit(1)
            asset["size"] = os.path.getsize(bin_path)
            asset["sha256"] = sha256_file(bin_path)

        assets.append(asset)

    return {
        "version": version,
        "securityVersion": security_version,
        "assets": assets,
    }


def build_manifest(
    manifest_url: str,
    channel_entries: dict[str, dict],
) -> dict:
    """Build the v2 manifest dictionary.

    Args:
        manifest_url: Self-referencing URL for the manifest file.
        channel_entries: Dict mapping channel name to channel entry dict.

    Returns:
        Complete v2 manifest dict ready for JSON serialisation.
    """
    return {
        "manifestVersion": 1,
        "manifestUrl": manifest_url,
        "channels": channel_entries,
    }


def parse_csv(value: str) -> list[str]:
    """Split a comma-separated string into a list of stripped, non-empty values."""
    return [v.strip() for v in value.split(",") if v.strip()]


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate v2 OTA firmware manifest.json with absolute URLs"
    )

    # --- Manifest-level args ---
    parser.add_argument(
        "--manifest-url",
        required=True,
        help="Self-referencing URL for the manifest (e.g. https://github.com/.../ota-manifest/manifest.json)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output file path (default: stdout)",
    )
    parser.add_argument(
        "--merge-into",
        default=None,
        help="Path to an existing manifest.json. If provided, the new channel "
             "entry is merged into the existing manifest instead of creating "
             "from scratch. Missing file is treated as an empty manifest.",
    )

    # --- Single-channel convenience args ---
    single = parser.add_argument_group(
        "single-channel mode",
        "Convenience args for a single channel (use --channel, --version, etc.)"
    )
    single.add_argument(
        "--version",
        default=None,
        help="Release version string (e.g. v3.1.0)",
    )
    single.add_argument(
        "--channel",
        default=None,
        choices=["stable", "beta", "dev", "rc"],
        help="Release channel",
    )
    single.add_argument(
        "--security-version",
        default=None,
        type=int,
        help="Anti-rollback security version counter",
    )
    single.add_argument(
        "--boards",
        default=None,
        help="Comma-separated list of board names (e.g. esp32dev,esp32s3nano)",
    )
    single.add_argument(
        "--firmware-dir",
        default=None,
        help="Directory containing firmware-{board}.bin files",
    )
    single.add_argument(
        "--base-url",
        default=None,
        help="URL prefix for firmware/signature asset URLs",
    )

    # --- Multi-channel args ---
    multi = parser.add_argument_group(
        "multi-channel mode",
        "Args for multiple channels (comma-separated, matched by position)"
    )
    multi.add_argument(
        "--channels",
        default=None,
        help="Comma-separated channel names",
    )
    multi.add_argument(
        "--versions",
        default=None,
        help="Comma-separated version tags (one per channel)",
    )
    multi.add_argument(
        "--security-versions",
        default=None,
        help="Comma-separated security version counters (one per channel)",
    )
    multi.add_argument(
        "--firmware-dirs",
        default=None,
        help="Comma-separated firmware directories (one per channel)",
    )
    multi.add_argument(
        "--base-urls",
        default=None,
        help="Comma-separated base URLs (one per channel)",
    )

    return parser.parse_args()


def main() -> None:
    """Entry point."""
    args = parse_args()

    channel_entries: dict[str, dict] = {}

    # Determine mode: single-channel vs multi-channel
    if args.channels:
        # --- Multi-channel mode ---
        channels = parse_csv(args.channels)
        versions = parse_csv(args.versions) if args.versions else []
        sec_versions = parse_csv(args.security_versions) if args.security_versions else []
        firmware_dirs = parse_csv(args.firmware_dirs) if args.firmware_dirs else []
        base_urls = parse_csv(args.base_urls) if args.base_urls else []
        boards = parse_csv(args.boards) if args.boards else []

        if not boards:
            print("ERROR: --boards must specify at least one board", file=sys.stderr)
            sys.exit(1)

        n = len(channels)
        if len(versions) != n or len(sec_versions) != n or len(base_urls) != n:
            print(
                f"ERROR: --channels ({n}) must have matching count for "
                f"--versions ({len(versions)}), --security-versions ({len(sec_versions)}), "
                f"--base-urls ({len(base_urls)})",
                file=sys.stderr,
            )
            sys.exit(1)
        if firmware_dirs and len(firmware_dirs) != n:
            print(
                f"ERROR: --firmware-dirs ({len(firmware_dirs)}) must match "
                f"--channels ({n}) count when provided",
                file=sys.stderr,
            )
            sys.exit(1)

        for i, ch_name in enumerate(channels):
            channel_entries[ch_name] = build_channel_entry(
                version=versions[i],
                security_version=int(sec_versions[i]),
                boards=boards,
                base_url=base_urls[i],
                firmware_dir=firmware_dirs[i] if firmware_dirs else None,
            )

    elif args.channel:
        # --- Single-channel mode ---
        if not all([args.version, args.security_version is not None, args.boards, args.base_url]):
            print(
                "ERROR: single-channel mode requires --version, --channel, "
                "--security-version, --boards, and --base-url",
                file=sys.stderr,
            )
            sys.exit(1)

        boards = parse_csv(args.boards)
        if not boards:
            print("ERROR: --boards must specify at least one board", file=sys.stderr)
            sys.exit(1)

        channel_entries[args.channel] = build_channel_entry(
            version=args.version,
            security_version=args.security_version,
            boards=boards,
            base_url=args.base_url,
            firmware_dir=args.firmware_dir,
        )

    else:
        print(
            "ERROR: must specify either --channel (single) or --channels (multi)",
            file=sys.stderr,
        )
        sys.exit(1)

    # If --merge-into is specified, load the existing manifest as a base
    if args.merge_into:
        try:
            with open(args.merge_into, "r") as f:
                existing = json.load(f)
            print(f"Loaded existing manifest from {args.merge_into}", file=sys.stderr)
        except FileNotFoundError:
            print(f"No existing manifest at {args.merge_into} — starting fresh", file=sys.stderr)
            existing = {}
        except json.JSONDecodeError as e:
            print(f"WARNING: Could not parse {args.merge_into}: {e} — starting fresh",
                  file=sys.stderr)
            existing = {}

        # Merge: preserve existing channels, overlay new ones
        existing_channels = existing.get("channels", {})
        existing_channels.update(channel_entries)
        channel_entries = existing_channels

    manifest = build_manifest(
        manifest_url=args.manifest_url,
        channel_entries=channel_entries,
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
