"""
ESP32 App Foundation (c) 2026 CB Labs

PlatformIO pre-build script: scripts/inject_version.py
Description: Reads the current git tag (if HEAD is an exact tag match) and
             overrides VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
             VERSION_CHANNEL, and VERSION_PRE_NUM build flags accordingly.
             If no tag is found, the static defaults from platformio.ini apply.
"""

Import("env")

import subprocess
import re


def inject_version():
    """Check for a git tag on HEAD and override build flags if found."""
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--exact-match"],
            capture_output=True,
            text=True,
            timeout=5,
        )
    except (subprocess.TimeoutExpired, FileNotFoundError):
        print("[inject_version] git not available or timed out — using defaults")
        return

    if result.returncode != 0:
        print("[inject_version] HEAD is not a tagged commit — using static defaults")
        return

    tag = result.stdout.strip()
    print(f"[inject_version] Detected git tag: {tag}")

    # Parse tag: v3.1.0 or v3.1.0-beta.1
    pattern = r"^[vV]?(\d+)\.(\d+)\.(\d+)(?:-(dev|beta|rc)(?:\.(\d+))?)?$"
    m = re.match(pattern, tag)
    if not m:
        print(f"[inject_version] Tag '{tag}' does not match SemVer pattern — using defaults")
        return

    major = int(m.group(1))
    minor = int(m.group(2))
    patch = int(m.group(3))

    channel_map = {"dev": 0, "beta": 2, "rc": 3}
    channel_str = m.group(4)
    channel = channel_map.get(channel_str, 15)  # 15 = STABLE
    pre_num = int(m.group(5)) if m.group(5) else 0

    print(
        f"[inject_version] Building version: "
        f"{major}.{minor}.{patch}-{channel_str or 'stable'}.{pre_num} "
        f"(channel={channel})"
    )

    # Remove existing VERSION_* flags and inject overrides
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
            ]
        )
    ]

    filtered.extend([
        f"-DVERSION_MAJOR={major}",
        f"-DVERSION_MINOR={minor}",
        f"-DVERSION_PATCH={patch}",
        f"-DVERSION_CHANNEL={channel}",
        f"-DVERSION_PRE_NUM={pre_num}",
    ])

    env.Replace(BUILD_FLAGS=filtered)


inject_version()
