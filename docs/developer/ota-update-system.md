# OTA Update System

> ESP32 App Foundation (c) 2026 CB Labs
>
> The complete OTA update lifecycle — state machine, release channels, signature verification, config snapshots, error codes, and deployment procedures.

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
   config.otaManifestUrl = "https://github.com/your-org/your-repo/releases/download/manifest-latest/manifest.json";
   config.otaPublicKey   = YOUR_PUBLIC_KEY_PEM;  // extern const char[]
   config.otaToken       = nullptr;              // or a PAT/bearer token for private repos
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
| `OTAUpdateManager` | `modules/OTAUpdateManager/` | Orchestrator — state machine, FreeRTOS task, REST API, EventBus |
| `OTADownloadEngine` | `lib/OTADownloadEngine/` | Binary download, flash write, signature integration |
| `IOTAVerifier` | `include/interfaces/` | Verification interface (three-step: begin/update/verify) |
| `MbedTLSSignatureVerifier` | `lib/OTADownloadEngine/` | RSA+SHA256 verification via mbedtls |
| `SemVer` | `lib/SemVer/` | Firmware version parsing, packed 32-bit comparison |
| `OTAManifest` | `lib/OTAManifest/` | Multi-channel manifest parser (v2 schema) |
| `ConfigManifest` | `lib/ConfigManifest/` | Three-Tier config lifecycle (Active/Previous/Golden) |

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
| `Checking` | Fetching and verifying the signed manifest |
| `Downloading` | FreeRTOS task running the download pipeline |
| `Verifying` | Signature verification in progress |
| `Complete` | Update finalised — waiting for reboot |
| `Error` | An error occurred (query via REST API or `getLastOtaError()`) |

### 3.2 Pipeline Steps

When `performUpdate()` is called, a FreeRTOS task (16KB stack (OTA_TASK_STACK_BYTES = 16384), priority 5)
executes the following pipeline:

1. **Guard checks** — firmware updates enabled? `otaManifestUrl` configured?
2. **Fetch manifest** — download `manifest.json` from the learned URL (or
   fall back to the compiled-in URL; see [§4.1.2](#412-manifest-url-resilience))
3. **Verify manifest signature** (Gate 1) — download `manifest.json.sig`,
   verify the detached ECDSA signature. If verification fails on the learned
   URL, fall back to the compiled-in URL. If both fail, abort (fail-closed).
4. **Parse manifest** — extract the channel entry for the device's configured
   channel. Select the asset matching `BOARD_ID`.
5. **Version comparison** — compare the manifest channel version against the
   running firmware version. If no update, stop here (2 HTTP requests total).
6. **Config snapshot** — `snapshotActiveToPrevious()` (see
   [§9. Config Lifecycle](#9-config-lifecycle))
7. **Init verifier** — load the RSA public key into `MbedTLSSignatureVerifier`
8. **Allocate HTTPS client** — `WiFiClientSecure` on the heap
9. **Create download engine** — configure `OTADownloadEngine`
10. **Fetch `.sig`** (Gate 2) — download the firmware signature file from the
    absolute URL in the manifest
11. **Download and flash** — download firmware from the absolute URL in the
    manifest. Read the first 1KB chunk, validate the ESP32 magic byte
    (`0xE9`) and the `esp_app_desc_t` embedded version string against the
    expected release version (Gate 3). If the binary version doesn't match,
    abort before any flash writes occur. Then stream the remaining firmware
    in 1024-byte chunks, feeding the verifier and the task watchdog.
12. **Finalise and reboot** — 2-second delay for EventBus event propagation,
    then `esp_restart()`

**Request count summary:**

| Outcome | HTTP Requests |
|---------|---------------|
| No update available | 2 (manifest + manifest signature) |
| Update available | 4 (manifest + manifest sig + firmware + firmware sig) |

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

## 4. Release Channels & Manifest-First Discovery

### 4.1 Release Discovery (Manifest-First)

The OTA system uses a **manifest-first** architecture. A single signed
`manifest.json` is the sole entry point for all OTA operations. The manifest
covers all channels, contains absolute download URLs, and is self-describing
(including its own URL for future migration). This eliminates `GithubClient`
from the OTA path and makes the system fully portable to any hosting provider.

**Discovery flow:**
```
OTAUpdateManager  → GET manifest.json                 (~3 KB, any host)
                  → GET manifest.json.sig              (~256 bytes)
                  → verify signature → parse → select channel
                  → compare version → stop (if no update)
                  → GET {firmware URL from manifest}    (direct, any host)
                  → GET {firmware.sig URL from manifest} (direct, any host)
                  → verify firmware signature → validate binary header → flash
```

| Channel | Tag Convention | Manifest Behaviour |
|---------|----------------|--------------------|
| `stable` | `v3.1.0` (no suffix) | `channels.stable` entry in manifest |
| `beta` | `v3.2.0-beta.1` | `channels.beta` entry in manifest |
| `dev` | `v3.2.0-dev.3` | `channels.dev` entry in manifest |

The deploy script (`deploy_release.py`) scans all releases and constructs
the manifest with the best available version for each channel, ensuring a
consistent risk floor. The device simply reads its configured channel from
the manifest — no scanning, no GitHub API pagination, no floating tags.

### 4.1.1 Manifest Signing

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

The `manifestUrl` field is a control-plane redirect — an attacker who can
inject a new URL captures the device's OTA channel permanently. Manifest
signing is the gate that prevents this.

**Verification rule:** Verify manifest signature FIRST, then parse. If
verification fails:
1. Do not trust any field from the response
2. If this was the learned URL → fall back to the compiled-in URL
3. If this was the compiled-in URL → abort the check (fail-closed)

### 4.1.2 Manifest URL Resilience

The device maintains **two** manifest URLs:

1. **Compiled-in URL** — set at build time in `FrameworkConfig.otaManifestUrl`,
   never overwritten, survives factory reset
2. **Learned URL** — stored in OTA module config (`learnedManifestUrl` in
   `/mod/ota.json`), updated from the `manifestUrl` field in a **verified**
   manifest response only

**Check order:** Try learned URL first → fall back to compiled-in URL. The
compiled-in URL is an unkillable safety net.

### 4.1.3 Defence-in-Depth

Three independent safety gates, each catching different failure modes:

```
Gate 1: Manifest signature     → prevents tampered URLs, versions, redirects
Gate 2: Firmware signature     → prevents tampered/malicious binaries
Gate 3: Binary version header  → prevents flashing wrong version (esp_app_desc_t check)
```

> **Note:** Gate 3 validates the `esp_app_desc_t.version` field embedded in
> the first 1KB of the downloaded binary against the expected version before
> any flash partition writes occur (`BinaryVersionMismatch` error code -16).

### 4.2 Channel Configuration

The active channel is persisted as an `IConfigurable` field in `/mod/ota.json`:

```json
{
  "schema": 5,
  "channel": "stable",
  "autoCheck": true,
  "updatesEnabled": true,
  "checkHour": 3,
  "lastCheckedChannel": "stable",
  "learnedManifestUrl": ""
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

The framework uses RSA-2048 streaming signature verification via the `IOTAVerifier` interface. When `FrameworkConfig.otaPublicKey` is configured, the pipeline is **fail-closed** — unsigned or mis-signed firmware is always rejected. When the public key is `nullptr`, verification is skipped (development only).

> For detailed coverage of signature verification, anti-rollback, key rotation,
> and the DRAM task constraint, see [Security § OTA Firmware Security](security.md#8-ota-firmware-security).

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

Key rotation requires a two-step transitional release: ship a firmware signed with the **old** key that embeds the **new** public key, then switch to signing with the new key.

> For the full step-by-step rotation procedure and security warnings, see [Security § Key Rotation](security.md#key-rotation).

---

## 7. Private Repository & PAT Configuration

If your OTA releases are hosted in a **private** GitHub repository, devices need a
Personal Access Token (PAT) to authenticate manifest and asset downloads. This
section covers token creation, repository layout options, and the
`deploy_release.py --repo` flag.

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

| Setup | `config.otaManifestUrl` | `otaSecrets.hpp` | `deploy_release.py` |
|---|---|---|---|
| Single private repo | `"https://github.com/owner/repo/releases/download/manifest-latest/manifest.json"` | Include PAT via `config.otaToken` | (default) |
| Split releases repo | `"https://github.com/owner/releases-repo/releases/download/manifest-latest/manifest.json"` | Include PAT via `config.otaToken` | `--repo owner/releases-repo` |
| Public repo | `"https://github.com/owner/public-repo/releases/download/manifest-latest/manifest.json"` | Omit (`nullptr`) | (default) |

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

### 8.1 Manifest Schema (v2)

The manifest uses a multi-channel schema with absolute asset URLs. A single
manifest covers all channels and all boards:

```json
{
  "manifestVersion": 1,
  "manifestUrl": "https://github.com/.../releases/download/manifest-latest/manifest.json",
  "channels": {
    "stable": {
      "version": "v1.0.0",
      "securityVersion": 1,
      "assets": [
        {
          "board": "esp32dev",
          "firmwareUrl": "https://github.com/.../releases/download/v1.0.0/firmware-esp32dev.bin",
          "signatureUrl": "https://github.com/.../releases/download/v1.0.0/firmware-esp32dev.bin.sig",
          "size": 1341651,
          "sha256": "abc123..."
        }
      ]
    },
    "dev": {
      "version": "v1.1.0-dev.3",
      "securityVersion": 1,
      "assets": [
        {
          "board": "esp32dev",
          "firmwareUrl": "https://github.com/.../releases/download/v1.1.0-dev.3/firmware-esp32dev.bin",
          "signatureUrl": "https://github.com/.../releases/download/v1.1.0-dev.3/firmware-esp32dev.bin.sig",
          "size": 1345000,
          "sha256": "def456..."
        }
      ]
    }
  }
}
```

**Key properties:**
- `manifestVersion: 1` — integer schema version, consistent with `IConfigurable::getSchemaVersion()` convention
- `manifestUrl` — self-referencing URL; device stores this for future migration (see [§4.1.2](#412-manifest-url-resilience))
- `channels` — keyed by channel name, each containing version + per-board assets
- Asset URLs are **absolute** — no GitHub URL construction needed on-device

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

The orchestrator calls `findAssetForBoard(channelEntry, BOARD_ID)` to resolve
the correct firmware binary for the running board from the manifest's channel
assets.

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

The framework implements a software-based anti-rollback counter using the `SECURITY_VERSION` compile-time macro and an NVS monotonic counter. Increment `SECURITY_VERSION` in `platformio.ini` `build_flags` when a release contains a security fix that must not be rolled back past.

> **Warning:** Incrementing `SECURITY_VERSION` is a **one-way operation**. Once
> devices update past this barrier, they cannot be rolled back to any firmware
> with a lower security version — even via the web portal rollback button.

> For the full anti-rollback implementation details and check logic, see [Security § Anti-Rollback Protection](security.md#anti-rollback-protection).

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
3. Generates `manifest.json` (v2 multi-channel schema) with absolute asset URLs
4. Signs the manifest: `openssl dgst -sha256 -sign private_key.pem manifest.json`
5. Creates the versioned GitHub Release: `gh release create <tag>`
6. Uploads versioned assets: `.bin`, `.bin.sig`, `.bin.sha256`
7. Uploads/updates the `manifest-latest` fixed release with `manifest.json` + `manifest.json.sig`

The manifest is hosted at a fixed release tag (`manifest-latest`) so the device
always fetches from the same URL regardless of which version is latest.

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
| `otaPublicKey` | `const char*` | `nullptr` | PEM-encoded RSA public key (for manifest + firmware sig verification) |
| `otaManifestUrl` | `const char*` | `nullptr` | Compiled-in manifest URL — unkillable fallback for manifest-first OTA |
| `otaToken` | `const char*` | `nullptr` | Optional bearer token for authenticated manifest/asset downloads |

### IConfigurable Fields (persisted in `/mod/ota.json`, schema version 5)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | `char[16]` | `"stable"` | Active release channel |
| `autoCheck` | `bool` | `true` | Auto-check on boot |
| `updatesEnabled` | `bool` | `true` | Master kill-switch for applying firmware updates |
| `checkHour` | `uint8_t` | `3` | Hour (0–23) for the daily scheduled check |
| `lastCheckedChannel` | `char[16]` | `""` | Implicit downgrade detection |
| `learnedManifestUrl` | `char[256]` | `""` | Learned manifest URL — updated from verified manifest responses |

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
| `BinaryVersionMismatch` | -16 | Binary's embedded version doesn't match expected |

---

## 17. Security Checklist

Before deploying to production, verify the OTA security posture:

- [ ] `FrameworkConfig.otaPublicKey` is set to your PEM-encoded RSA public key
- [ ] `FrameworkConfig.otaManifestUrl` points to your `manifest-latest` release
- [ ] `SECURITY_VERSION` is set in all PlatformIO environments
- [ ] `manifest.json` is signed and uploaded to the `manifest-latest` release on every deploy
- [ ] `manifest.json.sig` (detached signature) accompanies the manifest
- [ ] All `.bin` assets have corresponding `.bin.sig` files
- [ ] The `deploy_release.py` script is used for signing (not CI)
- [ ] All three defence-in-depth gates are active (manifest sig → firmware sig → binary version header)

> For the full security audit checklist (private key handling, bootloader rollback,
> TWDT panic, NVS encryption), see [Security § Security Audit Checklist](security.md#10-security-audit-checklist).
