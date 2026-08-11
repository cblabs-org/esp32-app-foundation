---
title: Include Architecture Alignment & Compliance Remediation
distilled_at: 2026-08-11T22:21:00Z
original_plan_id: standalone
commit: 1e58058
artifacts:
  - context_artifacts/adr_include_architecture.md
  - context_artifacts/graveyard_relative_includes.md
  - context_artifacts/state_library_json_manifest.md
---

# Context Bridge

## Executive Summary

Resolved a fundamental configuration asymmetry between native and target PlatformIO environments (LDF `chain` vs `deep` mode) that was the root cause of fragile relative `../` include paths throughout the codebase. All 59 files committed, 482/482 native tests pass, all target builds clean, device verified on hardware.

## Next Steps

1. **No immediate code work required** — commit `1e58058` is pushed to `main` and flashed to the CrowPanel.
2. **Future module authors**: just add a `library.json` with `{"name": "ModuleName", "build": {"flags": ["-I../../include"]}}` and declare cross-boundary `dependencies`. LDF resolves everything automatically.
3. **`pio run -e native`** (non-test build) still fails with `_main` undefined — this is expected because `src/main.cpp` is `#ifdef`'d out on native and GoogleTest provides its own `main()`. Only `pio test -e native` is meaningful.
4. **Consider**: the `pio run -e native` failure could be cleaned up by adding a `src_filter` to `platformio.ini` that excludes `src/main.cpp` on native, but this is cosmetic.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_include_architecture.md` - Why we aligned LDF modes, removed Application from lib_ignore, and adopted the exclude_on_native.py pattern
- `context_artifacts/graveyard_relative_includes.md` - Failed approaches: per-file relative path reverts, strict compat mode, missing dependency declarations
- `context_artifacts/state_library_json_manifest.md` - Complete manifest of all 22 library.json files and their dependency declarations
