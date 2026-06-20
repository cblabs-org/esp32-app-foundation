# PR-37: Split-Repo OTA with Git-Managed Manifest

**PR Branch:** `pr-37-split-repo-ota`
**Base:** `pr-36-manifest-first-ota`
**Status:** 📋 **PLANNED — awaiting user approval to begin execution**

## Design Decisions

| Decision | Resolution |
|---|---|
| **Public vs private releases repo** | Public. Signed binaries are tamper-proof; public download doesn't increase exposure vs physical device flash dumps. Eliminates PAT entirely. |
| **Manifest storage** | Git-managed file in releases repo (`manifest.json` committed to `main`). NOT a GitHub Release asset. Atomic commits, full history, stable `raw.githubusercontent.com` URL. |
| **Firmware binary storage** | GitHub Release assets in the releases repo (large binaries shouldn't be in git). Public repo means `/releases/download/` URLs work without auth. |
| **GithubClient disposition** | Delete entirely. Dead code since PR-36. Only `ota_root_cas.h` is still used — moves to `lib/OTACerts/`. |
| **OTADownloadEngine rename** | No rename. Name is already GitHub-neutral. Documentation-only de-GitHub-ification. |
| **`FrameworkConfig.otaToken`** | Keep in struct (defaults to `nullptr`). Supports advanced private-releases-repo use case at zero cost. |
| **CA certificate bundle** | No changes needed. `raw.githubusercontent.com` → ISRG Root X1 (already in bundle). `github.com` → Sectigo E46 (already in bundle). |

## Objective

Migrate the OTA distribution model from a single private repo (where GitHub rejects Bearer auth on `/releases/download/` URLs) to a **dedicated public releases repo** with a **git-managed manifest**. This eliminates compiled-in PATs, deletes the dead `GithubClient` library, removes the `manifest-latest` release hack, and simplifies the deploy script by ~170 lines.

### Current Architecture (broken for private repos)
```
esp32-app-foundation (private)
├── Releases: manifest-latest → manifest.json (mutable hack)
├── Releases: v0.0.12-dev.3 → firmware binaries
└── Device needs PAT → Bearer auth → 404 on /releases/download/
```

### Target Architecture
```
esp32-app-foundation (private)        ← source code stays private
esp32-app-foundation-releases (public) ← binaries + manifest
├── manifest.json (git-committed)     ← raw.githubusercontent.com URL
├── manifest.json.sig (git-committed)
└── Releases: v0.0.12-dev.3 → firmware binaries (public download)
```

## Work Order Ledger

- **[ ] [wo1-infrastructure](wo1-infrastructure/WORKORDER.md):** Delete GithubClient, migrate CA certs to `lib/OTACerts/`, update `main.cpp` manifest URL, remove PAT ifdef, update includes
- **[ ] [wo2-deploy-script](wo2-deploy-script/WORKORDER.md):** Replace release-asset manifest flow with git-commit flow, remove floating tags, update URL construction
- **[ ] [wo3-docs-cleanup](wo3-docs-cleanup/WORKORDER.md):** De-GitHub-ify OTADownloadEngine docs, fix manifest field names, update pat-management.md, update ota-update-system.md, update deployment.md

## Serialisation

WOs are serialised: WO1 → WO2 → WO3. WO1 must land first (CA cert move, include path changes). WO2 depends on WO1 (deploy script references the new repo). WO3 is documentation and can run after either.

## Files Touched (per WO)

| WO | Files |
|----|-------|
| WO1 | `lib/GithubClient/` (DELETE), `lib/OTACerts/OTACerts.h` (NEW), `lib/OTACerts/library.json` (NEW), `modules/OTAUpdateManager/OTAUpdateManager.cpp`, `src/main.cpp`, `include/otaSecrets.hpp` (DELETE local), `test/test_unit/test_githubClient/` (DELETE) |
| WO2 | `scripts/deploy_release.py`, `scripts/generate_manifest.py` (minor) |
| WO3 | `lib/OTADownloadEngine/OTADownloadEngine.hpp`, `lib/OTADownloadEngine/OTADownloadEngine.cpp`, `docs/developer/ota-update-system.md`, `.agents/skills/framework-expert/systems/ota/deployment.md`, `.agents/skills/framework-expert/security/pat-management.md`, `modules/OTAUpdateManager/OTAUpdateManager.hpp` |

## Prerequisites

Before WO1 can begin:
1. User must create the `esp32-app-foundation-releases` public repo on GitHub
2. User must confirm `gh` CLI is authenticated for the new repo
3. The existing `v0.0.12-dev.3` release assets need to be re-deployed to the new repo

## Context

- **Parent PR:** PR-36 (manifest-first OTA, all 6 WOs merged on this branch)
- **Context Bridge:** [distillation-20260620-manifest-first-ota](../../context_archives/distillation-20260620-manifest-first-ota/context_bridge.md)
- **ADR:** See [implementation_plan.md](file:///Users/mcneillm/.gemini/antigravity/brain/cd0abc58-fcb4-44e6-8bce-4dd37e3aabc5/implementation_plan.md) for full architectural analysis
