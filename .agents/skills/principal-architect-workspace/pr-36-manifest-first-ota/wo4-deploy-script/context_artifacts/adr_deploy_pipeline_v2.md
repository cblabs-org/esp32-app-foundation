# ADR: Deploy Pipeline v2 Architecture

## Decision

Restructure `deploy_release.py` from a linear 6-step pipeline to an 7-step pipeline (with Step 7 legacy-only) that separates the versioned release from the manifest release.

## Context

The v1 pipeline attached `manifest.json` to the versioned release alongside firmware binaries. The v2 architecture requires a single `ota-manifest` fixed release that aggregates all channels. This means the deploy script must:

1. Create the versioned release (firmware + per-release manifest) — unchanged
2. Download the existing global manifest, merge in the new channel, and re-upload — new

## Key Decisions

### Staging directory separation
The OTA manifest staging uses `./release/ota-manifest-staging/` rather than mixing into `./release/`. This prevents the versioned release upload from accidentally including the merged multi-channel manifest.

### `get_repo_slug()` auto-detection
Uses `gh repo view --json nameWithOwner` to auto-detect the repo slug when `--repo` is not provided. Falls back to `UNKNOWN/REPO` with a warning. This avoids requiring `--repo` for the common case (deploying from the source repo).

### Manifest signing in two places
The per-release manifest (Step 3c) and the merged ota-manifest (Step 6d) are signed independently. The per-release copy is a snapshot; the ota-manifest copy is the live one. Both signatures use the same key.

### `ota-manifest` release creation
The script creates the release if it doesn't exist (`check=False` on the create command), then uploads with `--clobber`. This is idempotent.
