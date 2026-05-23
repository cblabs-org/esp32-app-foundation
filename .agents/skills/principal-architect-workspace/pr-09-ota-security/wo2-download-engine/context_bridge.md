---
title: "WO2 — OTA Download Engine Distillation"
distilled_at: "2026-05-22T20:27:00Z"
original_plan_id: "pr-09-ota-security"
artifacts:
  - context_artifacts/adr_download_engine_design.md
---

## Executive Summary

WO2 delivered the `OtaDownloadEngine` — a GitHub-aware firmware download utility in `lib/otaDownloadEngine/` that handles redirect-following HTTP fetches, `.sig` signature file retrieval, ESP32 binary header validation, partition space checks, chunked streaming flash writes through `Update`, and `IOtaVerifier`-based cryptographic signature verification. Commit `0b6cd07` on branch `pr-09-ota-security-wo2-download-engine`. All 12 tests pass (8 WO2 + 4 WO1 regression).

## Next Steps

1. **WO3 — Manager Upgrade:** Wire `OtaDownloadEngine` + `MbedTLSSignatureVerifier` into `otaUpdateManager`. Add `checkForUpdate()` (calls `GithubClient`, compares version tags) and `performUpdate()` (orchestrates full download→verify→flash). Add `FrameworkConfig.otaPublicKey` field.
2. **WO4 — Spec Update:** Update `docs/design_specification.md` §13.3 with the full cryptographic verification pipeline documentation.
3. **Merge WO2 into feature branch:** `git checkout pr-09-ota-security && git merge pr-09-ota-security-wo2-download-engine`

## Deliverables

| File | Purpose |
|------|---------|
| `lib/otaDownloadEngine/OtaError.hpp` | `OtaError` enum class (16 error codes) |
| `lib/otaDownloadEngine/OtaError.cpp` | `otaErrorToString()` switch mapping |
| `lib/otaDownloadEngine/OtaDownloadEngine.hpp` | Engine class declaration + native stubs |
| `lib/otaDownloadEngine/OtaDownloadEngine.cpp` | Full hardware impl (303 lines) + native stubs |
| `test/test_unit/test_ota_download_engine/test_ota_download_engine.cpp` | 8 GoogleTest cases with MockVerifier |

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_download_engine_design.md` - Architecture decisions for the download engine's API surface, verifier integration pattern, and error handling strategy
