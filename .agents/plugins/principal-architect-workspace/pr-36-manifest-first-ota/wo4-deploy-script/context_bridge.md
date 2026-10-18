---
title: "WO4 — Deploy Script v2: Manifest Signing, Risk-Floor Scanning, OTA Manifest Release"
distilled_at: 2026-06-20T11:51:00Z
original_plan_id: pr-36-manifest-first-ota
artifacts:
  - context_artifacts/adr_deploy_pipeline_v2.md
  - context_artifacts/adr_risk_floor_logic.md
  - context_artifacts/state_script_interfaces.md
---

# Context Bridge — WO4 Deploy Script + Manifest Signing

## Executive Summary

The deploy pipeline (`deploy_release.py`) has been rewritten to produce signed v2 manifests and upload them to a fixed `ota-manifest` GitHub Release. The `generate_manifest.py` script gained `--merge-into` support for incremental multi-channel manifest building. Floating tags are now behind `--legacy-floating-tags` (default: off) with a `DeprecationWarning`.

## What Was Done

1. **`generate_manifest.py`** — Added `--merge-into` argument: loads an existing manifest and overlays new channel entries. Handles missing/corrupt files gracefully (falls back to empty manifest).

2. **`deploy_release.py`** — Major rewrite:
   - New `sign_manifest()` step (Step 3c) — detached ECDSA signature via `openssl dgst`
   - New `update_ota_manifest_release()` (Step 6) — downloads existing manifest from `ota-manifest` release, merges via `generate_manifest.py --merge-into`, applies risk-floor scanning, signs, uploads
   - New `FirmwareVersion` class — Python SemVer comparison with channel ordering `dev(0) < beta(1) < rc(2) < stable(3)`
   - New `apply_risk_floor()` — ensures each channel has the best available version for its stability floor
   - New `get_repo_slug()` — resolves `owner/repo` via `gh repo view` or `--repo` flag
   - Floating tags moved behind `--legacy-floating-tags` (default: off) with `DeprecationWarning`

## Commit

- `9c265ff` on branch `pr-36-manifest-first-ota-wo4-deploy-script`

## Next Steps

- **WO5:** Remove `GithubClient` from OTA path, remove floating tag code, dead code removal
- **WO6:** Update docs (`ota-update-system.md`, `deployment.md`) and `/framework-expert` skill

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_deploy_pipeline_v2.md` - Architecture decisions for the v2 deploy pipeline structure
- `context_artifacts/adr_risk_floor_logic.md` - Design rationale for risk-floor channel scanning
- `context_artifacts/state_script_interfaces.md` - Current CLI interfaces for both scripts
