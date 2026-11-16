#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

Script: scripts/deploy_release.py
Description: Signs firmware binaries, creates a versioned GitHub Release with
             all release assets, then updates the git-managed OTA manifest
             (manifest.json committed to the target repo). Supports both
             single-repo and split-repo models via --repo and --releases-dir.

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
from urllib.parse import urlparse


# Board list matching the CI matrix
BOARDS = ["esp32dev", "esp32s3nano"]

# Release staging directory
ARTIFACTS_DIR = "./release"



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


_GITHUB_SLUG_PART_RE = re.compile(r"^[A-Za-z0-9._-]+$")
_GITHUB_SCP_RE = re.compile(r"^[^/@\s]+@github\.com:([^/\s].*)$", re.IGNORECASE)
_GITHUB_RELEASE_URL_PREFIX_RE = re.compile(
    r"^https://github\.com/[^/]+/[^/]+/releases/download/"
)


def parse_github_slug(remote_url: str) -> str | None:
    """Parse an ``owner/repo`` slug from a GitHub HTTPS or SSH remote URL.

    Supports ``https://`` (and ``http://``), SCP-style ``git@github.com:owner/repo``,
    and ``ssh://git@github.com/owner/repo`` URLs, with optional ``.git`` suffix
    and optional trailing slash. Returns ``None`` for empty strings, non-GitHub
    hosts, local paths, or malformed repository paths.
    """
    if not isinstance(remote_url, str):
        return None
    url = remote_url.strip()
    if not url:
        return None

    path = None
    if "://" in url:
        parsed = urlparse(url)
        if parsed.scheme.lower() not in ("https", "http", "ssh"):
            return None
        if (parsed.hostname or "").lower() != "github.com":
            return None
        if parsed.query or parsed.fragment:
            return None
        path = parsed.path.lstrip("/")
    else:
        scp_match = _GITHUB_SCP_RE.match(url)
        if not scp_match:
            return None
        path = scp_match.group(1)

    path = path.rstrip("/")
    parts = path.split("/")
    if len(parts) != 2:
        return None

    owner, repo = parts
    if repo.endswith(".git"):
        repo = repo[:-4]

    if not owner or not repo or owner in (".", "..") or repo in (".", ".."):
        return None
    if not (_GITHUB_SLUG_PART_RE.fullmatch(owner) and _GITHUB_SLUG_PART_RE.fullmatch(repo)):
        return None

    return f"{owner}/{repo}"


def _infer_slug_from_git(cwd: str | None) -> str | None:
    """Infer a GitHub ``owner/repo`` slug from git origin or ``gh repo view``."""
    git_cmd = (
        ["git", "-C", cwd, "remote", "get-url", "origin"]
        if cwd
        else ["git", "remote", "get-url", "origin"]
    )
    try:
        git_res = subprocess.run(
            git_cmd, capture_output=True, text=True, check=True, timeout=10,
        )
        slug = parse_github_slug(git_res.stdout.strip())
        if slug:
            return slug
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        pass

    try:
        gh_res = subprocess.run(
            ["gh", "repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner"],
            cwd=cwd, capture_output=True, text=True, check=True, timeout=10,
        )
        slug = gh_res.stdout.strip()
        if slug:
            return slug
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return None


def get_repo_slug(target_repo: str = None, releases_dir: str = None) -> str:
    """Return the repo slug (owner/repo) from --repo, releases_dir, or CWD git remote."""
    if target_repo and target_repo.strip():
        return target_repo.strip()

    if releases_dir and os.path.exists(os.path.join(releases_dir, ".git")):
        slug = _infer_slug_from_git(releases_dir)
    else:
        slug = _infer_slug_from_git(None)

    if slug:
        return slug

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
    """Build firmware for all boards with PlatformIO.

    Runs a clean build (``pio run -t clean`` then ``pio run``) to prevent
    ESP-IDF's CMake cache from retaining a stale PROJECT_VER in
    ``esp_app_desc_t.version`` across incremental rebuilds.
    """
    print("\n--- Step 0: Build firmware (local, clean) ---")
    for board in BOARDS:
        run(["pio", "run", "-e", board, "-t", "clean"], dry_run=dry_run)
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


def preflight_gates(tag: str, dry_run: bool) -> None:
    """Run pre-flight safety gates before build/deploy.

    Gate 1 — Clean working tree: Abort if the working tree is dirty.
             ESP-IDF appends '-dirty' to esp_app_desc_t.version via
             ``git describe --dirty``, which will never match the manifest.

    Gate 2 — Tag describes HEAD: Verify ``git describe --tags --exact-match HEAD``
             matches the --tag argument. Multiple tags on HEAD can cause
             git describe to pick a different one.

    In dry-run mode gates are advisory (the tag may not exist yet).
    """
    print("\n--- Pre-flight safety gates ---")

    # Gate 1: Clean working tree
    print("  Gate 1: Checking for clean working tree...")
    result = subprocess.run(
        ["git", "status", "--porcelain"],
        capture_output=True, text=True, timeout=10,
    )
    if result.stdout.strip():
        if dry_run:
            print("  ⚠ WARNING: Working tree is dirty (would abort in live mode)")
        else:
            print("ERROR: Working tree is dirty. Commit or stash changes before deploying.",
                  file=sys.stderr)
            print(f"  Dirty files:\n{result.stdout}", file=sys.stderr)
            sys.exit(1)
    else:
        print("  ✓ Working tree is clean")

    # Gate 2: Tag describes HEAD
    print(f"  Gate 2: Verifying git describe matches '{tag}'...")
    result = subprocess.run(
        ["git", "describe", "--tags", "--exact-match", "HEAD"],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        if dry_run:
            print(f"  ⚠ WARNING: HEAD has no exact tag (would abort in live mode)")
        else:
            print(f"ERROR: HEAD has no exact tag. Expected '{tag}' to be on HEAD.\n"
                  f"       Create the tag first: git tag {tag}",
                  file=sys.stderr)
            sys.exit(1)
    else:
        described_tag = result.stdout.strip()
        if described_tag != tag:
            if dry_run:
                print(f"  ⚠ WARNING: git describe returns '{described_tag}', "
                      f"expected '{tag}' (would abort in live mode)")
            else:
                print(f"ERROR: git describe returns '{described_tag}', expected '{tag}'.\n"
                      f"       Multiple tags on HEAD? Remove the stale one or use --tag.",
                      file=sys.stderr)
                sys.exit(1)
        else:
            print(f"  ✓ git describe confirms '{tag}' on HEAD")


def resolve_releases_dir(explicit_dir: str = None) -> str:
    """Resolve the local git checkout directory for the manifest.

    Resolution order:
        1. Explicit --releases-dir value
        2. ./releases/ subdirectory (if it exists and contains .git)
        3. Current directory '.' (single-repo model)

    Returns:
        Absolute path to the releases directory.
    """
    if explicit_dir:
        path = os.path.abspath(explicit_dir)
        if not os.path.isdir(path):
            print(f"ERROR: --releases-dir does not exist: {path}", file=sys.stderr)
            sys.exit(1)
        print(f"  Releases dir (explicit): {path}")
        return path

    releases_subdir = os.path.abspath("./releases")
    if os.path.isdir(releases_subdir) and os.path.isdir(os.path.join(releases_subdir, ".git")):
        print(f"  Releases dir (auto-detected): {releases_subdir}")
        return releases_subdir

    cwd = os.path.abspath(".")
    print(f"  Releases dir (current directory): {cwd}")
    return cwd


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


def apply_risk_floor(manifest: dict, new_version_str: str, repo_slug: str = None) -> dict:
    """Apply risk-floor pruning, promotion, and asset URL normalization.

    Pass 1 (Prune): Remove channel entries whose version is invalid or below
    the channel's stability floor (e.g. a dev tag sitting in ``stable``).

    Pass 2 (Promote): Upgrade surviving channels whose floor is met by
    ``new_version_str`` whenever ``new_ver > existing_ver``, cloning asset
    dicts so channels remain independent.

    Pass 3 (Normalize URLs): When ``repo_slug`` is known, rewrite any GitHub
    release download URL prefix on surviving assets to ``repo_slug``.

    Args:
        manifest: The full manifest dict (with all channel entries).
        new_version_str: The version being deployed (e.g. "v1.0.0").
        repo_slug: Target repo slug (owner/repo) for normalizing asset URLs.

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
    if not isinstance(channels, dict):
        channels = {}

    # Pass 1: Prune invalid or sub-floor channel entries
    for ch_name in list(channels.keys()):
        ch_entry = channels[ch_name]
        ch_floor = channel_stability_floor(ch_name)
        if not isinstance(ch_entry, dict):
            print(f"  Pruning invalid channel '{ch_name}': entry is not an object")
            del channels[ch_name]
            continue

        raw_ver = ch_entry.get("version", "")
        try:
            existing_ver = FirmwareVersion(raw_ver if isinstance(raw_ver, str) else "")
        except ValueError:
            print(f"  Pruning invalid channel '{ch_name}': invalid version {raw_ver!r}")
            del channels[ch_name]
            continue

        if existing_ver.channel < ch_floor:
            print(
                f"  Pruning invalid channel '{ch_name}': version '{raw_ver}' "
                f"does not meet '{ch_name}' stability floor"
            )
            del channels[ch_name]

    # Pass 2: Risk-floor promotion across surviving channels
    new_channel_name = parse_channel(new_version_str)
    new_entry = channels.get(new_channel_name, {})
    if not isinstance(new_entry, dict):
        new_entry = {}

    for ch_name, ch_entry in channels.items():
        ch_floor = channel_stability_floor(ch_name)
        if new_stability < ch_floor:
            continue

        existing_ver = FirmwareVersion(ch_entry["version"])
        if new_ver > existing_ver:
            print(f"  Risk-floor: upgrading '{ch_name}' from {existing_ver.raw} → {new_ver.raw}")
            ch_entry["version"] = new_ver.raw
            if "assets" in new_entry:
                ch_entry["assets"] = [
                    dict(asset) if isinstance(asset, dict) else asset
                    for asset in new_entry["assets"]
                ]
            if "securityVersion" in new_entry:
                ch_entry["securityVersion"] = new_entry["securityVersion"]

    # Pass 3: Normalize GitHub release download URLs to target repo_slug
    if repo_slug and repo_slug != "UNKNOWN/REPO":
        target_prefix = f"https://github.com/{repo_slug}/releases/download/"
        for ch_entry in channels.values():
            assets = ch_entry.get("assets")
            if not isinstance(assets, list):
                continue
            for asset in assets:
                if not isinstance(asset, dict):
                    continue
                for url_key in ("firmwareUrl", "signatureUrl"):
                    url_val = asset.get(url_key)
                    if isinstance(url_val, str):
                        asset[url_key] = _GITHUB_RELEASE_URL_PREFIX_RE.sub(
                            target_prefix, url_val, count=1
                        )

    manifest["channels"] = channels
    return manifest


def update_manifest(tag: str, repo_slug: str, private_key: str,
                    releases_dir: str, dry_run: bool) -> None:
    """Update the git-managed OTA manifest and push to origin.

    Flow:
        1. Pull latest from origin/main
        2. Read existing manifest.json (or start fresh)
        3. Merge new channel entry (version + constructed URLs)
        4. Apply risk-floor scanning
        5. Set manifestUrl (raw.githubusercontent.com)
        6. Write manifest.json
        7. Sign manifest.json
        8. Commit and push

    Args:
        tag: Version tag being deployed (e.g. v3.1.0).
        repo_slug: Target repo slug (owner/repo) for URL construction.
        private_key: Path to the RSA-2048 private key.
        releases_dir: Local git checkout of the releases repo.
        dry_run: If True, print commands without executing.
    """
    print("\n--- Step 6: Update git-managed manifest ---")

    channel = parse_channel(tag)
    base_url = f"https://github.com/{repo_slug}/releases/download/{tag}"
    manifest_url = f"https://raw.githubusercontent.com/{repo_slug}/main/manifest.json"
    manifest_path = os.path.join(releases_dir, "manifest.json")
    sig_path = os.path.join(releases_dir, "manifest.json.sig")

    # 6a. Pull latest
    print("  Pulling latest from origin/main...")
    run(["git", "-C", releases_dir, "pull", "--ff-only", "origin", "main"],
        dry_run=dry_run, check=False)

    # 6b. Read existing manifest
    print("  Reading existing manifest...")
    if os.path.isfile(manifest_path):
        try:
            with open(manifest_path, "r") as f:
                manifest = json.load(f)
            print(f"  Loaded existing manifest ({len(manifest.get('channels', {}))} channels)")
        except (json.JSONDecodeError, IOError) as e:
            print(f"  WARNING: Could not read manifest: {e} — starting fresh",
                  file=sys.stderr)
            manifest = {"manifestVersion": 1, "channels": {}}
    else:
        print("  No existing manifest — creating from scratch")
        manifest = {"manifestVersion": 1, "channels": {}}

    # 6c. Merge new channel entry
    print(f"  Merging channel '{channel}' with version '{tag}'...")
    manifest["channels"][channel] = {
        "version": tag,
        "securityVersion": 1,
        "assets": [
            {
                "board": board,
                "firmwareUrl": f"{base_url}/firmware-{board}.bin",
                "signatureUrl": f"{base_url}/firmware-{board}.bin.sig",
            }
            for board in BOARDS
        ],
    }

    # 6d. Apply risk-floor scanning
    print("  Applying risk-floor scanning...")
    manifest = apply_risk_floor(manifest, tag, repo_slug=repo_slug)
    print("  Risk-floor scan complete")

    # 6e. Set manifestUrl
    manifest["manifestUrl"] = manifest_url

    # 6f. Write manifest
    print(f"  Writing manifest to {manifest_path}...")
    if not dry_run:
        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=2)
            f.write("\n")
    else:
        print(f"  [DRY-RUN] Would write manifest to {manifest_path}")
        print(f"  Manifest preview:\n{json.dumps(manifest, indent=2)}")

    # 6g. Sign manifest
    print("  Signing manifest...")
    run(
        ["openssl", "dgst", "-sha256", "-sign", private_key,
         "-out", sig_path, manifest_path],
        dry_run=dry_run,
    )

    # 6h. Commit and push
    print("  Committing and pushing manifest...")
    run(["git", "-C", releases_dir, "add", "manifest.json", "manifest.json.sig"],
        dry_run=dry_run)
    run(["git", "-C", releases_dir, "commit", "-m",
         f"chore: update manifest for {tag}"],
        dry_run=dry_run)
    run(["git", "-C", releases_dir, "push", "origin", "main"],
        dry_run=dry_run)

    # 6i. Summary
    print(f"\n  ✅ Manifest updated and pushed")
    print(f"  Manifest URL: {manifest_url}")
    if not dry_run:
        result = subprocess.run(
            ["git", "-C", releases_dir, "rev-parse", "HEAD"],
            capture_output=True, text=True,
        )
        if result.returncode == 0:
            print(f"  Commit: {result.stdout.strip()[:12]}")





def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Sign firmware binaries, generate signed manifest, and create a GitHub Release",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--tag", help="Git tag for the release (e.g. v3.1.0). "
                                      "If omitted, auto-detected from the tag on HEAD.")
    parser.add_argument("--private-key", required=True, help="Path to RSA-2048 private key")

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
        help="Target GitHub repo for releases AND the manifest (owner/repo "
             "format, e.g. 'cblabs-org/esp32-firmware-releases'). "
             "In single-repo mode (default), this is the current repo's origin. "
             "In split-repo mode, pass the public releases repo.",
    )
    parser.add_argument(
        "--releases-dir",
        help="Local path to the git checkout of the target repo (where "
             "manifest.json is committed). Defaults: (1) explicit value, "
             "(2) ./releases/ if it exists with .git, (3) current directory.",
    )
    parser.add_argument(
        "--sign-only",
        action="store_true",
        help="Stage, sign, and hash firmware — but skip GitHub release "
             "creation and manifest update",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without executing them",
    )
    return parser.parse_args()


def resolve_tag(explicit_tag: str = None) -> str:
    """Return the release tag — from --tag or auto-detected from HEAD.

    When multiple tags point at HEAD, the highest SemVer tag wins.
    """
    if explicit_tag:
        return explicit_tag
    try:
        result = subprocess.run(
            ["git", "tag", "--points-at", "HEAD"],
            capture_output=True, text=True, check=True, timeout=5,
        )
        tags = [t.strip() for t in result.stdout.strip().splitlines() if t.strip()]
        # Filter to SemVer tags only
        semver_tags = []
        for t in tags:
            try:
                semver_tags.append((FirmwareVersion(t), t))
            except ValueError:
                continue
        if semver_tags:
            semver_tags.sort(key=lambda x: x[0], reverse=True)
            tag = semver_tags[0][1]
            if len(semver_tags) > 1:
                print(f"Multiple tags on HEAD: {[t for _, t in semver_tags]} — using highest: {tag}")
            else:
                print(f"Auto-detected tag from HEAD: {tag}")
            return tag
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        pass
    print("ERROR: No --tag provided and HEAD has no exact SemVer tag. "
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
    releases_dir = None if args.sign_only else resolve_releases_dir(args.releases_dir)
    repo_slug = get_repo_slug(args.repo, releases_dir=releases_dir)
    print(f"=== Deploy Release: {args.tag} ===")
    print(f"  Mode:        {mode}")
    print(f"  Private key: {args.private_key}")
    print(f"  Target repo: {repo_slug}")
    print(f"  Dry-run:     {args.dry_run}")

    # Pre-flight safety gates (before build/deploy)
    preflight_gates(args.tag, args.dry_run)

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

    if args.sign_only:
        print(f"\n✅ Signed binaries + hashes ready in {ARTIFACTS_DIR}/")
    else:
        target_repo = repo_slug if repo_slug != "UNKNOWN/REPO" else args.repo
        create_release(args.tag, args.dry_run, target_repo=target_repo)

        # Update git-managed manifest
        update_manifest(
            tag=args.tag,
            repo_slug=repo_slug,
            private_key=args.private_key,
            releases_dir=releases_dir,
            dry_run=args.dry_run,
        )

        print("\n✅ Release deployed successfully!")


if __name__ == "__main__":
    main()
