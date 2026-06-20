---
title: "WO3: OTA Pipeline Rewire — Manifest-First Flow"
distilled_at: "2026-06-20T11:42:00Z"
original_plan_id: "pr-36-manifest-first-ota"
commit: "337effe"
artifacts:
  - context_artifacts/adr_manifest_first_rewire.md
  - context_artifacts/graveyard_rewire.md
---

# WO3 Context Bridge

## Executive Summary

WO3 rewired `performUpdateTask()` from GitHub-API-coupled OTA to manifest-first flow. Replaced ~300 lines of GithubClient/floating-tag/asset-search code with ~230 lines using `fetchAndVerifyManifest_()` → `findChannel()` → `findAssetForBoard()` → direct URL download. `FrameworkConfig.otaGithubRepo` removed, `otaGithubToken` renamed to `otaToken`. Both `esp32dev` and `esp32s3nano` compile clean.

## Next Steps (WO4)

1. Update `deploy_release.py` to generate v2 manifest with risk-floor channel scanning
2. Sign manifest with `openssl dgst` (same ECDSA key as firmware)
3. Upload manifest + signature to fixed `ota-manifest` release tag on GitHub
4. Firmware asset URLs in manifest must be absolute GitHub release download URLs

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_manifest_first_rewire.md` - Architecture decisions: no risk-floor on-device, `checkForUpdate()` `#if 0`, channel-switch handling, signature URL guard
- `context_artifacts/graveyard_rewire.md` - Case-sensitive git paths, MCP build serialisation, `checkForUpdate()` stub warning for WO5
