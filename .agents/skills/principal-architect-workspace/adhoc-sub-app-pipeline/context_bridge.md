---
title: Sub-App Build Pipeline Alignment — Data Merge, Portal Builder, OTA Separation
distilled_at: 2026-06-30T20:58:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_framework_data_merge.md
  - context_artifacts/adr_ota_separation.md
  - context_artifacts/state_sub_app_pipeline.md
---

# Context Bridge

## Executive Summary

Aligned the template-app and sample-app build pipelines with the root framework project. Created a new `merge_framework_data.py` pre-build script that copies framework-owned LittleFS files (like `timezones.json`) into sub-app `data/` directories at build time, maintaining a single source of truth. Fixed the sample-app's OTA manifest URL which incorrectly pointed at the framework's releases repo. Added the missing `esp32s3nano` environment to the template-app.

## Next Steps

1. **Timezone download fallback**: Consider adding a build-time or first-boot check in `TimeManager` that gracefully handles a missing `timezones.json` — either by downloading it or bundling a minimal fallback in PROGMEM.
2. **Sample-app OTA setup**: If the Weather Station sample-app is ever meant to be deployed as a real product, it needs its own releases repo, signing keypair, and manifest URL (the TODO comments are now in place).
3. **`merge_framework_data.py` extension**: If future framework modules need LittleFS files (beyond `timezones.json`), they'll be automatically picked up — no script changes needed, just drop the file in root `data/`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_framework_data_merge.md` - Why we chose a pre-build copy strategy over symlinks, submodules, or PIO data_dir overrides
- `context_artifacts/adr_ota_separation.md` - Why the sample-app should not share the framework's OTA releases repo
- `context_artifacts/state_sub_app_pipeline.md` - Final state of all three projects' platformio.ini and build scripts
