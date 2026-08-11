# Graveyard: esp32dev Bootloader Linker Failure

## What failed

`esp32dev` environment failed to build with:
```
/path/to/esp32.rom.libc-funcs.ld: duplicated input file
```

## Root cause

pioarduino's `espidf.py:build_bootloader()` (L1955–2100) double-injects `esp32.rom.libc-funcs.ld`:
1. Once via CMake's `INTERFACE_LINK_LIBRARIES` (merged at L2011)
2. Again via the processed `-T` flags re-append (L2087)

Old binutils ignored duplicate `-T` flags. GCC 14.2.0 ships binutils 2.40+ which makes it **fatal**.

## What was tried

1. **`scripts/dedup_linkflags.py`** — a pre-build SCons script to deduplicate `LINKFLAGS`. **Did not work** because the duplication happens inside the platform builder's bootloader subprocess, not in our project's SCons environment. The script was reverted.

2. **Direct investigation** of `~/.platformio/platforms/espressif32/builder/frameworks/espidf.py` confirmed the bug is in the platform code, not our project.

## Resolution

This is an **upstream pioarduino bug**. The user spawned a separate session to report it. The fix needs to go into `espidf.py:build_bootloader()` to deduplicate `-T` flags before passing them to the linker. Our project cannot fix it.

## Current status

The user reported finding a fix in a separate session — `esp32dev` builds were passing by 2026-08-05. The fix was likely applied via a platform update or local patch.
