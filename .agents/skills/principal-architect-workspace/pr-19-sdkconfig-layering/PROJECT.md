# PR-19: sdkconfig Architecture — Base+Delta Layering

**Branch:** `pr-19-sdkconfig-layering`
**Base:** `main`
**Status:** 🔄 IN PROGRESS
**Principal Architect Session:** 34de947b-cd38-4c49-8e6b-3c79452f4912

---

## Problem Statement

The project's two `sdkconfig.defaults.*` files are completely independent — not
layered at all. ~90% of their content is verbatim duplication. During PR-14
(OTA production), TCP window sizing and P-384 ECC fixes were applied to
`sdkconfig.defaults` (esp32dev) but never propagated to
`sdkconfig.defaults.esp32s3nano`. This caused `CONFIG_SPIRAM` to become stale
in the generated `sdkconfig.esp32s3nano`, breaking PSRAM on the ESP32-S3 Nano.

There is no mechanism in the build system to detect when a `.defaults` file
changes and invalidate the generated config. This makes the project brittle to
any future shared config changes.

## Architectural Goal

Migrate to the canonical ESP-IDF multi-file layered architecture:

```
sdkconfig.defaults            ← shared base (~120 lines, one copy of everything shared)
sdkconfig.defaults.esp32s3nano ← board delta (~12 lines, ONLY what genuinely differs)
```

Wired together via PlatformIO's `board_build.cmake_extra_args`:
```ini
board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3nano"
```

Add a `sync_sdkconfig.py` pre-build guard so any future change to any `.defaults`
file automatically invalidates the generated config and triggers regeneration.

## Work Orders

| ID | Topic | Branch | Status |
|----|-------|--------|--------|
| **[WO1](wo1-base-delta/WORKORDER.md)** | Restructure sdkconfig files (base+delta) + fix platformio.ini | `pr-19-sdkconfig-layering/wo1-base-delta` | ⬜ QUEUED |
| **[WO2](wo2-sync-script/WORKORDER.md)** | Write `sync_sdkconfig.py` pre-build guard | `pr-19-sdkconfig-layering/wo2-sync-script` | ⬜ QUEUED |

## File Touch Map

| File | WO1 | WO2 |
|------|-----|-----|
| `sdkconfig.defaults` | ✅ | — |
| `sdkconfig.defaults.esp32s3nano` | ✅ | — |
| `sdkconfig.esp32s3nano` (delete) | ✅ | — |
| `sdkconfig.esp32dev` (delete) | ✅ | — |
| `platformio.ini` | ✅ | ✅ |
| `scripts/sync_sdkconfig.py` (new) | — | ✅ |

> **Concurrency:** WO1 and WO2 both touch `platformio.ini`. They MUST be
> serialised. WO2 branches off WO1, not off the feature branch directly.

## Merge Log

| WO | Merged At | Commit |
|----|-----------|--------|
| WO1 | — | — |
| WO2 | — | — |

---
*Context distillation:* [./context_bridge.md](./context_bridge.md) (populated after wrap-up)
