---
title: PR-19 WO1 — sdkconfig Base+Delta Restructure
distilled_at: 2026-06-06T11:05:00Z
original_plan_id: pr-19-sdkconfig-layering
status: COMPLETED
branch: pr-19/wo1-base-delta
commit: "8998538"
artifacts:
  - context_artifacts/adr_sdkconfig_layering.md
  - context_artifacts/graveyard_build_pitfalls.md
---

## Executive Summary

WO1 is **fully complete and approved**. The `sdkconfig.defaults` / `sdkconfig.defaults.esp32s3nano` pair
has been restructured from two independent 90%-duplicate files into a canonical base+delta architecture.
The S3 Nano environment in `platformio.ini` now uses `board_build.cmake_extra_args` to layer base then
delta via the official ESP-IDF semicolon-list mechanism. Both boards build cleanly. The PSRAM regression
from PR-14 WO9 is confirmed fixed (`CONFIG_SPIRAM=y` verified in the generated `sdkconfig.esp32s3nano`).

## Current State

- **Branch:** `pr-19/wo1-base-delta`
- **Commit:** `8998538` — "refactor(build): restructure sdkconfig to canonical base+delta layering"
- **`sdkconfig.defaults`:** 140 lines — shared base for ALL boards, header promotes it as canonical
- **`sdkconfig.defaults.esp32s3nano`:** 22 lines — true delta: stack size, PSRAM (4 keys), TLS out buffer, USB-JTAG console
- **`platformio.ini`:** `[env:esp32s3nano]` uses `board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3nano"`
- **Generated configs deleted** — will regenerate on next build from the new pair

## Next Steps (for WO2 agent)

WO2 is `scripts/sync_sdkconfig.py` — a drift detection script. Key information needed:
1. The base file is `sdkconfig.defaults`
2. Delta files are named `sdkconfig.defaults.<boardname>` (currently only `esp32s3nano`)
3. The script should diff base against delta and alert when a setting appears in the delta that also exists
   in the base with the same value (i.e., unnecessary duplication crept back in)
4. It should be wired as a `pre:` script in `platformio.ini` or run as a standalone CI check
5. See `.agents/skills/principal-architect-workspace/pr-19-sdkconfig-layering/` for the project-level PLAN

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_sdkconfig_layering.md` — WHY base+delta was chosen, why `custom_sdkconfig` was removed, why no `sdkconfig.defaults.esp32` needed, WO2 dependency
- `context_artifacts/graveyard_build_pitfalls.md` — managed_components shutil.Error fix, MCP diagnostic false-positive pattern, why stale generated sdkconfigs must be deleted before build
