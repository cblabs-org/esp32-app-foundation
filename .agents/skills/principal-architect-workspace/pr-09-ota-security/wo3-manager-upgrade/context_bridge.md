---
title: "WO3 — otaUpdateManager Upgrade to Full OTA Orchestrator"
distilled_at: "2026-05-22T20:40:00Z"
original_plan_id: "pr-09-ota-security"
artifacts:
  - context_artifacts/adr_manager_architecture.md
  - context_artifacts/graveyard_native_stubs.md
  - context_artifacts/state_file_manifest.md
---

## Executive Summary

WO3 upgraded `otaUpdateManager` from a thin legacy wrapper into the full OTA orchestrator, wiring `OtaDownloadEngine` (WO2) and `MbedTLSSignatureVerifier` (WO1) into a single GitHub-based download→verify→flash pipeline with FreeRTOS task offloading and EventBus progress publishing. All 349 native tests pass with zero regressions.

## Next Steps

1. **Integration PR**: Merge `pr-09-ota-security-wo3-manager-upgrade` into the PR-09 integration branch
2. **Wire in Application**: In the consuming app's `setup()`, call `otaManager.begin(app)` after `app.begin(config)` with `config.otaPublicKey`, `config.otaGithubRepo`, and optionally `config.otaGithubToken` populated
3. **Hardware validation**: Flash to device and trigger `POST /api/ota {"action":"checkGithub"}` to exercise the full pipeline end-to-end
4. **Version comparison**: `checkForUpdate()` currently returns `true` whenever any release is found; a proper semver comparison against `APP_FOUNDATION_VERSION` should be added

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_manager_architecture.md` - Key architectural decisions: state machine design, FreeRTOS task strategy, EventBus payload layout, Application getConfig() addition
- `context_artifacts/graveyard_native_stubs.md` - Failed native build attempts due to include ordering and Application forward-declaration vs. full-type issues
- `context_artifacts/state_file_manifest.md` - Complete file manifest with line counts, key symbols, and test coverage
