# OTA Developer Guide

> **Audience:** Downstream application developers who consume the ESP32 App
> Foundation framework and need to configure, test, and operate the OTA firmware
> update system.

---

## 1. Quick-Start Checklist

To enable production OTA in your downstream application:

1. **Generate an RSA key pair** (see [§6. Key Management](#6-key-management)):
   ```bash
   bash scripts/generate_keys.sh
   ```
2. **Configure `FrameworkConfig`** in your `main.cpp`:
   ```cpp
   FrameworkConfig config;
   config.otaGithubRepo  = "your-org/your-repo";
   config.otaPublicKey   = YOUR_PUBLIC_KEY_PEM;  // extern const char[]
   config.otaGithubToken = nullptr;              // or a PAT for private repos
   ```
3. **Set the `SECURITY_VERSION`** in `platformio.ini` `build_flags`:
   ```ini
   build_flags =
       -DSECURITY_VERSION=1
   ```
4. **Tag a release** and let CI build the firmware:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```
5. **Sign and deploy** (from your local machine):
   ```bash
   python scripts/deploy_release.py --tag v1.0.0
   ```

---

## 2. Architecture Overview

The OTA subsystem follows a layered architecture:

```
┌─────────────────────────────────────────────────┐
│                 Application Hub                  │
│        (Step 17b: markAppValid blessing)          │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│            OTAUpdateManager (module)             │
│  State machine · FreeRTOS task · REST API ·      │
│  IConfigurable (channel, autoCheck) · EventBus   │
└──────┬────────┬────────┬────────┬───────────────┘
       │        │        │        │
┌──────▼──┐ ┌──▼────┐ ┌▼──────┐ ┌▼───────────────┐
│ SemVer  │ │OTAMani│ │Config │ │OTADownloadEngine│
│ (lib)   │ │fest   │ │Manif. │ │ (lib)           │
│         │ │(lib)  │ │(lib)  │ │                 │
└─────────┘ └───────┘ └───────┘ └──────┬──────────┘
                                       │
                               ┌───────▼──────────┐
                               │ IOTAVerifier      │
                               │ (interface)       │
                               │ MbedTLSSignature  │
                               │ Verifier (impl)   │
                               └──────────────────┘
```

**Key design principle:** The `OTAUpdateManager` module depends on `lib/`
libraries, but the libraries have no knowledge of the module (Hollywood
Principle). The download engine accepts a `WiFiClient&` from the caller —
it never creates its own network client.

### Component Responsibilities

| Component | Location | Role |
|-----------|----------|------|
| `OTAUpdateManager` | `modules/otaUpdateManager/` | Orchestrator — state machine, FreeRTOS task, REST API, EventBus |
| `OTADownloadEngine` | `lib/OTADownloadEngine/` | Binary download, flash write, signature integration |
| `IOTAVerifier` | `include/interfaces/` | Verification interface (three-step: begin/update/verify) |
| `MbedTLSSignatureVerifier` | `lib/OTADownloadEngine/` | RSA+SHA256 verification via mbedtls |
| `SemVer` | `lib/SemVer/` | Firmware version parsing, packed 32-bit comparison |
| `OTAManifest` | `lib/OTAManifest/` | Multi-architecture manifest parser |
| `ConfigManifest` | `lib/ConfigManifest/` | Three-Tier config lifecycle (Active/Previous/Golden) |
| `GithubClient` | `lib/GithubClient/` | GitHub API client for release queries |

---

## 3. Update Lifecycle

### 3.1 State Machine

```
Idle ──→ Checking ──→ Idle              (no update found)
Idle ──→ Downloading ──→ Verifying ──→ Complete ──→ (reboot)
Any  ──→ Error                          (on failure)
```

| State | Description |
|-------|-------------|
| `Idle` | Ready to accept a check or update |
| `Checking` | Querying GitHub for latest release metadata |
| `Downloading` | FreeRTOS task running the download pipeline |
| `Verifying` | Signature verification in progress |
| `Complete` | Update finalised — waiting for reboot |
| `Error` | An error occurred (query via REST API or `getLastOtaError()`) |

### 3.2 Pipeline Steps

When `performUpdate()` is called, a FreeRTOS task (8KB stack, priority 1)
executes the following pipeline:

1. **Guard checks** — firmware updates enabled? GitHub repo configured?
2. **Query GitHub** — `GithubClient::getLatestReleaseForChannel(_channel)`
3. **Download manifest** — parse `manifest.json` from release assets (or fall
   back to legacy asset scan for pre-manifest releases)
4. **Board matching** — `findAssetForBoard(manifest, BOARD_ID)` resolves the
   correct firmware binary for this board
5. **Fail-closed check** — reject missing/empty `.sig` when public key is
   configured (see [§5. Signature Verification](#5-signature-verification))
6. **Config snapshot** — `snapshotActiveToPrevious()` (see
   [§9. Config Lifecycle](#9-config-lifecycle))
7. **Init verifier** — load the RSA public key into `MbedTLSSignatureVerifier`
8. **Allocate HTTPS client** — `WiFiClientSecure` on the heap (per §15.3)
9. **Create download engine** — configure `OTADownloadEngine`
10. **Fetch `.sig`** — download the detached signature file
11. **Download and flash** — stream firmware in 1024-byte chunks, feeding the
    verifier and the task watchdog
12. **Finalise and reboot** — 2-second delay for EventBus event propagation,
    then `esp_restart()`

### 3.3 Post-Reboot Self-Test

After reboot, the new firmware enters **pending verification** state. The
`Application` hub runs the full boot sequence (Steps 1–17), and only at
**Step 17b** calls `markAppValid()`. This blessing barrier covers:

- Display hardware bootstrap
- EventBus, LittleFS mount, FrameworkConfig
- DeviceCrypto, KeyManager
- ConfigManager (register + beginAll)
- WiFi, Time, Data, OTA, Web, MCP
- Display integration, carousel board registration
- Registry lock (`_compactAndLock`)

If any step crashes or hangs before `markAppValid()`, the ESP-IDF bootloader
automatically reverts to the previous firmware partition on reboot.

---

## 4. Release Channels

### 4.1 Available Channels

| Channel | Tag Convention | `getLatestReleaseForChannel()` Behaviour |
|---------|----------------|------------------------------------------|
| `stable` | `v3.1.0` (no suffix) | Delegates to `getLatestRelease()` (GitHub "latest" endpoint) |
| `beta` | `v3.2.0-beta.1` | Queries all releases, finds first pre-release with `beta` in tag |
| `dev` | `v3.2.0-dev.3` | Queries all releases, finds first pre-release with `dev` in tag |

### 4.2 Channel Configuration

The active channel is persisted as an `IConfigurable` field in `/mod/ota.json`:

```json
{
  "schema": 2,
  "channel": "stable",
  "autoCheck": true,
  "lastCheckedChannel": "stable"
}
```

**Change channel via REST API:**
```bash
curl -X POST http://<device-ip>/api/ota \
  -H "Content-Type: application/json" \
  -d '{"action": "setChannel", "channel": "beta"}'
```

**Change channel via web portal:** Use the OTA tab → Channel dropdown.

### 4.3 Implicit Downgrade

When the channel changes (e.g., `beta` → `stable`), `checkForUpdate()` detects
that `_channel != _lastCheckedChannel` and signals "update available" even if the
remote version is numerically lower than the running firmware. This allows
seamless channel switching without manual intervention.

### 4.4 SemVer Comparison

The `SemVer` library provides zero-allocation version comparison:

```cpp
FirmwareVersion running = parseVersionTag("v3.1.0");
FirmwareVersion remote  = parseVersionTag("v3.2.0-beta.1");

if (packVersion(remote) > packVersion(running)) {
    // Update available
}
```

**Bit layout:** `MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(4) | PRE_NUM(4)`

**Channel ordering:** `DEV(0) < BETA(2) < RC(3) < STABLE(15)` — a stable
release always sorts higher than any pre-release of the same major.minor.patch.

---

## 5. Signature Verification

### 5.1 Fail-Closed Mode (Production)

When `FrameworkConfig.otaPublicKey` is configured (non-null):

- **No `.sig` asset in release** → update aborted immediately
- **Empty `.sig` file** → update aborted immediately
- **Signature mismatch** → `OTAError::SignatureVerifyFailed`

There is **no code path** that accepts unsigned firmware when a public key is
present.

### 5.2 Degraded Mode (Development)

When `otaPublicKey` is `nullptr`:

- Signature verification is skipped entirely
- A warning is logged at `ESP_LOGW` level
- This mode is for development only — **never deploy to production without a key**

### 5.3 Verification Pipeline

```
begin(publicKeyPem)    →  Load RSA key, init SHA-256 context
update(chunk, length)  →  Feed each downloaded chunk into SHA-256
verify(signature, len) →  Compare final digest against .sig file
```

The `MbedTLSSignatureVerifier` uses `mbedtls_pk_context` for key parsing and
`mbedtls_md_context_t` for digest accumulation. RAII lifecycle ensures
`mbedtls_pk_free()` + `mbedtls_sha256_free()` are called even on error paths.

---

## 6. Key Management

### 6.1 Generate Keys

```bash
# Generate a new RSA key pair (2048-bit)
bash scripts/generate_keys.sh
```

This creates:
- `keys/private_key.pem` — **keep secret, never commit to git**
- `keys/public_key.pem` — embed in your firmware via a `const char[]`

### 6.2 Embed the Public Key

In your downstream app, create a header with the PEM-encoded public key:

```cpp
// ota_public_key.h
#pragma once

const char OTA_PUBLIC_KEY[] = R"(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...
-----END PUBLIC KEY-----
)";
```

Then reference it in your `FrameworkConfig`:

```cpp
#include "ota_public_key.h"

FrameworkConfig config;
config.otaPublicKey = OTA_PUBLIC_KEY;
```

### 6.3 Key Rotation

To rotate keys:

1. Generate a new key pair: `bash scripts/generate_keys.sh`
2. Release a **transitional firmware** signed with the **old** key that embeds
   the **new** public key
3. Once all devices have updated, future releases are signed with the new
   private key

> **Warning:** If you lose the private key, devices running firmware with the
> corresponding public key can never receive signed OTA updates. Always back up
> your private key securely.

---

## 7. Private Repository & PAT Configuration

If your OTA releases are hosted in a **private** GitHub repository, devices need a
Personal Access Token (PAT) to authenticate API requests. This section covers
token creation, repository layout options, and the `deploy_release.py --repo` flag.

### 7.1 PAT Creation Walkthrough

1. Go to **GitHub → Settings → Developer settings → Personal access tokens → Fine-grained tokens**
2. Click **Generate new token**
3. Set **Token name** (e.g., `ota_update_token`)
4. Set **Expiration** to **No expiration**
5. Under **Repository access**, select **Only select repositories** → choose your OTA repo
6. Under **Repository permissions**, set **Contents** to **Read-only**
7. Leave all other permissions at **No access**
8. Click **Generate token** and copy the token immediately

> **Warning:** Treat the PAT like a private key. Never commit it to git. Store it
> in a separate `otaSecrets.hpp` header that is listed in `.gitignore`.

### 7.2 Repository Configuration

| Setup | `config.otaGithubRepo` | `otaSecrets.hpp` | `deploy_release.py` |
|---|---|---|---|
| Single private repo | `"owner/repo"` | Include PAT | (default) |
| Split releases repo | `"owner/releases-repo"` | Include PAT | `--repo owner/releases-repo` |
| Public repo | `"owner/public-repo"` | Omit (`nullptr`) | (default) |

**Split releases repo** is recommended for blast-radius reduction: your source
code repo and your firmware releases repo are separate. Devices only need
read access to the releases repo.

### 7.3 Deploy Script `--repo` Usage

When using a split releases repo, pass `--repo` to `deploy_release.py` so the
GitHub Release is created in the correct repository:

```bash
python scripts/deploy_release.py --tag v1.0.0 --private-key private_key.pem --local --repo owner/releases-repo
```

The `--repo` flag is passed through to `gh release create` and `gh release upload`.
If omitted, the script defaults to the current repository's `origin` remote.

---

## 8. Multi-Architecture Support

### 8.1 Manifest Schema

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
    },
    {
      "board": "esp32s3nano",
      "firmware": "firmware-esp32s3nano.bin",
      "signature": "firmware-esp32s3nano.bin.sig",
      "size": 1097152,
      "sha256": "def456..."
    }
  ]
}
```

### 8.2 Board ID Injection

Each PlatformIO environment defines a `BOARD_ID` macro:

```ini
[env:esp32dev]
build_flags =
    -DBOARD_ID=\"esp32dev\"
    -DSECURITY_VERSION=1

[env:esp32s3nano]
build_flags =
    -DBOARD_ID=\"esp32s3nano\"
    -DSECURITY_VERSION=1
```

The orchestrator calls `findAssetForBoard(manifest, BOARD_ID)` to resolve the
correct firmware binary for the running board.

### 8.3 Legacy Fallback

If no `manifest.json` is present in a release (pre-manifest releases), the
orchestrator falls back to a linear scan of release assets, matching by `.bin`
and `.sig` file extensions. This ensures backward compatibility with older
releases that don't include a manifest.

---

## 9. Config Lifecycle

### 9.1 Three-Tier Model

The `ConfigManifest` library manages configuration versioning across OTA updates:

| Tier | Description | Snapshot Trigger |
|------|-------------|-----------------|
| **Active** | Currently running config schema version per module | Updated by `ConfigManager` during `beginAll()` |
| **Previous** | Config state before the last OTA download | `snapshotActiveToPrevious()` — called before OTA download |
| **Golden** | Last-known-good config (firmware blessed) | `snapshotActiveToGolden()` — called from `markAppValid()` |

### 9.2 OTA Integration Points

```
  OTA Download Triggered
         │
         ▼
  snapshotActiveToPrevious() + save()
         │
         ▼
  Download → Flash → Reboot
         │
         ▼
  Boot Sequence (Steps 1-17)
         │
         ▼
  Step 17b: markAppValid()
         │
         ▼
  snapshotActiveToGolden() + save()
```

### 9.3 Config Persistence

Config manifest is persisted at `/config_manifest.json` on LittleFS. Per-module
configs are at `/mod/<namespace>.json`.

---

## 10. Anti-Rollback

### 10.1 Security Version Counter

The application-layer anti-rollback mechanism uses an NVS counter to prevent
downgrade past security-critical firmware boundaries.

**Components:**
- `SECURITY_VERSION` macro — defined in `platformio.ini` `build_flags`
- NVS storage — `Preferences` namespace `"ota_security"`, key `"sec_ver"`
- Manifest field — `securityVersion` in `manifest.json`

### 10.2 When to Increment

Increment `SECURITY_VERSION` when a release contains a security fix that must
not be rolled back past. For example:

```ini
# Before security fix
build_flags = -DSECURITY_VERSION=1

# After security fix
build_flags = -DSECURITY_VERSION=2
```

> **Warning:** Incrementing `SECURITY_VERSION` is a **one-way operation**. Once
> devices update past this barrier, they cannot be rolled back to any firmware
> with a lower security version — even via the web portal rollback button.

### 10.3 Check Logic

`checkSecurityVersion()` (called from `markAppValid()`):
- `SECURITY_VERSION > stored` → NVS updated to new value
- `SECURITY_VERSION < stored` → firmware below security floor (prevented by
  orchestrator checking manifest's `securityVersion` before download)

---

## 11. CI/CD Pipeline

### 11.1 Stage 1: GitHub Actions (CI)

The `.github/workflows/build.yml` workflow:

1. **Trigger:** Tag push matching `v*`
2. **Matrix build:** `esp32dev` + `esp32s3nano`
3. **Per-board steps:**
   - `pio run -e <board>` — compile firmware
   - Rename output to `firmware-<board>.bin`
   - Generate `firmware-<board>.bin.sha256`
   - Upload as GitHub Actions artifact

### 11.2 Stage 2: Local Release

Run `scripts/deploy_release.py` on your local machine:

```bash
python scripts/deploy_release.py --tag v1.0.0
```

This script:
1. Downloads CI artifacts: `gh run download <run-id>`
2. Signs each board's firmware: `openssl dgst -sha256 -sign private_key.pem`
3. Generates `manifest.json`: `python scripts/generate_manifest.py`
4. Creates GitHub Release: `gh release create <tag>`
5. Uploads all assets: `.bin`, `.bin.sig`, `.bin.sha256`, `manifest.json`

### 11.3 Why Sign Locally?

The private key must **never** be stored in GitHub Secrets or any remote system.
The CI pipeline produces unsigned artifacts; the human operator signs them with
the locally-held private key before publishing the release.

---

## 12. REST API Reference

| Method | Route | Body | Description |
|--------|-------|------|-------------|
| `GET` | `/api/ota` | — | Health check: `{ "status": "ready" }` |
| `POST` | `/api/ota` | `{ "action": "checkGithub" }` | Check for update + perform if available |
| `POST` | `/api/ota` | `{ "action": "status" }` | Returns state, version, channel, boardId, autoCheck, lastError |
| `POST` | `/api/ota` | `{ "action": "setChannel", "channel": "<ch>" }` | Change update channel (persisted) |
| `POST` | `/api/ota` | `{ "action": "rollback", "confirm": "ROLLBACK" }` | Rollback to previous firmware |
| `POST` | `/api/ota` | `{ "action": "markValid" }` | Bless the running firmware |

### Examples

**Check for updates:**
```bash
curl -X POST http://<device-ip>/api/ota \
  -H "Content-Type: application/json" \
  -d '{"action": "checkGithub"}'
```

**Query status:**
```bash
curl -X POST http://<device-ip>/api/ota \
  -H "Content-Type: application/json" \
  -d '{"action": "status"}'
```

**Rollback (requires confirmation):**
```bash
curl -X POST http://<device-ip>/api/ota \
  -H "Content-Type: application/json" \
  -d '{"action": "rollback", "confirm": "ROLLBACK"}'
```

> **Note:** The `rollback` action requires `"confirm": "ROLLBACK"` in the POST
> body. Without this exact string, the server responds with HTTP 403.

---

## 13. EventBus Integration

### 13.1 Progress Events

Subscribe to `"ota.progress"` for real-time update status:

```cpp
app.getEventBus().subscribe("ota.progress", [](const void* payload) {
    auto* event = static_cast<const OTAProgressEvent*>(payload);
    Serial.printf("OTA: %.0f%% — %s\n", event->progress * 100, event->status);
    if (event->complete) {
        Serial.printf("OTA %s\n", event->success ? "succeeded" : "failed");
    }
});
```

### 13.2 Event Payload

```cpp
struct OTAProgressEvent {
    float progress;       // 0.0 to 1.0
    const char* status;   // Human-readable status
    bool complete;        // True when OTA cycle is finished
    bool success;         // True if succeeded (valid only when complete)
};
```

Events are published:
- At each major pipeline step (checking, downloading, verifying, complete, error)
- On every progress callback from the download engine (per-chunk)
- With `complete=true` on both success and failure

---

## 14. Safety Mechanisms

### 14.1 Auto-Rollback (Bootloader)

**Prerequisite:** `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in
`sdkconfig.defaults`.

If the new firmware crashes or hangs during boot before `markAppValid()` is
called, the ESP-IDF bootloader automatically reverts to the previous partition.

### 14.2 Task Watchdog (TWDT)

The OTA FreeRTOS task subscribes to the Task Watchdog Timer:

```cpp
esp_task_wdt_add(NULL);   // Subscribe at task start
// ... download loop ...
esp_task_wdt_delete(NULL); // Unsubscribe before exit
```

With `CONFIG_ESP_TASK_WDT_PANIC=y`, a TWDT timeout triggers:
**Hung download → Watchdog timeout → Kernel panic → Reboot → Auto-rollback**

No human intervention required.

### 14.3 Closed Safety Loop

The combination of auto-rollback and task watchdog creates a closed safety loop:

```
Bad firmware flashed
        │
        ▼
  Device reboots
        │
        ├── Boot crash before markAppValid()
        │           │
        │           ▼
        │   Bootloader auto-rollback
        │
        └── Download hangs (network issue)
                    │
                    ▼
            TWDT timeout → panic → reboot → auto-rollback
```

---

## 15. FrameworkConfig Reference

`FrameworkConfig` carries only the compile-time secrets the framework cannot
know. The master update kill-switch is now a persisted OTA module setting
(`updatesEnabled`, below); `otaQuietHour` was removed — it was never implemented,
and daily scheduling is handled by `checkHour`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `otaPublicKey` | `const char*` | `nullptr` | PEM-encoded RSA public key |
| `otaGithubRepo` | `const char*` | `nullptr` | Repository in `"owner/repo"` format |
| `otaGithubToken` | `const char*` | `nullptr` | GitHub PAT for private repos |

### IConfigurable Fields (persisted in `/mod/ota.json`)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | `char[16]` | `"stable"` | Active release channel |
| `autoCheck` | `bool` | `true` | Auto-check on boot |
| `updatesEnabled` | `bool` | `true` | Master kill-switch for applying firmware updates |
| `checkHour` | `uint8_t` | `3` | Hour (0–23) for the daily scheduled check |
| `lastCheckedChannel` | `char[16]` | `""` | Implicit downgrade detection |

---

## 16. Troubleshooting

### Common Issues

| Symptom | Cause | Resolution |
|---------|-------|------------|
| "Missing .sig file — update rejected" | Release is missing signature file | Run `deploy_release.py` to sign and re-upload |
| `OTAError::SignatureFetchFailed` | `.sig` file download failed (404 or network) | Verify the `.sig` asset exists in the GitHub release |
| `OTAError::SignatureVerifyFailed` | Binary was modified or wrong key used | Re-sign with the correct private key |
| `OTAError::InsufficientSpace` | Binary too large for OTA partition | Reduce firmware size or increase partition size |
| `OTAError::BinaryHeaderInvalid` | Downloaded file is not ESP32 firmware | Check release assets — ensure `.bin` is a valid firmware |
| `OTAError::TooManyRedirects` | GitHub CDN redirect loop | Retry; check network connectivity |
| Device rolls back after update | New firmware crashes before `markAppValid()` | Debug the crash; check serial logs from the new firmware |
| Channel change doesn't trigger update | `lastCheckedChannel` already matches | Check `/mod/ota.json` on the device filesystem |

### Error Code Reference

| Code | Value | Description |
|------|-------|-------------|
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

---

## 17. Security Checklist

Before deploying to production, verify:

- [ ] `FrameworkConfig.otaPublicKey` is set to your PEM-encoded RSA public key
- [ ] Private key is stored securely and **never** committed to git
- [ ] `SECURITY_VERSION` is set in all PlatformIO environments
- [ ] `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in `sdkconfig.defaults`
- [ ] `CONFIG_ESP_TASK_WDT_PANIC=y` in `sdkconfig.defaults`
- [ ] `manifest.json` is generated and uploaded with each release
- [ ] All `.bin` assets have corresponding `.bin.sig` files
- [ ] The `deploy_release.py` script is used for signing (not CI)
