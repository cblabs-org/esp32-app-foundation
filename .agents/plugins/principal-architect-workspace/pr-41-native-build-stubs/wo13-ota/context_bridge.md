---
title: OTA Native Build Stubs Distillation
distilled_at: 2026-07-01T08:44:00Z
original_plan_id: wo13-ota
artifacts:
  - context_artifacts/adr_ota_native_stubs.md
  - context_artifacts/graveyard_ota_native_stubs.md
---

## Executive Summary
Stripped `#ifndef NATIVE_BUILD` guards across `OTAUpdateManager`, `OTADownloadEngine`, and `MbedTLSSignatureVerifier`, and replaced them with robust test stubs to mock ESP-IDF APIs. Verified native build stability and ensured that tests correctly target the native stub behavior instead of hardcoded skipped guard paths. Hardware ESP32S3 builds were also verified as functional.

## Next Steps
This workorder is complete and validated. The branch is clean and changes have been committed. Ready for PR-41 integration or the next work order.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_ota_native_stubs.md` - ADR covering why specific stubs (like `esp_ota_ops.h` and `PsychicUploadHandler.h`) were needed instead of #ifdefs.
- `context_artifacts/graveyard_ota_native_stubs.md` - Details of failed SIGHUP and duplicate definition errors encountered while stubbing.
