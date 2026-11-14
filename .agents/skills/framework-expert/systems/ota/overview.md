# OTA Update System Overview

The OTA (Over-The-Air) subsystem provides firmware updates via GitHub Releases with cryptographic signature verification, release channel management, and automatic rollback safety.

## Architecture

The OTA subsystem follows a layered architecture with strict dependency inversion:

| Component | Location | Role |
|---|---|---|
| `OTAUpdateManager` | `modules/otaUpdateManager/` | Orchestrator — state machine, FreeRTOS task, REST API, EventBus |
| `OTADownloadEngine` | `lib/OTADownloadEngine/` | Binary download, flash write, signature integration |
| `IOTAVerifier` | `include/interfaces/` | Verification interface (three-step: begin/update/verify) |
| `MbedTLSSignatureVerifier` | `lib/OTADownloadEngine/` | RSA+SHA256 verification via mbedtls |
| `SemVer` | `lib/SemVer/` | Firmware version parsing, packed 32-bit comparison |
| `OTAManifest` | `lib/OTAManifest/` | Multi-architecture manifest parser |
| `ConfigManifest` | `lib/ConfigManifest/` | Three-Tier config lifecycle (Active/Previous/Golden) |
| `GithubClient` | `lib/GithubClient/` | GitHub API client for release queries |

**Key design principle:** The `OTAUpdateManager` module depends on `lib/` libraries, but the libraries have no knowledge of the module (Hollywood Principle). The download engine accepts a `WiFiClient&` from the caller — it never creates its own network client.

## State Machine

```
Idle ──→ Checking ──→ Idle              (no update found)
Idle ──→ Downloading ──→ Verifying ──→ Complete ──→ (reboot)
Any  ──→ Error                          (on failure)
```

| State | Description |
|---|---|
| `Idle` | Ready to accept a check or update |
| `Checking` | Querying GitHub for latest release metadata |
| `Downloading` | FreeRTOS task running the download pipeline |
| `Verifying` | Signature verification in progress |
| `Complete` | Update finalised — waiting for reboot |
| `Error` | An error occurred (query via REST API or `getLastOtaError()`) |

## Pipeline Steps

When `performUpdate()` is called, a FreeRTOS task (8KB stack, priority 1) executes:

1. **Guard checks** — firmware updates enabled? GitHub repo configured?
2. **Query GitHub** — `GithubClient::getLatestReleaseForChannel(_channel)`
3. **Download manifest** — parse `manifest.json` from release assets
4. **Board matching** — `findAssetForBoard(manifest, BOARD_ID)`
5. **Fail-closed check** — reject missing/empty `.sig` when public key is configured
6. **Config snapshot** — `snapshotActiveToPrevious()`
7. **Init verifier** — load the RSA public key into `MbedTLSSignatureVerifier`
8. **Allocate HTTPS client** — `WiFiClientSecure` on the heap
9. **Create download engine** — configure `OTADownloadEngine`
10. **Fetch `.sig`** — download the detached signature file
11. **Download and flash** — stream firmware in 1024-byte chunks
12. **Finalise and reboot** — 2-second delay for EventBus event propagation, then `esp_restart()`

## Release Channels

| Channel | Tag Convention | Behaviour |
|---|---|---|
| `stable` | `v3.1.0` (no suffix) | Fetches `/releases/latest` (GitHub "latest" endpoint — stable only) |
| `beta` | `v3.2.0-beta.1` | Fetches `/releases/latest` (stable baseline) + `/releases/tags/latest-beta` (floating tag) |
| `dev` | `v3.2.0-dev.3` | Fetches `/releases/latest` (stable baseline) + `/releases/tags/latest-dev` (floating tag) |

Pre-release channels use **floating tags** (`latest-dev`, `latest-beta`, etc.) for O(1) polling — exactly two HTTP requests regardless of release count. The `deploy_release.py` script force-moves these tags on each deployment. See `systems/ota/deployment.md` § Floating Tags for the full design constraint (the release `name` field carries the SemVer).

### SemVer Comparison

**Bit layout:** `MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(4) | PRE_NUM(4)`

**Channel ordering:** `DEV(0) < BETA(2) < RC(3) < STABLE(15)` — a stable release always sorts higher than any pre-release of the same major.minor.patch.

## Post-Reboot Self-Test

After reboot, the new firmware enters **pending verification**. The `Application` hub runs the full boot sequence (Steps 1–17), and only at **Step 17b** calls `markAppValid()`. If any step crashes before blessing, the ESP-IDF bootloader automatically reverts to the previous firmware partition.

## Cross-References

- **Human docs:** [`docs/developer/ota-update-system.md`](../../../../../docs/developer/ota-update-system.md)
- **Interface contract:** [`interfaces/ota-verifier.md`](../../interfaces/ota-verifier.md)
- **Security concerns:** [`security/ota-security.md`](../../security/ota-security.md)
- **Config lifecycle details:** [`systems/ota/config-lifecycle.md`](config-lifecycle.md)
- **Deployment procedures:** [`systems/ota/deployment.md`](deployment.md)
