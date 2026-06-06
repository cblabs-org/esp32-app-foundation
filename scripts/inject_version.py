"""
ESP32 App Foundation (c) 2026 CB Labs

PlatformIO pre-build script: scripts/inject_version.py
Description: Single source of truth for build version and board-count flags.

    Golden source: git tags on HEAD — all tags pointing at HEAD are read,
                   each is parsed as SemVer, and the highest version is
                   selected. This is robust against multiple tags sharing
                   the same commit (e.g. v0.0.4 + v0.0.5 both on HEAD).
    Fallback:      0.0.1-stable.0 (untagged development builds)

    Injects the following build flags:
        -DVERSION_MAJOR=N
        -DVERSION_MINOR=N
        -DVERSION_PATCH=N
        -DVERSION_CHANNEL=N   (0=dev, 1=beta, 2=rc, 3=stable)
        -DVERSION_PRE_NUM=N
        -DBUILD_VERSION="N.N.N[-channel.pre]"
        -DMAX_OTA_BOARDS=N    (count of hardware envs in platformio.ini default_envs)

    No version or board-count flags should appear in platformio.ini —
    this script is the sole owner.
"""

Import("env")

import configparser
import subprocess
import re


def _count_ota_boards():
    """Return the number of hardware build environments from platformio.ini.

    Reads [platformio] default_envs, which is the canonical single declaration
    of which boards are built and released. This count drives MAX_OTA_BOARDS
    (and therefore MAX_RELEASE_ASSETS in GithubClient.hpp) so that adding a
    board to platformio.ini is the only change required.

    Returns a safe fallback of 8 if the file cannot be read.
    """
    cfg = configparser.ConfigParser()
    try:
        cfg.read("platformio.ini")
        raw = cfg.get("platformio", "default_envs", fallback="")
        boards = [e.strip() for e in raw.replace("\n", ",").split(",") if e.strip()]
        count = len(boards)
        if count == 0:
            print("[inject_version] WARNING: default_envs is empty — "
                  "defaulting MAX_OTA_BOARDS=8")
            return 8
        print(f"[inject_version] Board count from default_envs: {count} "
              f"({', '.join(boards)})")
        return count
    except Exception as exc:
        print(f"[inject_version] WARNING: could not read platformio.ini "
              f"({exc}) — defaulting MAX_OTA_BOARDS=8")
        return 8


# SemVer tag pattern: v3.1.0 or v3.1.0-beta.1
_SEMVER_RE = re.compile(
    r"^[vV]?(\d+)\.(\d+)\.(\d+)(?:-(dev|beta|rc)(?:\.(\d+))?)?$"
)
# Channel numeric values must match the Channel enum in lib/SemVer/SemVer.hpp.
# Ordering: DEV(0) < BETA(1) < RC(2) < STABLE(3) — contiguous 2-bit values.
_CHANNEL_MAP = {"dev": 0, "beta": 1, "rc": 2}


def _parse_tag(tag):
    """Parse a tag string into a sort key and component dict.

    Returns (sort_key_tuple, components_dict) or None if the tag does not
    match the expected SemVer pattern.

    sort_key: (major, minor, patch, channel_numeric, pre_num)
      Channel numeric for ordering: stable(15) > rc(3) > beta(2) > dev(0).
    """
    m = _SEMVER_RE.match(tag.strip())
    if not m:
        return None
    major    = int(m.group(1))
    minor    = int(m.group(2))
    patch    = int(m.group(3))
    chan_str = m.group(4)           # None for stable releases
    channel  = _CHANNEL_MAP.get(chan_str, 3)   # 3 = STABLE (matches Channel::STABLE in SemVer.hpp)
    pre_num  = int(m.group(5)) if m.group(5) else 0
    sort_key = (major, minor, patch, channel, pre_num)
    return sort_key, {
        "major": major, "minor": minor, "patch": patch,
        "channel": channel, "channel_str": chan_str, "pre_num": pre_num,
    }


def _get_tags_on_head():
    """Return a list of all tag names pointing at HEAD, or [] on error."""
    try:
        result = subprocess.run(
            ["git", "tag", "--points-at", "HEAD"],
            capture_output=True,
            text=True,
            timeout=5,
        )
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []
    if result.returncode != 0:
        return []
    return [t.strip() for t in result.stdout.splitlines() if t.strip()]


def inject_version():
    """Read all git tags on HEAD, select the highest SemVer, inject build flags."""

    # Defaults for untagged / no-git builds
    components = {
        "major": 0, "minor": 0, "patch": 1,
        "channel": 15, "channel_str": None, "pre_num": 0,
    }

    tags = _get_tags_on_head()

    if not tags:
        print("[inject_version] HEAD is not tagged — using defaults (0.0.1)")
    else:
        # Parse all tags, collect those matching SemVer
        parsed = []
        for tag in tags:
            result = _parse_tag(tag)
            if result:
                parsed.append((result[0], tag, result[1]))
            else:
                print(f"[inject_version] Skipping non-SemVer tag: '{tag}'")

        if not parsed:
            print(
                f"[inject_version] {len(tags)} tag(s) on HEAD but none match "
                f"SemVer — using defaults (0.0.1)"
            )
        else:
            # Sort descending by sort_key; pick the highest version
            parsed.sort(key=lambda x: x[0], reverse=True)
            _best_key, best_tag, components = parsed[0]
            if len(parsed) > 1:
                others = ", ".join(t for _, t, _ in parsed[1:])
                print(
                    f"[inject_version] {len(parsed)} SemVer tags on HEAD — "
                    f"selected highest: '{best_tag}' (ignored: {others})"
                )
            else:
                print(f"[inject_version] Detected git tag: {best_tag}")

    major    = components["major"]
    minor    = components["minor"]
    patch    = components["patch"]
    channel  = components["channel"]
    chan_str = components["channel_str"]
    pre_num  = components["pre_num"]

    version_str      = f"{major}.{minor}.{patch}"
    full_version_str = f"{version_str}-{chan_str}.{pre_num}" if chan_str else version_str
    print(
        f"[inject_version] Building version: {full_version_str} "
        f"(channel={channel})"
    )

    # Strip any pre-existing version flags, then inject ours
    flags = env.get("BUILD_FLAGS", [])
    filtered = [
        f for f in flags
        if not any(
            f.startswith(prefix)
            for prefix in [
                "-DVERSION_MAJOR=",
                "-DVERSION_MINOR=",
                "-DVERSION_PATCH=",
                "-DVERSION_CHANNEL=",
                "-DVERSION_PRE_NUM=",
                "-DBUILD_VERSION=",
                "-DMAX_OTA_BOARDS=",
                "-DAPP_FOUNDATION_VERSION=",  # Legacy — remove if present
            ]
        )
    ]

    board_count = _count_ota_boards()
    filtered.extend([
        f"-DVERSION_MAJOR={major}",
        f"-DVERSION_MINOR={minor}",
        f"-DVERSION_PATCH={patch}",
        f"-DVERSION_CHANNEL={channel}",
        f"-DVERSION_PRE_NUM={pre_num}",
        f'-DBUILD_VERSION=\\"{full_version_str}\\"',
        f"-DMAX_OTA_BOARDS={board_count}",
    ])

    env.Replace(BUILD_FLAGS=filtered)


inject_version()
