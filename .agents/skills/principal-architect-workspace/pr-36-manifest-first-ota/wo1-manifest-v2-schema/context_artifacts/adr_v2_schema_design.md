# ADR: v2 Manifest Schema Design

## Decision: Multi-channel manifest with absolute URLs

**Status:** Accepted and implemented (WO1)

### Context

The v1 manifest was a flat, single-channel structure with relative filenames (`firmware-esp32dev.bin`). The OTA pipeline had to construct download URLs by cross-referencing GitHub API release asset data, requiring `GithubClient` in the OTA path and coupling the system to GitHub's API.

### Decision

Redesign the manifest to:

1. **Multi-channel in a single file** — The `channels` object is keyed by channel name (`stable`, `dev`, `beta`, `rc`). Each channel contains its own `version`, `securityVersion`, and `assets` array. This eliminates the need for multiple API calls to discover versions across channels.

2. **Absolute URLs** — `OTAAsset::firmwareUrl` and `signatureUrl` are complete download URLs. No URL construction on-device. Makes the system portable to any hosting provider.

3. **Self-referencing `manifestUrl`** — The manifest contains its own URL for future migration (learned URL pattern). The device can update where it fetches manifests from.

4. **`manifestVersion: 1`** — Integer schema version following `IConfigurable::getSchemaVersion()` convention. Enables future schema evolution.

### Hard cut (no v1 backward compat)

- The v1 parser is completely removed. No compatibility layer.
- Rationale: v2 is a fundamentally different structure (flat → nested). A compatibility layer would add complexity for a migration that happens in a single PR. All downstream consumers (OTAUpdateManager) are updated in WO3.

### Structural choices

- `kMaxChannels = 4` — supports dev, beta, rc, stable. Sufficient for foreseeable use.
- `kMaxUrlLen = 256` — GitHub release download URLs are typically ~120 chars. 256 provides comfortable headroom.
- `findChannel()` added as a free function (linear scan, O(n) where n ≤ 4).
- `findAssetForBoard()` signature changed from `const OTAManifest&` to `const OTAChannel&` — callers must first select a channel, then find the board. This is the natural two-step lookup for the new schema.

### Consequences

- `OTAUpdateManager.cpp` will not compile until WO3 migrates its struct access.
- `generate_manifest.py` now requires `--base-url` and `--manifest-url` arguments.
- `deploy_release.py` (WO4) will need to pass these new arguments when invoking the script.
