---
name: pr-19-wo2
description: Handoff instructions for Work Order 2 — sync_sdkconfig.py pre-build guard
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Write `scripts/sync_sdkconfig.py` — a PlatformIO pre-build script that watches
all SDKCONFIG_DEFAULTS source files for an environment and automatically deletes
the stale generated `sdkconfig.<env>` file if any source is newer. Wire it into
`platformio.ini` as the first `extra_scripts` entry. This closes the build
system gap identified as the root cause of the PSRAM regression.

## Context You Must Know

**Why this script is needed:** ESP-IDF's `kconfig.cmake` intentionally omits
`sdkconfig.defaults.*` from its `CMAKE_CONFIGURE_DEPENDS` watch list. Once
`sdkconfig.<env>` exists on disk, ESP-IDF ignores `.defaults` files completely
until the generated file is deleted. This script adds the missing invalidation
layer.

**How the build system works in this project (dual-framework mode):**
- Framework: `arduino, espidf` — PlatformIO's Arduino sdkconfig processor is
  bypassed (`flag_custom_sdkconfig = False`)
- `SDKCONFIG_PATH` is always `sdkconfig.<PIOENV>` in the project root
  (from `espidf.py` line 213-214)
- The `board_build.cmake_extra_args` value (set in WO1) contains the
  `SDKCONFIG_DEFAULTS` list as a CMake arg string

**After WO1, the relevant platformio.ini state is:**
```ini
[env:esp32dev]
custom_sdkconfig = sdkconfig.defaults          # single file

[env:esp32s3nano]
board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3nano"
```

**Script contract:**
1. For `esp32s3nano`: parse `board_build.cmake_extra_args` to extract the
   semicolon-separated `SDKCONFIG_DEFAULTS` list → watch all files in the list
2. For `esp32dev`: read `custom_sdkconfig` → watch that single file
3. For `native` or any env without sdkconfig: skip gracefully
4. If any source file is newer than the generated `sdkconfig.<env>` → delete
   the generated file → print a clear `[sync_sdkconfig]` message
5. If generated file doesn't exist → print informational message (normal first build)

**Follow the house style of existing scripts:** Use the same module header,
docstring, and comment style as `scripts/inject_version.py`. Read that file
for reference before writing a single line.

## Strict Boundary Box

- ❌ Do NOT touch `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3nano`, or `platformio.ini` sections other than `extra_scripts`
- ❌ Do NOT touch any `.cpp` or `.h` files
- ❌ Do NOT write any files to the build directory (`.pio/`)
- ❌ Do NOT add any third-party imports — only `os`, `re`, and `time` from stdlib
- ✅ You are working on branch `pr-19-sdkconfig-layering/wo2-sync-script`

## Execution Checklist

### Step 1 — Read reference files

- [ ] Read `scripts/inject_version.py` in full (for house style reference)
- [ ] Read `scripts/portalBuilder.py` in full (for SCons `Import("env")` pattern)
- [ ] Read current `platformio.ini` `extra_scripts` entry (to know where to insert)

### Step 2 — Write `scripts/sync_sdkconfig.py`

Create the file following the house style. The script must:

**Header block** (matching `inject_version.py` style):
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
```

**Core logic:**

```python
Import("env")
import os
import re

def _get_defaults_files(env):
    """Return list of .defaults file paths to watch for this environment.
    
    Checks board_build.cmake_extra_args first (new layered style),
    then falls back to custom_sdkconfig (legacy single-file style).
    Returns empty list if neither is configured (e.g. native env).
    """
    project_dir = env.subst("$PROJECT_DIR")
    
    # Strategy 1: board_build.cmake_extra_args with SDKCONFIG_DEFAULTS="a;b"
    try:
        cmake_args = env.GetProjectOption("board_build.cmake_extra_args", "")
        match = re.search(r'-DSDKCONFIG_DEFAULTS=["\']?([^"\'\\s]+)["\']?', cmake_args)
        if match:
            raw = match.group(1)
            return [os.path.join(project_dir, f.strip()) for f in raw.split(";") if f.strip()]
    except Exception:
        pass
    
    # Strategy 2: custom_sdkconfig (single file, legacy)
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
```

### Step 3 — Update `platformio.ini` — add to `extra_scripts`

Find the `extra_scripts` entry in `[env:base_esp32]` (or wherever it lives).
Add `pre:scripts/sync_sdkconfig.py` as the **first** entry:

```diff
-extra_scripts = pre:scripts/run_web_tests.py
+extra_scripts = pre:scripts/sync_sdkconfig.py
+                pre:scripts/run_web_tests.py
                 pre:scripts/portalBuilder.py
                 pre:scripts/inject_version.py
```

> It must be `pre:` and must run FIRST so stale config is deleted before any
> other pre-build script could trigger CMake.

### Step 4 — Smoke test the script

Run `pio run -e esp32s3nano` and verify:
- `[sync_sdkconfig] esp32s3nano: sdkconfig.esp32s3nano is up to date.` appears
  in build output (or deletion message if the file is stale)
- Build completes successfully

Run `touch sdkconfig.defaults.esp32s3nano && pio run -e esp32s3nano` and verify:
- `[sync_sdkconfig] esp32s3nano: sdkconfig.defaults.esp32s3nano is newer than sdkconfig.esp32s3nano`
  message appears
- `sdkconfig.esp32s3nano` is deleted and regenerated
- `grep CONFIG_SPIRAM sdkconfig.esp32s3nano` still shows `CONFIG_SPIRAM=y`

Run `pio run -e esp32dev` and verify no regression.

### Step 5 — Commit

```bash
git add scripts/sync_sdkconfig.py platformio.ini
git commit -m "feat(build): add sync_sdkconfig.py pre-build invalidation guard

Adds a PlatformIO pre-build script that detects when any sdkconfig.defaults.*
source file is newer than the generated sdkconfig.<env> file and deletes the
stale generated config, forcing ESP-IDF to regenerate from the full
SDKCONFIG_DEFAULTS list on the next CMake configure pass.

Root cause addressed: ESP-IDF's kconfig.cmake intentionally omits
sdkconfig.defaults.* from CMAKE_CONFIGURE_DEPENDS. This script adds
the missing invalidation edge between the source (.defaults) and the
generated output (sdkconfig.<env>) that ESP-IDF itself does not track.

Supports two configuration styles:
  - board_build.cmake_extra_args with SDKCONFIG_DEFAULTS list (S3 Nano)
  - custom_sdkconfig single file (esp32dev legacy)
Skips gracefully for native env and unconfigured environments."
```

## Verification Checklist (Before Reporting Back)

- [ ] `scripts/sync_sdkconfig.py` created with correct house-style header
- [ ] `_get_defaults_files()` handles both `board_build.cmake_extra_args` and `custom_sdkconfig`
- [ ] `sync_sdkconfig()` prints clear `[sync_sdkconfig]` messages in all code paths
- [ ] Script is silent/graceful for `native` env
- [ ] `platformio.ini` `extra_scripts` updated with `pre:scripts/sync_sdkconfig.py` as first entry
- [ ] `touch` test passes — stale file detected and deleted, then regenerated with correct content
- [ ] `pio run -e esp32dev` passes — no regression
- [ ] Commit made on branch `pr-19-sdkconfig-layering/wo2-sync-script`
- [ ] **Ask the user if they are happy before distilling or doing anything else**

Once the user approves, invoke the `@distillery` skill targeting
`.agents/skills/principal-architect-workspace/pr-19-sdkconfig-layering/wo2-sync-script/`
and then terminate your session.
