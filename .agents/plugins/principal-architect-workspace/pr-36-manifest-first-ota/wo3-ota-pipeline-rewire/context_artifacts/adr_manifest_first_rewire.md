# ADR: Manifest-First OTA Pipeline Rewire

## Decision: Replace GitHub API with Manifest-First Flow

### Context
The existing OTA pipeline in `performUpdateTask()` was tightly coupled to the GitHub API:
1. Called `GithubClient::getLatestReleaseForChannel()` to query releases
2. Searched release assets by filename extension (`.bin`, `.sig`)
3. Used floating tag resolution (`latest-dev` → manifest → version string)
4. Cross-referenced GitHub asset URLs for firmware and signature downloads

This required 2-4 API calls just to determine "no update available" and hard-coded GitHub URL construction on-device.

### Resolution
Replaced the entire GitHub flow with a manifest-first architecture:
1. Single `fetchAndVerifyManifest_()` call fetches + signature-verifies + parses the manifest
2. `findChannel(manifest, _channel)` does a direct O(n) lookup — no risk-floor scanning on-device (deploy script handles that in WO4)
3. `findAssetForBoard(*channel, BOARD_ID)` gets the exact firmware/signature URLs
4. URLs are absolute in the manifest — no URL construction on-device
5. Token renamed from `otaGithubToken` → `otaToken` (host-agnostic)

### Key Design Decisions

1. **No risk-floor scanning on-device** — the deploy script ensures each channel entry already contains the best version for that floor. Device just reads its channel.

2. **`checkForUpdate()` wrapped in `#if 0`** — This standalone method also used `GithubClient`. Since `otaGithubRepo` was removed from `FrameworkConfig`, the GithubClient code can't compile. Wrapped in `#if 0` matching the pattern used for `downloadAndParseManifestVersion_()`. WO5 will clean up both.

3. **Channel-switch implicit downgrade preserved** — When a user changes their channel (e.g. beta → stable), the remote version may be *lower* than local. The pipeline detects this via `_lastCheckedChannel` comparison and allows the "downgrade".

4. **Silent check handling** — Auto-checks (boot/daily) set `_silentCheck = true`. If `fetchAndVerifyManifest_` fails during a silent check, the error state is suppressed and the pipeline returns to Idle quietly.

5. **Signature URL guard** — Added explicit check for empty `signatureUrl` when `otaPublicKey` is configured. This is fail-closed: if the manifest doesn't contain a signature URL but the device expects signed firmware, the update is rejected.

### Files Changed
- `include/FrameworkConfig.hpp` — Removed `otaGithubRepo`, renamed `otaGithubToken` → `otaToken`
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — Full rewire of `performUpdateTask()`, `#if 0` on `checkForUpdate()` GithubClient section, token rename in `fetchAndVerifyManifest_()`
- `src/main.cpp` — Updated config to set `otaManifestUrl` and `otaToken`
