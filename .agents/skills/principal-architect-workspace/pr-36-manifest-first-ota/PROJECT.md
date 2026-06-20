# PR-36: Manifest-First OTA Architecture

**PR Branch:** `pr-36-manifest-first-ota`
**Base:** `main`
**Status:** 📋 **PLANNED — awaiting approval**

## Design Decisions

| Decision | Resolution |
|---|---|
| **v1/v2 backward compat** | Hard cut to v2. No v1 parser support. |
| **Risk-floor logic** | Moves to the deploy script. The script scans all releases and ensures each channel entry in the manifest contains the best available version for that floor. Device just reads its channel — no scanning. |
| **Manifest signing** | Required. Detached ECDSA signature (same key as firmware). Manifest sig is Gate 1 of defence-in-depth. |

## Objective

Replace the GitHub-API-coupled OTA version detection system with a **manifest-first architecture** where a single signed `manifest.json` is the sole entry point for all OTA operations. The manifest covers all channels, contains absolute download URLs, and is self-describing (including its own URL for future migration). This eliminates the `GithubClient` from the OTA path, removes floating tag dependence, and makes the OTA system fully portable to any hosting provider.

### Current Architecture (3 requests for "no update")
```
GithubClient → GET api.github.com/.../releases/latest          (2-4 KB, GitHub API)
             → GET api.github.com/.../releases/tags/latest-dev  (2-4 KB, GitHub API)
OTAManager   → GET manifest.json (via API asset URL)            (~500 bytes)
             → cross-reference filenames against API asset URLs
             → GET binary → GET signature
```

### Target Architecture (2 requests for "no update")
```
OTAManager   → GET manifest.json                                (~3 KB, any host)
             → GET manifest.json.sig                             (~256 bytes)
             → verify signature → parse → select channel → compare version → stop
             → GET {firmware URL from manifest}                  (direct, any host)
             → GET {firmware.sig URL from manifest}              (direct, any host)
             → verify firmware signature → validate binary header → flash
```

## Design

### New Manifest Schema (v2)

```json
{
  "manifestVersion": 1,
  "manifestUrl": "https://github.com/.../releases/download/ota-manifest/manifest.json",
  "channels": {
    "stable": {
      "version": "v1.0.0",
      "securityVersion": 1,
      "assets": [
        {
          "board": "esp32dev",
          "firmware": "https://github.com/.../releases/download/v1.0.0/firmware-esp32dev.bin",
          "signature": "https://github.com/.../releases/download/v1.0.0/firmware-esp32dev.bin.sig",
          "size": 1341651,
          "sha256": "abc123..."
        }
      ]
    },
    "dev": {
      "version": "v1.1.0-dev.3",
      "securityVersion": 1,
      "assets": [...]
    }
  }
}
```

**Key properties:**
- `manifestVersion: 1` — integer schema version, consistent with `IConfigurable::getSchemaVersion()` convention
- `manifestUrl` — self-referencing URL; device stores this for future migration
- `channels` — keyed by channel name, each containing version + per-board assets
- Asset URLs are **absolute** — no GitHub URL construction needed on-device
- Filenames (`firmware`, `signature`) replaced by full URLs

### Manifest Signing

The manifest is signed with the same ECDSA key used for firmware binaries:

```
manifest.json         (~3 KB)   — the manifest itself
manifest.json.sig     (~256 B)  — detached ECDSA signature
```

**Threat model — why signing is required:**

| Attack | Without signing | With signing |
|---|---|---|
| Point firmware URLs to malicious binary | ✅ Caught by firmware sig | ✅ Caught by manifest sig |
| Redirect `manifestUrl` to attacker server | ❌ **Persistent device capture** | ✅ Caught by manifest sig |
| Suppress updates (version = current) | ❌ Silent denial of service | ✅ Caught by manifest sig |
| Downgrade `securityVersion` | ❌ Weakens anti-rollback | ✅ Caught by manifest sig |

The `manifestUrl` field is a control-plane redirect — an attacker who can inject a new URL captures the device's OTA channel permanently. Manifest signing is the gate that prevents this.

**Verification rule:** Verify manifest signature FIRST, then parse. If verification fails:
1. Do not trust any field from the response
2. If this was the learned URL → fall back to the compiled-in URL
3. If this was the compiled-in URL → abort the check (fail-closed)

**Deploy cost:** One extra `openssl dgst` call in `deploy_release.py` — trivial.

### Manifest URL Resilience

The device maintains TWO manifest URLs:
1. **Compiled-in URL** — set at build time in `FrameworkConfig`, never overwritten, survives factory reset
2. **Learned URL** — stored in OTA module config (persisted via `IConfigurable`), updated from the `manifestUrl` field in a **verified** manifest response only

**Check order:** Try learned URL first → fall back to compiled-in URL. The compiled-in URL is an unkillable safety net.

### Manifest Hosting (GitHub)

For GitHub-hosted releases, the manifest lives at a fixed release tag:
```
https://github.com/{repo}/releases/download/ota-manifest/manifest.json
https://github.com/{repo}/releases/download/ota-manifest/manifest.json.sig
```

`deploy_release.py` creates/updates this fixed release on every deploy, uploading the new manifest + signature. The versioned release and its assets remain unchanged — the manifest just points to them with absolute URLs.

### Impact on FrameworkConfig

```cpp
struct FrameworkConfig {
    const char* otaPublicKey    = nullptr;  // Unchanged
    const char* otaManifestUrl  = nullptr;  // NEW — replaces otaGithubRepo
    const char* otaToken        = nullptr;  // Renamed from otaGithubToken (backward compat alias)
};
```

### Sizing

| Scenario | Channels | Boards | Est. Manifest Size |
|---|---|---|---|
| Current project | 4 (dev/beta/rc/stable) | 2 | ~2.5 KB |
| Future growth | 4 | 4 | ~4 KB |

Buffer: bump `manifestBuf` from 2048 → 4096 bytes (static, not stack — already the pattern used today).

### Defence-in-Depth Summary

Three independent safety gates, each catching different failure modes:

```
Gate 1: Manifest signature     → prevents tampered URLs, versions, redirects
Gate 2: Firmware signature     → prevents tampered/malicious binaries
Gate 3: Binary version header  → prevents flashing wrong version (esp_app_desc_t check)
```

### What Gets Removed

- `GithubClient` usage in OTA path (class retained for non-OTA uses if any)
- Floating tag creation in `deploy_release.py` (replaced by fixed `ota-manifest` release)
- Title hack (already removed this session)
- GitHub API JSON parsing for release metadata
- Cross-referencing filenames against API asset URLs
- `downloadAndParseManifestVersion_()` helper (replaced by direct manifest fetch)

## Work Order Ledger

- **[x] [wo1-manifest-v2-schema](wo1-manifest-v2-schema/WORKORDER.md):** New manifest v2 schema — merged `b1757e9`
- **[x] [wo2-manifest-fetcher](wo2-manifest-fetcher/WORKORDER.md):** Manifest fetcher + sig verification — merged `928466e`
- **[x] [wo3-ota-pipeline-rewire](wo3-ota-pipeline-rewire/WORKORDER.md):** OTA pipeline rewire — merged `337effe`
- **[x] [wo4-deploy-script](wo4-deploy-script/WORKORDER.md):** Deploy script v2 — merged `9c265ff`
- **[x] [wo5-cleanup](wo5-cleanup/WORKORDER.md):** Dead code cleanup — merged `5f44fb5` (−234 lines)
- **[ ] [wo6-docs-and-skill](wo6-docs-and-skill/WORKORDER.md):** Update `ota-update-system.md`, `deployment.md`, and `/framework-expert` skill systems

## Serialisation

WOs are serialised: WO1 → WO2 → WO3 → WO4 → WO5 → WO6. Each builds on the prior.

## Files Touched (per WO)

| WO | Files |
|----|-------|
| WO1 | `lib/OTAManifest/OTAManifest.hpp`, `lib/OTAManifest/OTAManifest.cpp`, `scripts/generate_manifest.py` |
| WO2 | `modules/otaUpdateManager/OTAUpdateManager.hpp`, `modules/otaUpdateManager/OTAUpdateManager.cpp` |
| WO3 | `modules/otaUpdateManager/OTAUpdateManager.cpp`, `include/FrameworkConfig.hpp` |
| WO4 | `scripts/deploy_release.py`, `scripts/generate_manifest.py` |
| WO5 | `modules/otaUpdateManager/OTAUpdateManager.cpp`, `lib/GithubClient/GithubClient.cpp` |
| WO6 | `docs/developer/ota-update-system.md`, `.agents/skills/framework-expert/systems/ota/deployment.md`, `.agents/skills/framework-expert/SKILL.md` |
