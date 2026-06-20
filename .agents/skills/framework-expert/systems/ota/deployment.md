# OTA Deployment

Covers the REST API for device-side OTA operations, the `deploy_release.py` release workflow, manifest-first discovery, manifest signing, and error code reference.

## REST API Reference

| Method | Route | Body | Description |
|---|---|---|---|
| `GET` | `/api/ota` | — | Health check: `{ "status": "ready" }` |
| `POST` | `/api/ota` | `{ "action": "checkGithub" }` | Check for update + perform if available |
| `POST` | `/api/ota` | `{ "action": "status" }` | Returns state, version, channel, boardId, autoCheck, lastError |
| `POST` | `/api/ota` | `{ "action": "setChannel", "channel": "<ch>" }` | Change update channel (persisted) |
| `POST` | `/api/ota` | `{ "action": "rollback", "confirm": "ROLLBACK" }` | Rollback to previous firmware |
| `POST` | `/api/ota` | `{ "action": "markValid" }` | Bless the running firmware |

> **Note:** The `rollback` action requires `"confirm": "ROLLBACK"` in the POST body. Without this exact string, the server responds with HTTP 403.

## EventBus Integration

Subscribe to `"ota.progress"` for real-time update status:

```cpp
struct OTAProgressEvent {
    float progress;       // 0.0 to 1.0
    const char* status;   // Human-readable status
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if succeeded (valid only when complete)
};
```

Events are published at each major pipeline step and on every download progress callback.

## Deploy Release Workflow

The `scripts/deploy_release.py` script handles building, signing, and publishing:

> [!WARNING]
> **Agent Execution Rule:** NEVER execute `deploy_release.py` directly using terminal tools. The script requires the private key passphrase, which `openssl` will prompt for interactively. Because background tasks cannot provide interactive standard input, the process will hang indefinitely. **Always** output the exact CLI command in a markdown code block and instruct the user to run it in their own terminal.

```bash
python3 scripts/deploy_release.py \
    --tag v1.0.0 \
    --local --build \
    --private-key private_key.pem
```

The script:
1. Builds firmware for all hardware environments (`pio run -e <board>`)
2. Signs each board's firmware: `openssl dgst -sha256 -sign private_key.pem`
3. Generates `manifest.json` (v2 multi-channel schema) with absolute asset URLs via `scripts/generate_manifest.py`
4. Signs the manifest: `openssl dgst -sha256 -sign private_key.pem manifest.json`
5. Creates the versioned GitHub Release: `gh release create <tag>`
6. Uploads versioned assets: `.bin`, `.bin.sig`, `.bin.sha256`
7. Uploads/updates the `ota-manifest` fixed release with `manifest.json` + `manifest.json.sig`

### The `ota-manifest` Fixed Release

Instead of floating tags (`latest-dev`, `latest-beta`, etc.), the deploy script
uploads the manifest and its detached signature to a single **fixed release tag**
called `ota-manifest`. The device always fetches from the same URL:

```
https://github.com/{repo}/releases/download/ota-manifest/manifest.json
https://github.com/{repo}/releases/download/ota-manifest/manifest.json.sig
```

This approach:
- Eliminates GitHub API pagination and memory exhaustion on the ESP32
- Removes `GithubClient` from the OTA path entirely
- Makes the OTA system portable to any HTTP hosting provider
- Provides a stable URL that never changes between releases

### Manifest Signing

The manifest is signed with the same ECDSA key used for firmware binaries.
The detached signature (`manifest.json.sig`) is verified on-device before
the manifest content is trusted.

**Why:** The `manifestUrl` field inside the manifest is a control-plane
redirect. An attacker who can inject a new URL captures the device's OTA
channel permanently. Manifest signing prevents this.

**Verification rule:** Verify manifest signature FIRST, then parse. If
verification fails:
1. Do not trust any field from the response
2. If this was the learned URL → fall back to the compiled-in URL
3. If this was the compiled-in URL → abort the check (fail-closed)

### Defence-in-Depth Gates

Three independent safety gates, each catching different failure modes:

```
Gate 1: Manifest signature     → prevents tampered URLs, versions, redirects
Gate 2: Firmware signature     → prevents tampered/malicious binaries
Gate 3: Binary version header  → prevents flashing wrong version (esp_app_desc_t check)
```

### `--legacy-floating-tags` Flag

The deploy script supports a `--legacy-floating-tags` flag for backward
compatibility with devices running firmware older than the manifest-first
migration. When set, the script will also create/update the old-style
floating tag releases (`latest-dev`, `latest-beta`, etc.) in addition to
the `ota-manifest` release. This allows mixed fleets to co-exist during
a rollout transition.

### Split-Repo Deployment

When using a separate releases repository, pass `--repo`:

```bash
python3 scripts/deploy_release.py --tag v1.0.0 --local --build \
    --private-key private_key.pem --repo owner/releases-repo
```

## Manifest Schema (v2)

A single manifest covers all channels and all boards with absolute asset URLs:

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
- `manifestVersion: 1` — integer schema version
- `manifestUrl` — self-referencing URL; device stores this as the learned URL for future migration
- `channels` — keyed by channel name, each containing version + per-board assets
- Asset URLs are **absolute** — no GitHub URL construction needed on-device

Board IDs are injected at compile time via `BOARD_ID` in `platformio.ini` `build_flags`. The orchestrator calls `findAssetForBoard(channelEntry, BOARD_ID)` to resolve the correct firmware binary.

## Manifest URL Resilience

The device maintains **two** manifest URLs:

1. **Compiled-in URL** — set at build time in `FrameworkConfig.otaManifestUrl`, never overwritten, survives factory reset
2. **Learned URL** — stored in OTA module config (`learnedManifestUrl` in `/mod/ota.json`), updated from the `manifestUrl` field in a **verified** manifest response only

**Check order:** Try learned URL first → fall back to compiled-in URL. The compiled-in URL is an unkillable safety net.

## Error Codes

| Code | Value | Description |
|---|---|---|
| `None` | 0 | Success |
| `HttpConnectionFailed` | -1 | TCP/TLS connection failed |
| `HttpWrongResponseCode` | -2 | Unexpected HTTP status |
| `HttpFileNotFound` | -3 | 404 — asset not found |
| `HttpForbidden` | -4 | 403 — access denied |
| `ServerNoContentLength` | -5 | Missing Content-Length header |
| `TooManyRedirects` | -6 | Exceeded 5 redirect hops |
| `InsufficientSpace` | -7 | Binary exceeds free sketch space |
| `NoOtaPartition` | -8 | No OTA partition in partition table |
| `BinaryHeaderInvalid` | -9 | First byte is not `0xE9` |
| `SignatureFetchFailed` | -10 | Failed to download `.sig` |
| `SignatureVerifyFailed` | -11 | Cryptographic verification failed |
| `UpdateBeginFailed` | -12 | `Update.begin()` returned false |
| `UpdateWriteFailed` | -13 | Incomplete chunk write |
| `UpdateEndFailed` | -14 | `Update.end()` returned false |
| `StreamReadTimeout` | -15 | Download stream timed out |
| `BinaryVersionMismatch` | -16 | Binary's embedded version doesn't match expected |

## Cross-References

- **Human docs:** [`docs/developer/ota-update-system.md`](../../../../docs/developer/ota-update-system.md)
- **Interface contract:** [`interfaces/ota-verifier.md`](../../interfaces/ota-verifier.md)
- **Security concerns:** [`security/ota-security.md`](../../security/ota-security.md)
- **OTA overview:** [`systems/ota/overview.md`](overview.md)
