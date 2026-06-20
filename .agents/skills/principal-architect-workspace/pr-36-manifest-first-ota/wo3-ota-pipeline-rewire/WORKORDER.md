---
name: pr-36-wo3
description: Handoff instructions for Work Order 3 (OTA Pipeline Rewire)
complexity: architect
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Rewire `OTAUpdateManager::performUpdateTask()` to use the manifest-first flow. Replace the `GithubClient` query + version comparison with the new `fetchAndVerifyManifest_()` method and direct channel lookup. Update `FrameworkConfig` to use `otaManifestUrl`.

# Strict Boundary Box
- DO NOT touch `GithubClient` code (that is WO5 cleanup).
- DO NOT modify `deploy_release.py` or `generate_manifest.py` (that is WO4).
- DO NOT modify `OTADownloadEngine` — it already accepts raw URLs.
- You may leave the old `GithubClient` includes in place for now — WO5 removes them.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`) to compile and verify your code instead of running raw `pio` CLI commands.

# Context
Read the project plan:
`.agents/skills/principal-architect-workspace/pr-36-manifest-first-ota/PROJECT.md`

# Line-Item Instructions

## FrameworkConfig.hpp

- [ ] Replace `const char* otaGithubRepo = nullptr;` with `const char* otaManifestUrl = nullptr;` — add a comment noting this is the compiled-in manifest URL (unkillable fallback).
- [ ] Rename `otaGithubToken` to `otaToken` with a comment. Keep backward compat by adding a deprecated alias if needed, or just rename everywhere.
- [ ] Update the file-level docstring to reflect the new fields.

## OTAUpdateManager::performUpdateTask() — Rewire

- [ ] Replace the `GithubClient` instantiation and `getLatestReleaseForChannel()` call with a call to `fetchAndVerifyManifest_(manifest)`.
- [ ] After successful manifest fetch, implement direct channel lookup:
  1. Read the device's channel from `_channel`.
  2. Call `findChannel(manifest, _channel)` to get the `OTAChannel*` for this device's channel.
  3. If the channel is not found in the manifest, log a warning and return (fail-closed).
  4. **No risk-floor scanning on-device** — the deploy script ensures each channel entry in the manifest already contains the best available version for that floor.
- [ ] Use the selected channel's `version` for the version comparison against `local`.
- [ ] Use `findAssetForBoard()` on the selected channel to get the `OTAAsset` for this board.
- [ ] Use `asset.firmwareUrl` and `asset.signatureUrl` directly for `engine.downloadAndFlash()` and `engine.fetchSignature()` — no more cross-referencing.
- [ ] Pass `_config->otaToken` (renamed from `otaGithubToken`) to the download calls.
- [ ] Set `engine.setExpectedVersion()` using the selected channel's version string.
- [ ] Remove all references to `github.releaseId`, `github.releaseAssets`, `github.releaseAssetName[]`, `github.releaseAssetURL[]`.
- [ ] Update all log messages to reference the new flow (manifest URL, resolved version, channel).

## Guard for otaManifestUrl

- [ ] Replace the existing `otaGithubRepo` null-check guard with a check for `otaManifestUrl`:
  ```cpp
  if (cfg.otaManifestUrl == nullptr) {
      ESP_LOGE(TAG, "No otaManifestUrl configured");
      ...
  }
  ```

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool for both `esp32dev` and `esp32s3nano`. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
