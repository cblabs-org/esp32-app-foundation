#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Script: scripts/deploy_release.py
Description: Signs firmware binaries, generates and signs a v2 manifest.json,
             creates a versioned GitHub Release, and uploads all release assets.
             After the versioned release, updates the fixed ``ota-manifest``
             release with a merged, signed manifest covering all channels.

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
import json
import os
import re
import shutil
import subprocess
import sys
import warnings

# Board list matching the CI matrix
BOARDS = ["esp32dev", "esp32s3nano"]

# Release staging directory
ARTIFACTS_DIR = "./release"

# Floating tag for the OTA manifest (device fetches from this fixed URL)
OTA_MANIFEST_TAG = "manifest-latest"

# Channel stability ordering: dev(0) < beta(1) < rc(2) < stable(3)
CHANNEL_ORDER = {"dev": 0, "beta": 1, "rc": 2, "stable": 3}


# ---------------------------------------------------------------------------
#  SemVer comparison (mirrors firmware FirmwareVersion logic)
# ---------------------------------------------------------------------------

_SEMVER_RE = re.compile(
    r"^[vV]?(\d+)\.(\d+)\.(\d+)(?:-(dev|beta|rc)(?:\.(\d+))?)?$"
)


class FirmwareVersion:
    """Comparable SemVer representation matching the firmware's SemVer logic.

    Channel ordering: dev(0) < beta(1) < rc(2) < stable(3).
    A stable version (no pre-release suffix) always beats a pre-release
    version with the same major.minor.patch.
    """

    def __init__(self, version_str: str):
        m = _SEMVER_RE.match(version_str.strip())
        if not m:
            raise ValueError(f"Invalid SemVer: {version_str!r}")
        self.major = int(m.group(1))
        self.minor = int(m.group(2))
        self.patch = int(m.group(3))
        chan_str = m.group(4)  # None for stable
        self.channel = CHANNEL_ORDER.get(chan_str, 3)  # 3 = stable
        self.pre_num = int(m.group(5)) if m.group(5) else 0
        self.raw = version_str.strip()

    def _sort_key(self):
        return (self.major, self.minor, self.patch, self.channel, self.pre_num)

    def __lt__(self, other):
        return self._sort_key() < other._sort_key()

    def __le__(self, other):
        return self._sort_key() <= other._sort_key()

    def __gt__(self, other):
        return self._sort_key() > other._sort_key()

    def __ge__(self, other):
        return self._sort_key() >= other._sort_key()

    def __eq__(self, other):
        return self._sort_key() == other._sort_key()

    def __repr__(self):
        return f"FirmwareVersion({self.raw!r})"


def channel_stability_floor(channel_name: str) -> int:
    """Return the numeric stability floor for a channel name.

    A device on channel X should receive an update if the candidate version's
    stability is >= the channel floor.  This means a stable release is always
    a valid candidate for dev/beta/rc channels.
    """
    return CHANNEL_ORDER.get(channel_name, 0)


# ---------------------------------------------------------------------------
#  Utilities
# ---------------------------------------------------------------------------

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


def run(cmd: list[str], dry_run: bool = False, check: bool = True, **kwargs) -> subprocess.CompletedProcess:
    """Run a command, or print it in dry-run mode."""
    if dry_run:
        print(f"  [DRY-RUN] {' '.join(cmd)}")
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")
    print(f"  $ {' '.join(cmd)}")
    result = subprocess.run(cmd, check=check, **kwargs)
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


def get_repo_slug(target_repo: str = None) -> str:
    """Return the repo slug (owner/repo) — from --repo or git remote."""
    if target_repo:
        return target_repo
    try:
        result = subprocess.run(
            ["gh", "repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"],
            capture_output=True, text=True, check=True, timeout=10,
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        print("WARNING: Could not determine repo slug — manifest URLs may be incorrect",
              file=sys.stderr)
        return "UNKNOWN/REPO"


# ---------------------------------------------------------------------------
#  Pipeline steps
# ---------------------------------------------------------------------------

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


def generate_manifest(tag: str, base_url: str, manifest_url: str,
                      dry_run: bool, merge_into: str = None) -> None:
    """Generate the OTA manifest.json for this release's channel.

    Args:
        tag: Version tag (e.g. v3.1.0).
        base_url: Absolute URL prefix for firmware/signature assets.
        manifest_url: Self-referencing manifest URL.
        dry_run: If True, print commands without executing.
        merge_into: Path to existing manifest to merge into, or None.
    """
    print("\n--- Step 3b: Generate manifest.json ---")
    channel = parse_channel(tag)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    manifest_script = os.path.join(script_dir, "generate_manifest.py")

    cmd = [
        sys.executable, manifest_script,
        "--version", tag,
        "--channel", channel,
        "--security-version", "1",
        "--boards", ",".join(BOARDS),
        "--firmware-dir", ARTIFACTS_DIR,
        "--base-url", base_url,
        "--manifest-url", manifest_url,
        "--output", os.path.join(ARTIFACTS_DIR, "manifest.json"),
    ]
    if merge_into:
        cmd.extend(["--merge-into", merge_into])

    run(cmd, dry_run=dry_run)


def sign_manifest(private_key: str, manifest_path: str, dry_run: bool) -> None:
    """Sign the manifest.json with the private key (detached ECDSA signature).

    Args:
        private_key: Path to the ECDSA private key file.
        manifest_path: Path to the manifest.json to sign.
        dry_run: If True, print the command without executing.
    """
    print("\n--- Step 3c: Sign manifest.json ---")
    sig_path = manifest_path + ".sig"
    run(
        ["openssl", "dgst", "-sha256", "-sign", private_key, "-out", sig_path, manifest_path],
        dry_run=dry_run,
    )


def create_release(tag: str, dry_run: bool, target_repo: str = None) -> None:
    """Create a versioned GitHub Release and upload all assets."""
    print("\n--- Step 4: Create GitHub Release ---")
    channel = parse_channel(tag)
    release_cmd = ["gh", "release", "create", tag, "--title", tag, "--generate-notes"]
    if channel != "stable":
        release_cmd.append("--prerelease")
    if target_repo:
        release_cmd.extend(["--repo", target_repo])
    run(release_cmd, dry_run=dry_run, check=False)

    print("\n--- Step 5: Upload release assets ---")
    asset_patterns = ["*.bin", "*.bin.sig", "*.bin.sha256"]
    assets = []
    for pattern in asset_patterns:
        assets.extend(glob.glob(os.path.join(ARTIFACTS_DIR, pattern)))

    if not assets and not dry_run:
        print("ERROR: No assets found to upload", file=sys.stderr)
        sys.exit(1)

    upload_cmd = ["gh", "release", "upload", tag, "--clobber"]
    if target_repo:
        upload_cmd.extend(["--repo", target_repo])
    upload_cmd.extend(assets)
    run(upload_cmd, dry_run=dry_run)


def apply_risk_floor(manifest: dict, new_version_str: str) -> dict:
    """Apply risk-floor scanning across all channels in the manifest.

    For each channel, ensure its version is the best available for that
    stability floor. A stable release is valid for ALL channels. A dev
    release is only valid for the dev channel.

    Example: deploying v1.0.0 (stable) — if dev channel has v0.0.9-dev.5,
    update dev to v1.0.0 because stable >= dev floor and v1.0.0 > v0.0.9-dev.5.

    The new version's assets are copied to any channel that gets upgraded.

    Args:
        manifest: The full manifest dict (with all channel entries).
        new_version_str: The version being deployed (e.g. "v1.0.0").

    Returns:
        Updated manifest dict with risk-floor adjustments applied.
    """
    try:
        new_ver = FirmwareVersion(new_version_str)
    except ValueError:
        print(f"  WARNING: Cannot parse '{new_version_str}' for risk-floor scan — skipping",
              file=sys.stderr)
        return manifest

    new_stability = new_ver.channel  # 0=dev, 1=beta, 2=rc, 3=stable
    channels = manifest.get("channels", {})

    # Find the channel entry for the new version (to copy its assets)
    new_channel_name = parse_channel(new_version_str)
    new_entry = channels.get(new_channel_name, {})

    for ch_name, ch_entry in channels.items():
        ch_floor = channel_stability_floor(ch_name)

        # The new version is eligible for this channel if its stability >= the floor
        if new_stability < ch_floor:
            continue

        try:
            existing_ver = FirmwareVersion(ch_entry.get("version", "v0.0.0"))
        except ValueError:
            continue

        if new_ver > existing_ver:
            print(f"  Risk-floor: upgrading '{ch_name}' from {existing_ver.raw} → {new_ver.raw}")
            ch_entry["version"] = new_ver.raw
            # Copy assets from the new entry if available
            if "assets" in new_entry:
                ch_entry["assets"] = new_entry["assets"]
            if "securityVersion" in new_entry:
                ch_entry["securityVersion"] = new_entry["securityVersion"]

    manifest["channels"] = channels
    return manifest


def update_ota_manifest_release(tag: str, private_key: str, dry_run: bool,
                                 target_repo: str = None) -> None:
    """Download existing manifest, merge new channel, apply risk-floor, sign, upload.

    This creates/updates the fixed ``ota-manifest`` GitHub Release which serves
    as the canonical source for the v2 OTA manifest.

    Args:
        tag: The version tag being deployed (e.g. v3.1.0).
        private_key: Path to the ECDSA private key.
        dry_run: If True, print commands without executing.
        target_repo: Target repo slug (owner/repo), or None for current repo.
    """
    print("\n--- Step 6: Update OTA manifest release ---")

    repo_slug = get_repo_slug(target_repo)
    base_url = f"https://github.com/{repo_slug}/releases/download/{tag}"
    manifest_url = f"https://github.com/{repo_slug}/releases/download/{OTA_MANIFEST_TAG}/manifest.json"
    manifest_dir = os.path.join(ARTIFACTS_DIR, "ota-manifest-staging")
    os.makedirs(manifest_dir, exist_ok=True)
    existing_manifest_path = os.path.join(manifest_dir, "manifest.json")
    final_manifest_path = os.path.join(manifest_dir, "manifest.json")
    final_sig_path = os.path.join(manifest_dir, "manifest.json.sig")

    # 6a. Download existing manifest from ota-manifest release (if it exists)
    print("  Downloading existing manifest from ota-manifest release...")
    download_cmd = [
        "gh", "release", "download", OTA_MANIFEST_TAG,
        "--pattern", "manifest.json",
        "--dir", manifest_dir,
        "--clobber",
    ]
    if target_repo:
        download_cmd.extend(["--repo", target_repo])
    result = run(download_cmd, dry_run=dry_run, check=False,
                 capture_output=True if not dry_run else False)

    if dry_run:
        print("  [DRY-RUN] Would download existing manifest or start fresh")
    elif result.returncode != 0:
        print("  No existing ota-manifest release found — will create from scratch")
        # Write an empty manifest skeleton so generate_manifest.py --merge-into works
        with open(existing_manifest_path, "w") as f:
            json.dump({"manifestVersion": 1, "manifestUrl": manifest_url, "channels": {}}, f)

    # 6b. Call generate_manifest.py with --merge-into
    print("  Generating merged manifest...")
    channel = parse_channel(tag)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    manifest_script = os.path.join(script_dir, "generate_manifest.py")

    gen_cmd = [
        sys.executable, manifest_script,
        "--version", tag,
        "--channel", channel,
        "--security-version", "1",
        "--boards", ",".join(BOARDS),
        "--firmware-dir", ARTIFACTS_DIR,
        "--base-url", base_url,
        "--manifest-url", manifest_url,
        "--merge-into", existing_manifest_path,
        "--output", final_manifest_path,
    ]
    run(gen_cmd, dry_run=dry_run)

    # 6c. Risk-floor scanning
    print("  Applying risk-floor scanning...")
    if not dry_run:
        try:
            with open(final_manifest_path, "r") as f:
                manifest = json.load(f)
            manifest = apply_risk_floor(manifest, tag)
            with open(final_manifest_path, "w") as f:
                json.dump(manifest, f, indent=2)
                f.write("\n")
            print("  Risk-floor scan complete")
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"  WARNING: Risk-floor scan failed: {e}", file=sys.stderr)
    else:
        print("  [DRY-RUN] Would apply risk-floor scanning across all channels")

    # 6d. Sign the merged manifest
    print("  Signing merged manifest...")
    run(
        ["openssl", "dgst", "-sha256", "-sign", private_key, "-out", final_sig_path, final_manifest_path],
        dry_run=dry_run,
    )

    # 6e. Ensure ota-manifest release exists
    print("  Ensuring ota-manifest release exists...")
    create_cmd = [
        "gh", "release", "create", OTA_MANIFEST_TAG,
        "--title", "OTA Manifest",
        "--notes", "Fixed release holding the signed OTA manifest. Updated on every deploy.",
    ]
    if target_repo:
        create_cmd.extend(["--repo", target_repo])
    run(create_cmd, dry_run=dry_run, check=False)

    # 6f. Upload manifest + signature
    print("  Uploading manifest.json + manifest.json.sig to ota-manifest release...")
    upload_cmd = [
        "gh", "release", "upload", OTA_MANIFEST_TAG, "--clobber",
        final_manifest_path, final_sig_path,
    ]
    if target_repo:
        upload_cmd.extend(["--repo", target_repo])
    run(upload_cmd, dry_run=dry_run)


def update_floating_tags(tag: str, dry_run: bool, target_repo: str = None) -> None:
    """Create/update floating tag releases (LEGACY — deprecated).

    This replicates the pre-v2 behaviour of creating ``latest-{channel}``
    releases that point to the same assets. In the v2 architecture the
    ``ota-manifest`` release replaces this mechanism entirely.

    .. deprecated::
        Use ``--legacy-floating-tags`` only during the migration period.
        This flag and all floating-tag code will be removed in a future release.
    """
    warnings.warn(
        "Floating tags (latest-{channel}) are DEPRECATED. "
        "The v2 OTA manifest (ota-manifest release) replaces this mechanism. "
        "Remove --legacy-floating-tags once all devices are on v2.",
        DeprecationWarning,
        stacklevel=2,
    )

    channel = parse_channel(tag)
    floating_tag = f"latest-{channel}"

    print(f"\n--- Step 7 (LEGACY): Update floating tag ({floating_tag}) ---")

    # Ignore errors if the release/tag doesn't exist yet
    delete_cmd = ["gh", "release", "delete", floating_tag, "-y", "--cleanup-tag"]
    if target_repo:
        delete_cmd.extend(["--repo", target_repo])
    run(delete_cmd, dry_run=dry_run, check=False, stderr=subprocess.DEVNULL)

    # Set the title to the exact semantic version for human readability in the GitHub UI.
    # NOTE: This is NOT a critical contract. The ESP32 OTA engine resolves the version
    # from manifest.json (not the release title). The title is purely cosmetic.
    floating_release_cmd = ["gh", "release", "create", floating_tag, "--title", tag, "--notes", f"Alias for {tag}"]
    if channel != "stable":
        floating_release_cmd.append("--prerelease")
    if target_repo:
        floating_release_cmd.extend(["--repo", target_repo])
    run(floating_release_cmd, dry_run=dry_run)

    assets = []
    for pattern in ["*.bin", "*.bin.sig", "*.bin.sha256", "manifest.json", "manifest.json.sig"]:
        assets.extend(glob.glob(os.path.join(ARTIFACTS_DIR, pattern)))

    floating_upload_cmd = ["gh", "release", "upload", floating_tag, "--clobber"]
    if target_repo:
        floating_upload_cmd.extend(["--repo", target_repo])
    floating_upload_cmd.extend(assets)
    run(floating_upload_cmd, dry_run=dry_run)


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Sign firmware binaries, generate signed manifest, and create a GitHub Release",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--tag", help="Git tag for the release (e.g. v3.1.0). "
                                      "If omitted, auto-detected from the tag on HEAD.")
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
        "--sign-only",
        action="store_true",
        help="Stage, sign, hash, and generate manifest — but skip GitHub release creation",
    )
    parser.add_argument(
        "--legacy-floating-tags",
        action="store_true",
        help="(DEPRECATED) Also create latest-{channel} floating tag releases. "
             "The v2 ota-manifest release replaces this mechanism.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without executing them",
    )
    return parser.parse_args()


def resolve_tag(explicit_tag: str = None) -> str:
    """Return the release tag — from --tag or auto-detected from HEAD."""
    if explicit_tag:
        return explicit_tag
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--exact-match", "HEAD"],
            capture_output=True, text=True, check=True, timeout=5,
        )
        tag = result.stdout.strip()
        if tag:
            print(f"Auto-detected tag from HEAD: {tag}")
            return tag
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        pass
    print("ERROR: No --tag provided and HEAD has no exact tag. "
          "Tag your commit first (e.g. 'git tag v1.0.0') or pass --tag explicitly.",
          file=sys.stderr)
    sys.exit(1)


def main() -> None:
    """Entry point."""
    args = parse_args()

    if args.build and args.run_id:
        print("ERROR: --build is only valid with --local", file=sys.stderr)
        sys.exit(1)

    args.tag = resolve_tag(args.tag)

    mode = "local" if args.local else f"CI (run {args.run_id})"
    print(f"=== Deploy Release: {args.tag} ===")
    print(f"  Mode:        {mode}")
    print(f"  Private key: {args.private_key}")
    print(f"  Dry-run:     {args.dry_run}")
    if args.repo:
        print(f"  Target repo: {args.repo}")

    check_prerequisites(args.private_key, need_pio=args.build)
    ensure_tag_on_head(args.tag, args.dry_run)

    # Compute URLs for manifest generation
    repo_slug = get_repo_slug(args.repo)
    base_url = f"https://github.com/{repo_slug}/releases/download/{args.tag}"
    manifest_url = f"https://github.com/{repo_slug}/releases/download/{OTA_MANIFEST_TAG}/manifest.json"

    if args.local:
        if args.build:
            build_firmware(args.dry_run)
        collect_local_binaries(args.dry_run)
    else:
        download_artifacts(args.run_id, args.dry_run)

    sign_firmware(args.private_key, args.dry_run)
    generate_hashes(args.dry_run)
    generate_manifest(args.tag, base_url, manifest_url, args.dry_run)
    sign_manifest(args.private_key, os.path.join(ARTIFACTS_DIR, "manifest.json"), args.dry_run)

    if args.sign_only:
        print(f"\n✅ Signed binaries + manifest ready in {ARTIFACTS_DIR}/")
    else:
        create_release(args.tag, args.dry_run, target_repo=args.repo)
        update_ota_manifest_release(args.tag, args.private_key, args.dry_run,
                                     target_repo=args.repo)

        if args.legacy_floating_tags:
            update_floating_tags(args.tag, args.dry_run, target_repo=args.repo)

        print("\n✅ Release deployed successfully!")


if __name__ == "__main__":
    main()
