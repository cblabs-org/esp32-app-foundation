"""
ESP32 App Foundation (c) 2026 CB Labs

PlatformIO pre-build script: scripts/inject_version.py
Description: Single source of truth for build version flags.

    Golden source: git tags on HEAD — all tags pointing at HEAD are read,
                   each is parsed as SemVer, and the highest version is
                   selected. This is robust against multiple tags sharing
                   the same commit (e.g. v0.0.4 + v0.0.5 both on HEAD).
    Fallback:      0.0.1-stable.0 (untagged development builds)

    Injects the following build flags:
        -DVERSION_MAJOR=N
        -DVERSION_MINOR=N
        -DVERSION_PATCH=N
        -DVERSION_CHANNEL=N   (0=dev, 2=beta, 3=rc, 15=stable)
        -DVERSION_PRE_NUM=N
        -DBUILD_VERSION="N.N.N"

    No version flags should appear in platformio.ini — this script
    is the sole owner.
"""

Import("env")

import subprocess
import re


# SemVer tag pattern: v3.1.0 or v3.1.0-beta.1
_SEMVER_RE = re.compile(
    r"^[vV]?(\d+)\.(\d+)\.(\d+)(?:-(dev|beta|rc)(?:\.(\d+))?)?$"
)
_CHANNEL_MAP = {"dev": 0, "beta": 2, "rc": 3}


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
    channel  = _CHANNEL_MAP.get(chan_str, 15)   # 15 = STABLE
    pre_num  = int(m.group(5)) if m.group(5) else 0
    sort_key = (major, minor, patch, channel, pre_num)
    return sort_key, {
        "major": major, "minor": minor, "patch": patch,
        "channel": channel, "channel_str": chan_str, "pre_num": pre_num,
    }


def _get_tags_on_head():
    """Return a list of tag names for the current HEAD, or [] when none exist.

    Two-strategy lookup — both are attempted before giving up:

    Strategy 1 — exact match (release builds):
        ``git tag --points-at HEAD`` returns every tag whose object is exactly
        the current HEAD commit.  This is the happy path for a proper release
        build where the tag was created after all commits were finalised.

    Strategy 2 — ancestor search (development / PA-commit builds):
        ``git describe --tags --abbrev=0 --match 'v*'`` walks the commit graph
        backwards to find the nearest reachable tag.  This handles the common
        case where one or more housekeeping commits (PA workspace updates,
        distillery artefacts, doc fixes) were pushed *after* the release tag
        was created, moving HEAD past the tag and causing Strategy 1 to return
        nothing.  A WARNING is emitted so the agent knows the tag is not on
        HEAD and can retag before the next release.

    Returns [] only when the repository has no tags at all in its history.
    """
    try:
        # --- Strategy 1: exact tag on HEAD (fastest, most precise) ---
        result = subprocess.run(
            ["git", "tag", "--points-at", "HEAD"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        if result.returncode == 0:
            tags = [t.strip() for t in result.stdout.splitlines() if t.strip()]
            if tags:
                return tags

        # --- Strategy 2: nearest ancestor tag via git describe ---
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0", "--match", "v*"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        if result.returncode == 0:
            tag = result.stdout.strip()
            if tag:
                print(
                    f"[inject_version] WARNING: HEAD has no exact tag — "
                    f"using nearest ancestor: '{tag}'. "
                    f"Run 'git tag {tag} -f && git push origin --force {tag}' "
                    f"to retag HEAD before the next release."
                )
                return [tag]

    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass

    return []


def inject_version():
    """Read all git tags on HEAD, select the highest SemVer, inject build flags."""

    # Defaults for untagged / no-git builds
    components = {
        "major": 0, "minor": 0, "patch": 1,
        "channel": 15, "channel_str": None, "pre_num": 0,
    }

    tags = _get_tags_on_head()

    if not tags:
        # Emit on both stdout (captured by PlatformIO build log) and stderr
        # (visible in agent terminal / CI output) so neither path is silent.
        _no_tag_warning = (
            "[inject_version] \u26a0\ufe0f  NO GIT TAGS FOUND in repository history. "
            "Version will default to 0.0.1-stable.0. "
            "This build MUST NOT be released — tag HEAD with a SemVer tag "
            "(e.g. 'git tag v0.1.0') before building for release."
        )
        print(_no_tag_warning)
        import sys as _sys
        print(_no_tag_warning, file=_sys.stderr)
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

    version_str = f"{major}.{minor}.{patch}"
    # Full version includes channel suffix for non-stable builds.
    # e.g. "0.0.6-beta.1", "0.0.6-rc.2", "0.0.6" (stable omits suffix).
    if chan_str:
        full_version_str = f"{version_str}-{chan_str}.{pre_num}"
    else:
        full_version_str = version_str
    print(
        f"[inject_version] Building version: {full_version_str} (channel={channel})"
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
                "-DAPP_FOUNDATION_VERSION=",  # Legacy — remove if present
            ]
        )
    ]

    filtered.extend([
        f"-DVERSION_MAJOR={major}",
        f"-DVERSION_MINOR={minor}",
        f"-DVERSION_PATCH={patch}",
        f"-DVERSION_CHANNEL={channel}",
        f"-DVERSION_PRE_NUM={pre_num}",
        f'-DBUILD_VERSION=\\"{full_version_str}\\"',
    ])

    env.Replace(BUILD_FLAGS=filtered)


inject_version()
