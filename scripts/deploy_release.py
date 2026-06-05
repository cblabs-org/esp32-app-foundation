#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Script: scripts/deploy_release.py
Description: Downloads CI build artifacts, signs each firmware binary locally,
             generates a manifest.json, creates a GitHub Release, and uploads
             all release assets.

Usage:
    python3 scripts/deploy_release.py \\
        --tag v3.1.0 \\
        --run-id 12345678 \\
        --private-key private_key.pem

    # Dry-run mode (prints what would happen, without executing):
    python3 scripts/deploy_release.py \\
        --tag v3.1.0 \\
        --run-id 12345678 \\
        --private-key private_key.pem \\
        --dry-run

Prerequisites:
    - gh CLI (authenticated): https://cli.github.com
    - openssl
    - Python 3.8+
    - scripts/generate_manifest.py (in the same directory)
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys

# Board list matching the CI matrix
BOARDS = ["esp32dev", "esp32s3nano"]

# Artifacts download directory
ARTIFACTS_DIR = "./artifacts"


def check_prerequisites(private_key: str) -> None:
    """Verify that required tools and files are available."""
    # Check gh CLI
    if not shutil.which("gh"):
        print("ERROR: 'gh' CLI not found. Install from https://cli.github.com", file=sys.stderr)
        sys.exit(1)

    # Check openssl
    if not shutil.which("openssl"):
        print("ERROR: 'openssl' not found. Install OpenSSL.", file=sys.stderr)
        sys.exit(1)

    # Check private key
    if not os.path.isfile(private_key):
        print(f"ERROR: Private key not found: {private_key}", file=sys.stderr)
        sys.exit(1)


def run(cmd: list[str], dry_run: bool = False, **kwargs) -> subprocess.CompletedProcess:
    """Run a command, or print it in dry-run mode."""
    if dry_run:
        print(f"  [DRY-RUN] {' '.join(cmd)}")
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")

    print(f"  $ {' '.join(cmd)}")
    result = subprocess.run(cmd, check=True, **kwargs)
    return result


def parse_channel(tag: str) -> str:
    """Extract the release channel from a version tag."""
    pattern = r"^v?\d+\.\d+\.\d+(?:-(dev|beta|rc)(?:\.\d+)?)?$"
    m = re.match(pattern, tag)
    if not m:
        print(f"WARNING: Tag '{tag}' doesn't match SemVer pattern, defaulting to 'stable'",
              file=sys.stderr)
        return "stable"
    return m.group(1) or "stable"


def download_artifacts(run_id: str, dry_run: bool) -> None:
    """Download CI build artifacts from GitHub Actions."""
    print("\n--- Step 1: Download CI artifacts ---")
    # Clean previous artifacts
    if os.path.isdir(ARTIFACTS_DIR) and not dry_run:
        shutil.rmtree(ARTIFACTS_DIR)

    run(["gh", "run", "download", run_id, "--dir", ARTIFACTS_DIR], dry_run=dry_run)

    if not dry_run:
        # gh run download creates subdirectories per artifact name — flatten them
        for subdir in glob.glob(os.path.join(ARTIFACTS_DIR, "*")):
            if os.path.isdir(subdir):
                for file in glob.glob(os.path.join(subdir, "*")):
                    dest = os.path.join(ARTIFACTS_DIR, os.path.basename(file))
                    shutil.move(file, dest)
                os.rmdir(subdir)


def sign_firmware(private_key: str, dry_run: bool) -> None:
    """Sign each board's firmware binary with the private key."""
    print("\n--- Step 2: Sign firmware binaries ---")
    for board in BOARDS:
        bin_file = os.path.join(ARTIFACTS_DIR, f"firmware-{board}.bin")
        sig_file = os.path.join(ARTIFACTS_DIR, f"firmware-{board}.bin.sig")

        if not dry_run and not os.path.isfile(bin_file):
            print(f"ERROR: Firmware binary not found: {bin_file}", file=sys.stderr)
            sys.exit(1)

        run(
            [
                "openssl", "dgst", "-sha256",
                "-sign", private_key,
                "-out", sig_file,
                bin_file,
            ],
            dry_run=dry_run,
        )


def generate_manifest(tag: str, dry_run: bool) -> None:
    """Generate the OTA manifest.json."""
    print("\n--- Step 3: Generate manifest.json ---")
    channel = parse_channel(tag)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    manifest_script = os.path.join(script_dir, "generate_manifest.py")

    run(
        [
            sys.executable, manifest_script,
            "--version", tag,
            "--channel", channel,
            "--security-version", "1",
            "--boards", ",".join(BOARDS),
            "--firmware-dir", ARTIFACTS_DIR,
            "--output", os.path.join(ARTIFACTS_DIR, "manifest.json"),
        ],
        dry_run=dry_run,
    )


def create_release(tag: str, dry_run: bool) -> None:
    """Create a GitHub Release and upload all assets."""
    print("\n--- Step 4: Create GitHub Release ---")
    run(
        ["gh", "release", "create", tag, "--title", tag, "--generate-notes"],
        dry_run=dry_run,
    )

    print("\n--- Step 5: Upload release assets ---")
    # Collect all release files
    asset_patterns = ["*.bin", "*.bin.sig", "*.bin.sha256", "manifest.json"]
    assets = []
    for pattern in asset_patterns:
        assets.extend(glob.glob(os.path.join(ARTIFACTS_DIR, pattern)))

    if not assets and not dry_run:
        print("ERROR: No assets found to upload", file=sys.stderr)
        sys.exit(1)

    run(["gh", "release", "upload", tag] + assets, dry_run=dry_run)


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Sign CI artifacts and create a GitHub Release"
    )
    parser.add_argument(
        "--tag",
        required=True,
        help="Git tag for the release (e.g. v3.1.0)",
    )
    parser.add_argument(
        "--run-id",
        required=True,
        help="GitHub Actions workflow run ID to download artifacts from",
    )
    parser.add_argument(
        "--private-key",
        required=True,
        help="Path to RSA private key (e.g. private_key.pem)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without executing them",
    )
    return parser.parse_args()


def main() -> None:
    """Entry point."""
    args = parse_args()

    print(f"=== Deploy Release: {args.tag} ===")
    print(f"  Run ID:      {args.run_id}")
    print(f"  Private key: {args.private_key}")
    print(f"  Dry-run:     {args.dry_run}")

    check_prerequisites(args.private_key)

    download_artifacts(args.run_id, args.dry_run)
    sign_firmware(args.private_key, args.dry_run)
    generate_manifest(args.tag, args.dry_run)
    create_release(args.tag, args.dry_run)

    print("\n✅ Release deployed successfully!")


if __name__ == "__main__":
    main()
