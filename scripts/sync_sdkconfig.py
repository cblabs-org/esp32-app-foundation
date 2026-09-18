"""
ESP32 App Foundation (c) 2026 CB Labs

PlatformIO pre-build script: scripts/sync_sdkconfig.py
Description: Invalidates stale generated sdkconfig.<env> files when any
             sdkconfig.defaults.* source file is newer.

    Root cause addressed: ESP-IDF's kconfig.cmake does NOT watch sdkconfig.defaults.*
    as CMake dependencies. Once sdkconfig.<env> is generated, .defaults files are
    ignored until the generated file is deleted. This script adds the missing
    invalidation layer, ensuring changes to .defaults files always propagate.

    Detection strategy:
        1. Parse SDKCONFIG_DEFAULTS from board_build.cmake_extra_args (S3 Nano)
           OR custom_sdkconfig (esp32dev) to discover which .defaults files to watch.
        2. Compare mtime of each source file against the generated sdkconfig.<env>.
        3. If any source is newer → delete the generated file.
        4. CMake will regenerate it from the full SDKCONFIG_DEFAULTS list on next run.

    Supported environments: esp32dev, esp32s3nano, esp32s3nano_ble
    Skips gracefully: native env, envs without sdkconfig configuration.
"""

Import("env")

import os
import re


def _get_defaults_files(env):
    """Return list of .defaults file paths to watch for this environment.

    Checks board_build.cmake_extra_args first (new layered style, e.g. esp32s3nano),
    then falls back to custom_sdkconfig (legacy single-file style, e.g. esp32dev).
    Returns empty list if neither is configured (e.g. native env).

    Note: board_build.* options in platformio.ini are stored in PlatformIO's
    BoardConfig under the "build.*" namespace and must be read via
    env.BoardConfig().get("build.<option>") — NOT via GetProjectOption.
    This mirrors how espidf.py (line 2573) reads cmake_extra_args internally.
    """
    project_dir = env.subst("$PROJECT_DIR")

    # Strategy 1: board_build.cmake_extra_args with SDKCONFIG_DEFAULTS="a;b"
    # Accessed via BoardConfig because board_build.* maps to build.* in the board JSON.
    try:
        board = env.BoardConfig()
        cmake_args = board.get("build.cmake_extra_args", "")
        match = re.search(r'-DSDKCONFIG_DEFAULTS=["\'"]?([^"\'\s]+)["\'"]?', cmake_args)
        if match:
            raw = match.group(1)
            return [os.path.join(project_dir, f.strip()) for f in raw.split(";") if f.strip()]
    except Exception:
        pass

    # Strategy 2: custom_sdkconfig (single file, legacy)
    # This is a custom project option so GetProjectOption works correctly here.
    try:
        custom = env.GetProjectOption("custom_sdkconfig", "")
        if custom.strip():
            return [os.path.join(project_dir, custom.strip())]
    except Exception:
        pass

    return []


def sync_sdkconfig():
    """Delete stale generated sdkconfig.<env> if any .defaults source is newer."""
    pioenv = env.subst("$PIOENV")
    project_dir = env.subst("$PROJECT_DIR")
    generated = os.path.join(project_dir, f"sdkconfig.{pioenv}")

    defaults_files = _get_defaults_files(env)

    if not defaults_files:
        # No sdkconfig configuration for this environment — skip silently
        return

    if not os.path.exists(generated):
        print(f"[sync_sdkconfig] {pioenv}: no generated config found — "
              f"will be created from defaults on first build.")
        return

    generated_mtime = os.path.getmtime(generated)

    for defaults_path in defaults_files:
        if not os.path.exists(defaults_path):
            print(f"[sync_sdkconfig] WARNING: {pioenv}: defaults file not found: "
                  f"{os.path.basename(defaults_path)}")
            continue

        defaults_mtime = os.path.getmtime(defaults_path)
        if defaults_mtime > generated_mtime:
            print(f"[sync_sdkconfig] {pioenv}: {os.path.basename(defaults_path)} "
                  f"is newer than sdkconfig.{pioenv} — deleting stale generated config.")
            print(f"[sync_sdkconfig] {pioenv}: ESP-IDF will regenerate from "
                  f"SDKCONFIG_DEFAULTS on next CMake configure.")
            os.remove(generated)
            return  # No need to check remaining files; deletion is done

    print(f"[sync_sdkconfig] {pioenv}: sdkconfig.{pioenv} is up to date.")


sync_sdkconfig()
