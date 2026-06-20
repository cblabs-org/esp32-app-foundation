---
title: "PR-37 Split-Repo OTA: Completed — Git-Managed Manifest, Deploy Safety Gates, Full Migration"
distilled_at: "2026-06-20T23:45:00Z"
original_plan_id: "pr-37-split-repo-ota"
artifacts:
  - context_artifacts/adr_deploy_safety_gates.md
  - context_artifacts/graveyard_ota_debugging.md
  - context_artifacts/state_final.md
---

# Context Bridge: PR-37 Split-Repo OTA

## Executive Summary

Migrated the entire OTA distribution model from a single private repo with
`manifest-latest` release-asset hack to a **public releases repo** with a
**git-managed manifest** at `raw.githubusercontent.com`. Deleted GithubClient
(dead code), centralised CA certs, eliminated PAT requirement, rewrote the
deploy script with three pre-flight safety gates discovered during live testing,
and de-GitHub-ified all OTA documentation. Verified end-to-end:
**dev.7 → dev.8** (bridge via legacy manifest) then **dev.8 → dev.9**
(new `raw.githubusercontent.com` flow).

## Work Orders Completed

| WO | Scope | Key Changes |
|----|-------|-------------|
| WO1 | Infrastructure | Deleted `lib/GithubClient/`, created `lib/OTACerts/`, updated `main.cpp` manifest URL to `raw.githubusercontent.com`, removed PAT ifdef |
| WO2 | Deploy script rewrite | `update_manifest()` git-commit flow, `--releases-dir` with 3-level resolution, deleted `update_floating_tags()` + `--legacy-floating-tags`, 3 safety gates |
| WO3 | Documentation cleanup | 6 files de-GitHub-ified, field names fixed (`firmware`→`firmwareUrl`), X-GitHub-Api-Version header removed, PAT guide updated |

## Bugs Fixed During Live Testing

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| Off-by-one `sigBuf[256]→[257]` | ECDSA signatures are 256 bytes but `snprintf` needs +1 for null terminator, causing buffer overflow in hex conversion | `OTADownloadEngine.cpp` — increased buffer |
| v-prefix mismatch | `strncmp` compared manifest version (no `v`) against binary version (has `v`), always mismatching | `OTADownloadEngine.cpp` — strip `v`/`V` from both sides |
| Stale `PROJECT_VER` after tag change | ESP-IDF CMake caches `PROJECT_VER`; incremental builds don't recompile `esp_app_desc.c` when only git tags change | Deploy script Gate 3: `pio run -t clean` before `pio run` |
| Dirty tree appends `-dirty` to binary version | `git describe --dirty` produces `v0.0.12-dev.7-dirty` which never matches manifest | Deploy script Gate 1: `git status --porcelain` abort |
| Wrong tag on HEAD | Multiple tags on HEAD causes `git describe` to pick the wrong one | Deploy script Gate 2: `git describe --tags --exact-match HEAD` must match `--tag` |
| Device fetching old manifest URL | OTA'd firmware (dev.7) was built before WO1 changed `main.cpp` — still had old `manifest-latest` URL | Bridge strategy: updated old `manifest-latest` to point at dev.8; dev.8 firmware has new URL |

## Architecture Decision: Deploy Safety Gates

Three pre-flight gates added to `deploy_release.py` based on real failures:

1. **Clean Working Tree** — `git status --porcelain` must be empty
2. **Tag Describes HEAD** — `git describe --tags --exact-match HEAD` must match `--tag`
3. **Clean Build** — when `--build`, runs `pio run -t clean` first

These prevent version string mismatches between `esp_app_desc_t.version` (ESP-IDF
CMake) and the manifest version, which cause the device to reject valid firmware.

## Architecture Decision: Bridge OTA Migration

When migrating manifest URLs, firmware already in the field still fetches from the
old URL. Strategy: temporarily update the old manifest location to point at the
new firmware version (which contains the new URL). After one OTA cycle, the device
uses the new URL for all future checks. The old manifest location can then be deleted.

## Superseded Context Archives

The following archives in `.agents/context_archives/` are now **fully superseded**
by this distillation:

| Archive | Status | Reason |
|---------|--------|--------|
| `distillation-20260619-ota-floating-tags` | ⚠️ **SUPERSEDED** | Floating tags deleted in WO2; replaced by git-managed manifest |
| `distillation-20260620-manifest-first-ota` | ⚠️ **SUPERSEDED** | PR-36 integration testing complete; PAT eliminated; all blockers resolved |
| `distillation-20260619-ota-upload-fix` | ✅ **Still valid** | Manual upload fix (PsychicHTTP filename matching) is independent of OTA distribution |

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly
> requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_deploy_safety_gates.md` — Why each gate exists, the specific failure mode it prevents, and the exact subprocess calls
- `context_artifacts/graveyard_ota_debugging.md` — All failed debugging attempts: serial port DTR/RTS stuck, CDN cache lag, `checkForUpdate()` stub confusion, `_silentCheck` error swallowing
- `context_artifacts/state_final.md` — Final verified state: git tags, releases, manifest content, device firmware version
