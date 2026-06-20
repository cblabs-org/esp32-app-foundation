# OTA Deployment

Covers the REST API for device-side OTA operations, the `deploy_release.py` release workflow, multi-board manifest format, and error code reference.

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
3. Generates `manifest.json` via `scripts/generate_manifest.py`
4. Creates GitHub Release: `gh release create <tag>`
5. Uploads all assets: `.bin`, `.bin.sig`, `.bin.sha256`, `manifest.json`

### Floating Tags (O(1) OTA Polling)

To prevent GitHub API pagination and memory exhaustion, the OTA engine uses **floating tags** (`latest-dev`, `latest-beta`, `latest-rc`, `latest-stable`) to look up updates in exactly two HTTP requests.

The `deploy_release.py` script automatically force-pushes these floating tags to the latest commit of that channel and creates a GitHub Release attached to it.

**Version Resolution:** Because the GitHub API returns the literal string (e.g. `latest-dev`) in the `tag_name` field of a floating release, the ESP32 cannot parse its semantic version directly from the tag. `GithubClient` detects floating tags via a `"latest-"` prefix check and defers version resolution to `OTAUpdateManager`, which downloads the `manifest.json` asset (~500 bytes) to extract the build-authoritative version string. This costs one extra HTTP request (~3–5s with TLS) but eliminates the fragile cross-component title contract that was previously used.

> [!NOTE]
> **Defence-in-depth:** Even after the manifest version is resolved, the pre-flash binary version validation gate (see OTADownloadEngine) independently verifies that the `esp_app_desc_t.version` field in the downloaded binary matches the expected version before any flash writes occur.

### Split-Repo Deployment

When using a separate releases repository, pass `--repo`:

```bash
python3 scripts/deploy_release.py --tag v1.0.0 --local --build \
    --private-key private_key.pem --repo owner/releases-repo
```

## Multi-Board Manifest Format

Each release includes a `manifest.json` that maps board IDs to firmware assets:

```json
{
  "version": "v3.1.0",
  "channel": "stable",
  "securityVersion": 1,
  "assets": [
    {
      "board": "esp32dev",
      "firmware": "firmware-esp32dev.bin",
      "signature": "firmware-esp32dev.bin.sig",
      "size": 1048576,
      "sha256": "abc123..."
    }
  ]
}
```

Board IDs are injected at compile time via `BOARD_ID` in `platformio.ini` `build_flags`. The orchestrator calls `findAssetForBoard(manifest, BOARD_ID)` to resolve the correct firmware binary.

**Legacy fallback:** If no `manifest.json` is present (pre-manifest releases), the orchestrator falls back to a linear asset scan matching by `.bin` and `.sig` file extensions.

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
