# ADR: Fail-Closed OTA Security Model

## Context

The original OTA pipeline had a **fail-open** vulnerability: when an `otaPublicKey` was configured in `FrameworkConfig` but the GitHub release lacked a `.sig` asset (or the signature fetch returned 0 bytes), the pipeline logged a warning and **continued with the update**. This allowed unsigned firmware to be flashed to a device that was explicitly configured to require signature verification.

## Decision

Convert all three signature-check locations to **fail-closed** semantics:

1. **OTAUpdateManager::performUpdateTask() — `.sig` URL missing**: Changed from `ESP_LOGW` + continue to `ESP_LOGE` + set `OTAState::Error` + `return`. The pipeline now refuses to proceed if no `.sig` asset exists in the release when a public key is configured.

2. **OTAUpdateManager::performUpdateTask() — `sigLen == 0`**: Changed from `ESP_LOGW` + continue to `ESP_LOGE` + set error state + `return`. If the `.sig` file exists but the fetch returns 0 bytes, the update is rejected.

3. **OTADownloadEngine::downloadAndFlash() — Step 10**: The old condition `verifier_ != nullptr && sigBuf != nullptr && sigLen > 0` silently skipped verification when the verifier was set but no signature was available. Now the outer check is `verifier_ != nullptr`, and inside that block, a missing signature triggers `OTAError::SignatureFetchFailed` with `Update.abort()`.

## Rationale

- **Defense in depth**: Three independent barriers ensure that no code path can flash unsigned firmware when verification is configured.
- **Dev mode preserved**: When `otaPublicKey` is `nullptr` (no verifier configured), the pipeline logs a dev-mode warning and proceeds. This is intentional for development builds.
- **Error propagation**: Each abort point sets `lastOtaError_` with a descriptive message that propagates through the EventBus to the display, giving the user clear feedback.

## Consequences

- Any GitHub release missing a `.sig` file will be rejected on devices with `otaPublicKey` configured.
- The CI/CD release pipeline must always include the `.sig` file alongside the `.bin` firmware binary.
