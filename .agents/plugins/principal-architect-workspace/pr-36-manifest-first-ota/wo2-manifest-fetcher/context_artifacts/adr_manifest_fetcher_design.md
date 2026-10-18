# ADR: Manifest Fetcher Design

## Decision: Static `downloadRawFile_()` Helper

Extracted the HTTPS download logic (CA-pinned → insecure fallback, Bearer auth, buffer sizing) into a static free function rather than a private method. Rationale:
- The function has no dependency on `OTAUpdateManager` instance state — it's a pure I/O utility.
- Being static prevents accidental mutation of instance state during download.
- Reused for both `manifest.json` and `manifest.json.sig` downloads.

## Decision: URL Fallback Ordering

The URL attempt list is built as: learned URL first, compiled-in URL second. If the learned URL is identical to the compiled-in URL, only one attempt is made (deduplication via `strcmp`). This prevents wasted TLS handshakes when no URL migration has occurred.

## Decision: Signature Buffer Size

Used a 256-byte stack-allocated `uint8_t sigBuf[256]` for the `.sig` file. ECDSA P-256 signatures are typically 70-72 bytes DER-encoded. 256 bytes provides headroom for larger key sizes without heap allocation.

## Decision: Config Schema v5

Bumped from v4 → v5 for the new `learnedManifestUrl` field. Migration is additive (defaults to empty string). The field is persisted in `/mod/ota.json` alongside existing OTA config.

## Decision: `otaManifestUrl` in FrameworkConfig

Added `otaManifestUrl` alongside the existing `otaGithubRepo` rather than replacing it. WO3 is explicitly responsible for the `otaGithubRepo` → `otaManifestUrl` migration and renaming. Adding it now allows `fetchAndVerifyManifest_()` to compile and reference the compiled-in URL.

## Decision: Learned URL Persistence Timing

The learned URL is updated and persisted immediately after successful manifest verification + parse, before returning to the caller. This ensures the URL migration is durable even if the subsequent OTA download fails. The config save uses `ConfigManager::saveModuleConfig(this)` which writes to `/mod/ota.json`.
