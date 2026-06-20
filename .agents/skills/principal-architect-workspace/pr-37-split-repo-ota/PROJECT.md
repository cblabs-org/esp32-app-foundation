# PR-37: Split-Repo OTA with Git-Managed Manifest

**PR Branch:** `pr-37-split-repo-ota`
**Base:** `pr-36-manifest-first-ota`
**Status:** ✅ **COMPLETE** — 3 WOs merged, OTA verified dev.7→dev.8→dev.9

## Design Decisions

| Decision | Resolution |
|---|---|
| **Public vs private releases repo** | Public. Signed binaries are tamper-proof; public download doesn't increase exposure vs physical device flash dumps. Eliminates PAT entirely. |
| **Manifest storage** | Git-managed file in releases repo (`manifest.json` committed to `main`). NOT a GitHub Release asset. Atomic commits, full history, stable `raw.githubusercontent.com` URL. |
| **Firmware binary storage** | GitHub Release assets in the releases repo (large binaries shouldn't be in git). Public repo means `/releases/download/` URLs work without auth. |
| **GithubClient disposition** | Deleted entirely. Dead code since PR-36. CA certs moved to `lib/OTACerts/`. |
| **Deploy safety gates** | 3 gates added from live testing: clean tree, tag verification, clean build. See [ADR](context_artifacts/adr_deploy_safety_gates.md). |
| **Legacy migration** | Bridge strategy: update old `manifest-latest` to point at firmware with new URL, then delete after migration. |

## Work Order Ledger

- **[x] [wo1-infrastructure](wo1-infrastructure/WORKORDER.md):** Delete GithubClient, migrate CA certs to `lib/OTACerts/`, update `main.cpp` manifest URL, remove PAT ifdef — **MERGED**
- **[x] [wo2-deploy-script](wo2-deploy-script/WORKORDER.md):** Replace release-asset manifest flow with git-commit flow, remove floating tags, add safety gates — **MERGED**
- **[x] [wo3-docs-cleanup](wo3-docs-cleanup/WORKORDER.md):** De-GitHub-ify OTADownloadEngine docs, fix manifest field names, update PAT guide — **MERGED**

## Bugs Fixed

| Bug | Fix |
|-----|-----|
| `sigBuf[256]→[257]` off-by-one | Buffer overflow in hex conversion |
| v-prefix mismatch in version comparison | Strip `v`/`V` from both sides of `strncmp` |
| Stale `PROJECT_VER` in incremental builds | Gate 3: `pio run -t clean` before build |
| Dirty tree `-dirty` suffix | Gate 1: `git status --porcelain` abort |
| Wrong tag on HEAD | Gate 2: `git describe --exact-match` verification |

## OTA Verification

| Transition | Method | Result |
|-----------|--------|--------|
| dev.6 → dev.7 | Boot auto-check, manifest-latest | ✅ |
| dev.7 → dev.8 | Bridge via updated manifest-latest | ✅ |
| dev.8 → dev.9 | New `raw.githubusercontent.com` flow | ✅ |

## Distillation

- [context_bridge.md](./context_bridge.md)

## Supersedes

- `.agents/context_archives/distillation-20260619-ota-floating-tags/` — floating tags deleted
- `.agents/context_archives/distillation-20260620-manifest-first-ota/` — PAT eliminated, all blockers resolved
