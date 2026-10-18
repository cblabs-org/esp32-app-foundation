---
name: pr-36-wo5
description: Handoff instructions for Work Order 5 (Cleanup — Remove GithubClient from OTA)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Remove `GithubClient` from the OTA pipeline, remove dead code from the floating-tag era, and clean up any remaining references to the old architecture.

# Strict Boundary Box
- DO NOT delete `lib/GithubClient/` entirely — it may have non-OTA uses. Only remove its usage from OTAUpdateManager.
- DO NOT modify `OTADownloadEngine` or the signature verification flow.
- DO NOT touch deploy scripts (WO4 already handled those).

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`) to compile and verify your code instead of running raw `pio` CLI commands.

# Line-Item Instructions

## OTAUpdateManager.cpp — Remove GithubClient Usage

- [ ] Remove `#include "../../lib/GithubClient/GithubClient.hpp"` if no longer referenced.
- [ ] Remove any remaining `GithubClient` instantiation code (constructor call, method calls).
- [ ] Remove the old `downloadAndParseManifestVersion_()` method implementation if still present.
- [ ] Remove any `github.releaseId`, `github.releaseAssets`, `github.releaseAssetName[]`, `github.releaseAssetURL[]` references.
- [ ] Remove any remaining floating tag detection code (`strncmp(..., "latest-", 7)`).
- [ ] Clean up any `#include` for `GithubClient/ota_root_cas.h` if the CA cert is now sourced differently or still needed for the manifest fetcher.

## OTAUpdateManager.hpp — Remove Forward Declarations

- [ ] Remove `class GithubClient;` forward declaration if present.
- [ ] Remove the old `downloadAndParseManifestVersion_()` declaration if still present.

## Verify No Other OTA References to GithubClient

- [ ] Grep the codebase for `GithubClient` references outside `lib/GithubClient/` itself. If any remain in OTA-related code, remove them.
- [ ] Grep for `otaGithubRepo` — should be fully replaced by `otaManifestUrl` (done in WO3). Fix any stragglers.

## FrameworkConfig.hpp — Final Cleanup

- [ ] Verify `otaGithubRepo` is fully removed (should be done in WO3, verify here).
- [ ] Verify `otaGithubToken` is renamed to `otaToken` (should be done in WO3, verify here).

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool for both `esp32dev` and `esp32s3nano`. Verify no warnings about unused includes or declarations. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

---

# Execution Status

**Status:** ✅ COMPLETE  
**Commit:** `5f44fb5` — `chore: remove GithubClient from OTA path — dead code cleanup`  
**Branch:** `pr-36-manifest-first-ota-wo5-cleanup`  
**Date:** 2026-06-20  
**Net delta:** 2 files changed, 26 insertions, 234 deletions

## Checklist Results

### OTAUpdateManager.cpp
- [x] Removed `#include "../../lib/GithubClient/GithubClient.hpp"`
- [x] Removed `#if 0` block in `checkForUpdate()` (~53 lines of GithubClient code)
- [x] Removed `#if 0` block with `downloadAndParseManifestVersion_()` (~135 lines)
- [x] All `github.releaseId`, `github.releaseAssets`, `otaGithubRepo`, `otaGithubToken` refs gone
- [x] No floating tag detection code found (`latest-` grep: zero hits)
- [x] Kept `ota_root_cas.h` include — still required by live `downloadRawFile_()` using `OTA_ROOT_CA_BUNDLE`

### OTAUpdateManager.hpp
- [x] Removed `class GithubClient;` forward declaration
- [x] Removed `#if 0` block with `downloadAndParseManifestVersion_()` declaration

### Codebase Verification
- [x] `GithubClient` refs outside `lib/GithubClient/` and `test/`: only `ota_root_cas.h` include (needed)
- [x] `otaGithubRepo`: zero matches
- [x] `otaGithubToken`: zero matches

### FrameworkConfig.hpp
- [x] `otaGithubRepo`: confirmed removed (WO3)
- [x] `otaGithubToken`: confirmed renamed to `otaToken` (WO3)

### Build Verification
- [x] `esp32dev`: SUCCESS — zero warnings
- [x] `esp32s3nano`: SUCCESS — zero warnings
