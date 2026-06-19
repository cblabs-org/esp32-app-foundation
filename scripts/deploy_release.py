#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Script: scripts/deploy_release.py
Description: Signs firmware binaries, generates a manifest.json, creates a
             GitHub Release, and uploads all release assets.

             Two source modes:
               CI mode (default): Downloads unsigned binaries from a GitHub
                 Actions workflow run, then signs them locally. The private key
                 never touches CI.
               Local mode (--local): Uses binaries already present in
                 ARTIFACTS_DIR (./release/). Use this after a local build.

Usage:
    # CI mode — download from Actions, sign, publish:
    python3 scripts/deploy_release.py \\
        --tag v3.1.0 \\
        --run-id 12345678 \\
        --private-key private_key.pem

    # Local mode — sign locally-built binaries and publish:
    python3 scripts/deploy_release.py \\
        --tag v3.1.0 \\
        --local \\
        --private-key private_key.pem

    # Local mode with build step — build, sign, and publish in one command:
    python3 scripts/deploy_release.py \\
        --tag v3.1.0 \\
        --local --build \\
        --private-key private_key.pem

    # Dry-run (prints what would happen, without executing):
    python3 scripts/deploy_release.py \\
        --tag v3.1.0 --local --private-key private_key.pem --dry-run

Prerequisites:
    - gh CLI (authenticated): https://cli.github.com
    - openssl
    - Python 3.8+
    - scripts/generate_manifest.py (in the same directory)
    - PlatformIO (only required with --build)
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

# Release staging directory
ARTIFACTS_DIR = "./release"


def check_prerequisites(private_key: str, need_pio: bool) -> None:
    """Verify that required tools and files are available."""
    if not shutil.which("gh"):
        print("ERROR: 'gh' CLI not found. Install from https://cli.github.com", file=sys.stderr)
        sys.exit(1)
    if not shutil.which("openssl"):
        print("ERROR: 'openssl' not found. Install OpenSSL.", file=sys.stderr)
        sys.exit(1)
    if not os.path.isfile(private_key):
        print(f"ERROR: Private key not found: {private_key}", file=sys.stderr)
        sys.exit(1)
    if need_pio and not shutil.which("pio"):
        print("ERROR: 'pio' not found. Install PlatformIO.", file=sys.stderr)
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


def ensure_tag_on_head(tag: str, dry_run: bool) -> None:
    """Ensure the given tag points at HEAD, creating or moving it as needed.

    This makes the deploy script the single owner of the tag lifecycle:
    - If the tag does not exist locally: creates it on HEAD.
    - If the tag exists and already points at HEAD: no-op (logs confirmation).
    - If the tag exists but points elsewhere: moves it to HEAD with a warning
      (safe when the difference is only agent/PA workspace commits with no
      firmware code changes).

    After ensuring the local tag is correct, pushes it to origin with
    ``--force`` so the subsequent ``gh release create`` always succeeds.

    Must be called before any PlatformIO build step so that inject_version.py
    picks up the tag from the exact HEAD that will be compiled.
    """
    print("\n--- Step 0: Ensure git tag is on HEAD ---")

    # Get current HEAD commit hash
    try:
        head = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True, text=True, check=True, timeout=5,
        ).stdout.strip()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        print("  WARNING: Could not resolve HEAD — skipping tag check", file=sys.stderr)
        return

    # Check where the tag currently points (empty string if it doesn't exist)
    tag_commit = ""
    try:
        result = subprocess.run(
            ["git", "rev-parse", f"refs/tags/{tag}"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0:
            tag_commit = result.stdout.strip()
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    if not tag_commit:
        print(f"  Tag '{tag}' does not exist — creating on HEAD ({head[:12]})")
        if not dry_run:
            subprocess.run(["git", "tag", tag], check=True)
    elif tag_commit == head:
        print(f"  Tag '{tag}' already on HEAD ({head[:12]}) — no change needed")
    else:
        print(
            f"  WARNING: Tag '{tag}' points to {tag_commit[:12]} but HEAD is {head[:12]}. "
            f"Moving tag to HEAD (safe if the difference is only non-firmware commits).",
            file=sys.stderr,
        )
        if not dry_run:
            subprocess.run(["git", "tag", "-f", tag], check=True)

    # Push tag to origin (force in case it was moved)
    print(f"  Pushing tag '{tag}' to origin...")
    if not dry_run:
        subprocess.run(["git", "push", "origin", "--force", f"{tag}"], check=True)
    else:
        print(f"  [DRY-RUN] git push origin --force {tag}")
    print(f"  Tag '{tag}' is confirmed on origin.")


def build_firmware(dry_run: bool) -> None:
    """Build firmware for all boards with PlatformIO."""
    print("\n--- Step 0: Build firmware (local) ---")
    for board in BOARDS:
        run(["pio", "run", "-e", board], dry_run=dry_run)


def collect_local_binaries(dry_run: bool) -> None:
    """Copy locally-built .bin files into ARTIFACTS_DIR."""
    print("\n--- Step 1: Collect local binaries ---")
    os.makedirs(ARTIFACTS_DIR, exist_ok=True)
    for board in BOARDS:
        src = os.path.join(".pio", "build", board, "firmware.bin")
        dst = os.path.join(ARTIFACTS_DIR, f"firmware-{board}.bin")
        if not dry_run:
            if not os.path.isfile(src):
                print(f"ERROR: Built binary not found: {src}\n"
                      f"       Run 'pio run -e {board}' first, or use --build.",
                      file=sys.stderr)
                sys.exit(1)
            shutil.copy2(src, dst)
            print(f"  Copied {src} → {dst}")
        else:
            print(f"  [DRY-RUN] copy {src} → {dst}")


def download_artifacts(run_id: str, dry_run: bool) -> None:
    """Download CI build artifacts from GitHub Actions."""
    print("\n--- Step 1: Download CI artifacts ---")
    # Clean previous artifacts
    if os.path.isdir(ARTIFACTS_DIR) and not dry_run:
        shutil.rmtree(ARTIFACTS_DIR)

    run(["gh", "run", "download", run_id, "--dir", ARTIFACTS_DIR], dry_run=dry_run)

    if not dry_run:
        # gh run download creates subdirectories per artifact — flatten them
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
            ["openssl", "dgst", "-sha256", "-sign", private_key, "-out", sig_file, bin_file],
            dry_run=dry_run,
        )


def generate_hashes(dry_run: bool) -> None:
    """Generate SHA-256 hash files for each firmware binary."""
    print("\n--- Step 3a: Generate SHA-256 hashes ---")
    for board in BOARDS:
        bin_file = os.path.join(ARTIFACTS_DIR, f"firmware-{board}.bin")
        sha_file = os.path.join(ARTIFACTS_DIR, f"firmware-{board}.bin.sha256")
        if not dry_run:
            result = subprocess.run(
                ["sha256sum", bin_file], capture_output=True, text=True, check=True
            )
            with open(sha_file, "w") as f:
                f.write(result.stdout)
            print(f"  {sha_file}: {result.stdout.split()[0]}")
        else:
            print(f"  [DRY-RUN] sha256sum {bin_file} > {sha_file}")


def generate_manifest(tag: str, dry_run: bool) -> None:
    """Generate the OTA manifest.json."""
    print("\n--- Step 3b: Generate manifest.json ---")
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


def create_release(tag: str, dry_run: bool, target_repo: str = None) -> None:
    """Create a GitHub Release and upload all assets."""
    print("\n--- Step 4: Create GitHub Release ---")
    channel = parse_channel(tag)
    release_cmd = ["gh", "release", "create", tag, "--title", tag, "--generate-notes"]
    if channel != "stable":
        release_cmd.append("--prerelease")
    if target_repo:
        release_cmd.extend(["--repo", target_repo])
    run(release_cmd, dry_run=dry_run)

    print("\n--- Step 5: Upload release assets ---")
    asset_patterns = ["*.bin", "*.bin.sig", "*.bin.sha256", "manifest.json"]
    assets = []
    for pattern in asset_patterns:
        assets.extend(glob.glob(os.path.join(ARTIFACTS_DIR, pattern)))

    if not assets and not dry_run:
        print("ERROR: No assets found to upload", file=sys.stderr)
        sys.exit(1)

    upload_cmd = ["gh", "release", "upload", tag]
    if target_repo:
        upload_cmd.extend(["--repo", target_repo])
    upload_cmd.extend(assets)
    run(upload_cmd, dry_run=dry_run)


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Sign firmware binaries and create a GitHub Release",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--tag", required=True, help="Git tag for the release (e.g. v3.1.0)")
    parser.add_argument("--private-key", required=True, help="Path to ECDSA private key")

    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument(
        "--run-id",
        help="GitHub Actions workflow run ID to download artifacts from (CI mode)",
    )
    source.add_argument(
        "--local",
        action="store_true",
        help="Use locally-built binaries from .pio/build/ (local mode)",
    )

    parser.add_argument(
        "--build",
        action="store_true",
        help="(Local mode only) Run 'pio run' for all boards before staging",
    )
    parser.add_argument(
        "--repo",
        help="Target GitHub repo for the release (owner/repo format, "
             "e.g. 'matthewmcneill/esp32-firmware-releases'). "
             "Defaults to the current repo's origin.",
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

    if args.build and args.run_id:
        print("ERROR: --build is only valid with --local", file=sys.stderr)
        sys.exit(1)

    mode = "local" if args.local else f"CI (run {args.run_id})"
    print(f"=== Deploy Release: {args.tag} ===")
    print(f"  Mode:        {mode}")
    print(f"  Private key: {args.private_key}")
    print(f"  Dry-run:     {args.dry_run}")
    if args.repo:
        print(f"  Target repo: {args.repo}")

    check_prerequisites(args.private_key, need_pio=args.build)
    ensure_tag_on_head(args.tag, args.dry_run)

    if args.local:
        if args.build:
            build_firmware(args.dry_run)
        collect_local_binaries(args.dry_run)
    else:
        download_artifacts(args.run_id, args.dry_run)

    sign_firmware(args.private_key, args.dry_run)
    generate_hashes(args.dry_run)
    generate_manifest(args.tag, args.dry_run)
    create_release(args.tag, args.dry_run, target_repo=args.repo)

    print("\n✅ Release deployed successfully!")


if __name__ == "__main__":
    main()
