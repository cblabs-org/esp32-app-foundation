# Current State: Git, Releases, and Device

## Git State

- **Branch:** `pr-36-manifest-first-ota`
- **HEAD:** `bedd7cb` — "fix: resolve highest SemVer tag when multiple tags on HEAD"
- **Tags on HEAD:** `v0.0.12-dev.3` (also `v0.0.12-dev.2` on an earlier commit)
- **Tags pushed to remote:** `v0.0.12-dev.2` (old commit), `v0.0.12-dev.3` NOT pushed yet

## Key commits on this branch (recent)

| Commit | Description |
|--------|-------------|
| `bedd7cb` | fix: resolve highest SemVer tag when multiple tags on HEAD |
| `630147a` | fix: remove dead per-release manifest signing (extra passphrase prompt) |
| `4af0966` | refactor: rename manifest tag to manifest-latest, remove manifest from binary releases |
| `1346baf` | fix: auto-detect --tag from HEAD in deploy_release.py |

## GitHub Releases

| Release | Tag | Assets |
|---------|-----|--------|
| OTA Manifest | `manifest-latest` | `manifest.json`, `manifest.json.sig` |
| v0.0.12-dev.3 | `v0.0.12-dev.3` | 6 firmware files (bins + sigs + hashes, NO manifest) |
| v0.0.12-dev.2 | `v0.0.12-dev.2` | 8 files (bins + sigs + hashes + manifest — legacy from before separation) |
| OTA Manifest (old) | `ota-manifest` | Stale — should be cleaned up |

## Manifest Content (manifest-latest)

```json
{
  "manifestVersion": 1,
  "manifestUrl": ".../releases/download/manifest-latest/manifest.json",
  "channels": {
    "dev": {
      "version": "v0.0.12-dev.3",
      "securityVersion": 1,
      "assets": [
        { "board": "esp32dev", "firmwareUrl": ".../v0.0.12-dev.3/firmware-esp32dev.bin" },
        { "board": "esp32s3nano", "firmwareUrl": ".../v0.0.12-dev.3/firmware-esp32s3nano.bin" }
      ]
    }
  }
}
```

Signature verified OK with `public_key.pem`. Firmware binary signature and SHA256 also verified.

## Device State

- **Board:** esp32dev on `/dev/cu.usbserial-5AA70033491`
- **Running firmware:** `v0.0.12-dev.2` (with `manifest-latest` URL compiled in)
- **IP:** `<device-ip>`
- **OTA status:** Manifest fetch returns HTTP 404 — **repo is private, PAT not configured**
- **`OTA_GITHUB_TOKEN`:** NOT defined in build flags; `main.cpp` has `#ifdef` guard but macro never set

## Verified Working

- ✅ Two-stream releases: binaries in versioned release, manifest in `manifest-latest`
- ✅ Manifest signature verification (openssl)
- ✅ Firmware signature verification (openssl)
- ✅ Firmware SHA256 match
- ✅ Firmware size match
- ✅ Deploy script with tag auto-detection
- ✅ Both build targets (esp32dev, esp32s3nano) compile clean

## Current Blocker

**OTA_GITHUB_TOKEN not set** → device gets 404 on private repo release downloads.
