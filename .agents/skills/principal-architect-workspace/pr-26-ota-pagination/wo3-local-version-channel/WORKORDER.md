---
name: pr-26-wo3
description: Handoff instructions for Work Order 3 — Local Version Channel Fix
complexity: mechanical
---

# Role

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Fix the local version comparison in `OTAUpdateManager.cpp` which hardcodes `Channel::STABLE` — causing devices running pre-release firmware (dev/beta) to incorrectly compare as stable, silently blocking OTA updates. Also add the missing `VERSION_CHANNEL` and `VERSION_PRE_NUM` build flags to the native test environment in `platformio.ini`.

# Strict Boundary Box

- **ONLY modify:** `modules/OTAUpdateManager/OTAUpdateManager.cpp` and `platformio.ini`
- **Do NOT touch:** `lib/GithubClient/`, `web/`, or any `.hpp` files
- **Do NOT touch:** any log lines (those were WO2's responsibility)

# Line-Item Instructions

## 3a — `checkForUpdate()` (~line 298)

In `modules/OTAUpdateManager/OTAUpdateManager.cpp`, find this exact line:
```cpp
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, Channel::STABLE, 0 };
```
This appears in `checkForUpdate()`. Replace with:
```cpp
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
                              static_cast<Channel>(VERSION_CHANNEL), VERSION_PRE_NUM };
```

## 3b — `performUpdateTask()` (~line 483)

Find the **second occurrence** of the same line in `performUpdateTask()`:
```cpp
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, Channel::STABLE, 0 };
```
Replace with (identical replacement):
```cpp
    FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
                              static_cast<Channel>(VERSION_CHANNEL), VERSION_PRE_NUM };
```

**Important:** There are exactly 2 occurrences of this line in the file. Both must be replaced.

## 3c — `platformio.ini` `[env:native]` section (~line 111)

In `platformio.ini`, inside the `[env:native]` section, find:
```ini
    -DBUILD_VERSION=\"0.0.0-native\"
    -DSECURITY_VERSION=1
    -I include
```
Replace with:
```ini
    -DBUILD_VERSION=\"0.0.0-native\"
    -DVERSION_MAJOR=0
    -DVERSION_MINOR=0
    -DVERSION_PATCH=0
    -DVERSION_CHANNEL=3
    -DVERSION_PRE_NUM=0
    -DSECURITY_VERSION=1
    -I include
```

**Why:** Hardware builds get `VERSION_CHANNEL` and `VERSION_PRE_NUM` from `inject_version.py` (which parses git tags). The native test environment doesn't run `inject_version.py`, so these flags must be defined manually. `VERSION_CHANNEL=3` means `Channel::STABLE` and `VERSION_PRE_NUM=0` means no pre-release number — safe defaults for native tests.

# Verification

1. Run: `pio test -e native` — all tests must pass
2. Run: `pio run -e esp32s3nano` — build must succeed (no need to flash)

# Completion

After verification passes, **STOP and send a message back to the Principal Architect** reporting:
- Whether native tests passed
- Whether the esp32s3nano build succeeded
- Any issues encountered
- Do NOT commit, distill, or wrap up until explicitly told to do so

## Status Update (2026-06-15)
- **Status:** Completed & Verified
- **Verification Details:**
  - `pio test -e native` succeeded (562/562 unit tests passed).
  - `pio run -e esp32s3nano` compiled successfully.
- **Implemented Changes:**
  - Resolved hardcoded local channel version mismatch in `checkForUpdate()` and `performUpdateTask()` in `OTAUpdateManager.cpp` by using `VERSION_CHANNEL` and `VERSION_PRE_NUM`.
  - Added test fallbacks to `platformio.ini` `[env:native]` build flags.
