---
title: "WO1: Manifest v2 Schema — Multi-channel, Absolute URLs, findChannel()"
distilled_at: "2026-06-20T11:18:00Z"
original_plan_id: pr-36-manifest-first-ota
artifacts:
  - context_artifacts/adr_v2_schema_design.md
  - context_artifacts/state_files_changed.md
---

# Context Bridge — WO1: Manifest v2 Schema

## Executive Summary

WO1 is **complete and committed**. The `OTAManifest` struct, parser, and `generate_manifest.py` have been redesigned from a flat single-channel v1 schema to a multi-channel v2 schema with absolute URLs, `manifestVersion`, `manifestUrl`, and a `findChannel()` lookup. The v2 test suite (14 cases) passes on the native target.

## Next Steps

1. **WO2 (Manifest Fetcher):** Implement manifest + signature download, ECDSA signature verification, and URL fallback (learned → compiled-in). This depends on the v2 `OTAManifest` struct delivered by WO1.
2. **WO3 (OTA Pipeline Rewire):** Update `OTAUpdateManager::performUpdateTask()` to use `findChannel()` + `findAssetForBoard(OTAChannel&)` instead of the old flat struct access. This fixes the expected compile errors in `OTAUpdateManager.cpp`.
3. **No v1 backward compatibility** — the old format is completely removed. Any downstream code referencing `manifest.version`, `manifest.channel`, `manifest.assetCount`, or `OTAAsset::firmware`/`signature` will not compile until WO3 migrates it.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_v2_schema_design.md` - Architecture decisions: why multi-channel, why absolute URLs, why hard cut
- `context_artifacts/state_files_changed.md` - Exact files changed with key structural details
