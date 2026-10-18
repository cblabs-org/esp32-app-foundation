# State: sync_sdkconfig.py — Final Implementation

**Session:** WO2  
**Commit:** `9e19d2c` on branch `pr-19/wo2-sync-script`  
**Date:** 2026-06-06

## File: `scripts/sync_sdkconfig.py`

```python
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

    Supported environments: esp32dev, esp32s3nano
    Skips gracefully: native env, envs without sdkconfig configuration.
"""

Import("env")

import os
import re


def _get_defaults_files(env):
    """...(see code)..."""
    project_dir = env.subst("$PROJECT_DIR")

    # Strategy 1: board_build.cmake_extra_args via BoardConfig
    try:
        board = env.BoardConfig()
        cmake_args = board.get("build.cmake_extra_args", "")
        match = re.search(r'-DSDKCONFIG_DEFAULTS=["\'"]?([^"\'\s]+)["\'"]?', cmake_args)
        if match:
            raw = match.group(1)
            return [os.path.join(project_dir, f.strip()) for f in raw.split(";") if f.strip()]
    except Exception:
        pass

    # Strategy 2: custom_sdkconfig (legacy single file)
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
        return  # skip silently

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
            return

    print(f"[sync_sdkconfig] {pioenv}: sdkconfig.{pioenv} is up to date.")


sync_sdkconfig()
```

## platformio.ini Change (diff)

```diff
-extra_scripts         = pre:scripts/run_web_tests.py
+extra_scripts         = pre:scripts/sync_sdkconfig.py
+                        pre:scripts/run_web_tests.py
                         pre:scripts/portalBuilder.py
                         pre:scripts/inject_version.py
```

## Verified Build Outputs

### esp32s3nano — up to date
```
[sync_sdkconfig] esp32s3nano: sdkconfig.esp32s3nano is up to date.
========================= [SUCCESS] Took 13.42 seconds =========================
```

### esp32s3nano — stale (touch test)
```
[sync_sdkconfig] esp32s3nano: sdkconfig.defaults.esp32s3nano is newer than sdkconfig.esp32s3nano — deleting stale generated config.
[sync_sdkconfig] esp32s3nano: ESP-IDF will regenerate from SDKCONFIG_DEFAULTS on next CMake configure.
========================= [SUCCESS] Took 13.37 seconds =========================
```
Post-touch: `grep CONFIG_SPIRAM sdkconfig.esp32s3nano` → `CONFIG_SPIRAM=y` ✅

### esp32dev — first build (no generated config)
```
[sync_sdkconfig] esp32dev: no generated config found — will be created from defaults on first build.
========================= [SUCCESS] Took 66.41 seconds =========================
```
