---
name: pr-36-wo4
description: Handoff instructions for Work Order 4 (Deploy Script + Manifest Signing)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Update `deploy_release.py` and `generate_manifest.py` to produce the v2 signed manifest format and upload it to a fixed `ota-manifest` GitHub Release. The versioned releases continue to hold the firmware binaries — the manifest just points to them with absolute URLs.

# Strict Boundary Box
- DO NOT touch any C++ source files.
- DO NOT modify the firmware signing flow — it is unchanged.
- Keep the versioned release creation (Step 4-5 in the current script) intact.

# Tooling Constraints
Test the scripts locally with `--dry-run` to verify command generation.

# Context
Read the project plan for the full manifest schema and hosting design:
`.agents/plugins/principal-architect-workspace/pr-36-manifest-first-ota/PROJECT.md`

# Line-Item Instructions

## generate_manifest.py — v2 Output

- [ ] Add `--manifest-url` argument (required) — the self-referencing URL.
- [ ] Add `--base-url` argument (required) — prefix for firmware/signature asset URLs (e.g. `https://github.com/owner/repo/releases/download/v1.0.0`).
- [ ] Update `build_manifest()` to output the v2 JSON structure:
  - `manifestVersion: 2`
  - `manifestUrl` from the argument
  - Single channel entry (the script is called once per channel by deploy_release.py)
  - Asset URLs constructed as `{base_url}/{firmware_filename}` and `{base_url}/{signature_filename}`
- [ ] Add a `--merge-into` argument that accepts an existing manifest.json path. If provided, read the existing manifest and add/update the channel entry rather than creating from scratch. This allows the deploy script to build up a multi-channel manifest incrementally.
- [ ] **Hard cut to v2** — no v1 backward compatibility. Remove the old v1 format output entirely.

## deploy_release.py — Manifest Signing + Risk-Floor Scanning + Fixed Release

- [ ] After generating manifest.json, sign it: `openssl dgst -sha256 -sign {private_key} -out manifest.json.sig manifest.json`
- [ ] Add a new step after the versioned release upload: "Update OTA manifest release"
  1. Compute `base_url` = `https://github.com/{repo}/releases/download/{tag}` (using `--repo` if provided).
  2. Compute `manifest_url` = `https://github.com/{repo}/releases/download/ota-manifest/manifest.json`.
  3. Download the existing `manifest.json` from the `ota-manifest` release (if it exists) to use as the base. If the release doesn't exist, start with an empty manifest.
  4. Call `generate_manifest.py` with `--base-url`, `--manifest-url`, and `--merge-into` (pointing to the downloaded manifest).
  5. **Risk-floor scanning:** After merging the new channel entry, scan ALL channel entries in the manifest. For each channel, ensure its version is the best available for that stability floor. Example: if deploying `v1.0.0` stable and the manifest's `dev` channel currently has `v0.0.9-dev.5`, update the `dev` entry to `v1.0.0` (stable ≥ dev floor). This ensures a device on `dev` always gets the highest version regardless of channel.
  6. Sign the merged manifest.
  7. Upload `manifest.json` + `manifest.json.sig` to the `ota-manifest` release (create if not exists, `--clobber` to overwrite).
- [ ] The risk-floor scanning should use the same `FirmwareVersion` comparison logic as the firmware (reimplement in Python or use SemVer parsing). The channel ordering is: dev < beta < rc < stable.
- [ ] Keep the floating tag creation code but behind a `--legacy-floating-tags` flag (default: off). Add a deprecation notice.

# Verification Requirement
Run the script with `--dry-run` and verify the generated commands are correct. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

---

# Status Update

**Status:** ✅ COMPLETE
**Commit:** `9c265ff` on `pr-36-manifest-first-ota-wo4-deploy-script`
**Date:** 2026-06-20
**Agent:** WO4 worker (conversation `3ee36ddc-bf1e-402e-ace6-febc05f506ff`)

## Summary
All checklist items implemented and verified via dry-run and unit tests. Manifest signing, risk-floor scanning, `--merge-into` merge, and `ota-manifest` release upload all operational. Floating tags moved behind `--legacy-floating-tags` (default: off) with `DeprecationWarning`.
