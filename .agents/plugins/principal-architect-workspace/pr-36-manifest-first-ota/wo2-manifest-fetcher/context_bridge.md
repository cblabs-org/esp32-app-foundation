---
title: "WO2: Manifest Fetcher + Signature Verification"
distilled_at: "2026-06-20T11:30:00Z"
original_plan_id: pr-36-manifest-first-ota
artifacts:
  - context_artifacts/adr_manifest_fetcher_design.md
  - context_artifacts/graveyard_v1_v2_compile_fixes.md
  - context_artifacts/state_files_changed.md
---

# WO2 Context Bridge — Manifest Fetcher + Signature Verification

## Executive Summary

Implemented `fetchAndVerifyManifest_()` in `OTAUpdateManager` with full URL fallback (learned → compiled-in), HTTPS CA-pinned → insecure TLS fallback, ECDSA manifest signature verification via `MbedTLSSignatureVerifier`, and learned URL persistence via IConfigurable schema v5. Both `esp32dev` and `esp32s3nano` build cleanly. Commit: `928466e`.

## Next Steps

1. **WO3** — Rewire `performUpdateTask()` to use `fetchAndVerifyManifest_()` instead of the legacy `GithubClient` flow. Replace `otaGithubRepo` with `otaManifestUrl` in `FrameworkConfig`. Remove the temporary pipeline shims added in WO2.
2. **WO4** — Update `deploy_release.py` for v2 manifest generation + signing.
3. The `#if 0` guards on `downloadAndParseManifestVersion_()` (header + implementation) should be fully removed in WO5 cleanup.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_manifest_fetcher_design.md` - Design decisions for fetchAndVerifyManifest_(), downloadRawFile_() helper, URL fallback ordering, and config schema bump
- `context_artifacts/graveyard_v1_v2_compile_fixes.md` - Pre-existing v1→v2 compile errors in performUpdateTask() and the temporary shims applied
- `context_artifacts/state_files_changed.md` - Exact files modified, line ranges, and diff summary
