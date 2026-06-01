# PR-14 — OTA Production System

## Holistic Architectural Shift

Transform the foundation's prototype OTA subsystem (PR-09: download engine + verifier + manager) into a production-grade firmware update pipeline. Adds SemVer comparison, release channels, multi-architecture manifests, config lifecycle management, auto-rollback wiring, web portal OTA UI, CI/CD, and comprehensive documentation.

### Foundation: What PR-09 Already Delivered

| Component | Status | Location |
|-----------|--------|----------|
| `IOtaVerifier` interface | ✅ Merged | `include/interfaces/IOtaVerifier.hpp` |
| `MbedTLSSignatureVerifier` | ✅ Merged | `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp/.cpp` |
| `OtaDownloadEngine` | ✅ Merged | `lib/otaDownloadEngine/OtaDownloadEngine.hpp/.cpp` |
| `OtaError` enum | ✅ Merged | `lib/otaDownloadEngine/OtaError.hpp/.cpp` |
| `otaUpdateManager` orchestrator | ✅ Merged | `modules/otaUpdateManager/otaUpdateManager.hpp/.cpp` |
| FreeRTOS task-based download | ✅ Merged | `otaUpdateManager::performUpdateTask()` |
| EventBus `ota.progress` | ✅ Merged | `OtaProgressEvent` payload |
| `markAppValid()` / `rollbackFirmware()` | ✅ Merged | Methods exist but are not wired into boot |
| `sdkconfig.defaults` rollback | ✅ Merged | `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` |
| Design spec §13.3 | ✅ Merged | OTA pipeline documentation |

### Foundation: What PR-11 Already Delivered

| Component | Status | Location |
|-----------|--------|----------|
| `iConfigurable` interface | ✅ Merged | `include/interfaces/iConfigurable.hpp` |
| Per-module `_v` schema versioning | ✅ Merged | `ConfigManager::loadModuleConfig()` |
| `migrateConfig(section, fromVersion)` | ✅ Merged | Schema migration on boot |
| `ConfigManager` as gateway | ✅ Merged | `modules/configManager/configManager.hpp/.cpp` |
| `otaUpdateManager` as `iConfigurable` | ✅ Merged | Own namespace, serialise/deserialise |

### Design Decisions (Agreed with User)

| Decision | Outcome | Rationale |
|----------|---------|-----------|
| **Versioning** | SemVer 2.0 (`MAJOR.MINOR.PATCH`) + 32-bit packed integer | Zero-allocation MCU comparison; replaces lexicographic string compare |
| **Release channels** | `dev`/`beta`/`stable` via GitHub pre-release flags | Channel stored in OTA module config; implicit downgrade on channel change |
| **Multi-architecture** | Compile-time `BOARD_ID`; per-board assets in `manifest.json` | `firmware-esp32dev.bin`, `firmware-esp32s3nano.bin` |
| **Signing** | RSA-2048 app-level (unchanged — PR-09) | Local-only signing, no keys in CI |
| **Public key** | Compiled-in only (no NVS override) | NVS override creates attack vector; physical re-flash for key rotation |
| **Auto-rollback** | `markAppValid()` call in boot — already enabled in sdkconfig | Need to wire the call into `Application::begin()` at the correct barrier point |
| **Anti-rollback** | Software-based via NVS (`security_version`) | Reversible; appropriate for hobbyist device; no eFuses |
| **Config preservation** | Three-Tier Lifecycle (Active + Previous + Golden) via `config_manifest.json` | Versioned filenames preserved (per-module `_v`); manifest controls pruning |
| **CI/CD** | GitHub Actions builds only; signing is local | CI produces unsigned artifacts; developer signs and publishes |
| **Channel downgrade** | Implicit via channel change | OTA allows `remote < local` when channel changed |
| **Web portal** | Channel dropdown + "Check Now" + "Rollback to Previous" | Three OTA actions in System tab |
| **Departures-board** | Read-only reference for ideas | All work targets esp32-app-foundation |

### Reference Material (Read-Only — departures-board patterns to mine)

| Pattern | Location (departures-board) | Purpose |
|---------|---------------------------|---------|
| HTTPUpdateGitHub | `lib/hTTPUpdateGitHub/HTTPUpdateGitHub.cpp` | Reference for redirect-aware download (already ported in PR-09) |
| githubClient streaming parser | `lib/githubClient/githubClient.hpp` | Reference for GitHub API integration |
| otaUpdater orchestrator | `lib/otaUpdater/otaUpdater.cpp` | Reference for version comparison, quiet hours |
| publicKey.hpp | `lib/publicKey/publicKey.hpp` | Reference for compiled-in key pattern |
| deploy_release.py | `scripts/deploy_release.py` | Reference for release automation |
| generate_keys.sh | `scripts/generate_keys.sh` | Reference for RSA key generation |
| build_timestamp.py | `scripts/build_timestamp.py` | Reference for git-aware build metadata |

---

## Work Order Ledger

| WO | Topic | Status | Key Files | Dependencies |
|----|-------|--------|-----------|--------------|
| **[ ] [wo1-semver-engine](wo1-semver-engine/WORKORDER.md)** | SemVer 2.0 engine: `FirmwareVersion` struct, packed 32-bit comparison, `parseVersionTag()`, version injection build script | ⬜ READY | new `lib/semver/`, `platformio.ini` build_flags | None |
| **[ ] [wo2-rollback-wiring](wo2-rollback-wiring/WORKORDER.md)** | Wire `markAppValid()` into `Application::begin()` at self-test barrier; NVS `security_version` counter | ⬜ READY | `modules/application/Application.cpp`, `modules/otaUpdateManager/otaUpdateManager.cpp` | None |
| **[ ] [wo3-multi-arch-manifest](wo3-multi-arch-manifest/WORKORDER.md)** | Multi-architecture support: `BOARD_ID` build injection, `manifest.json` schema + parser, architecture-aware asset selection in `otaUpdateManager` | ⬜ READY | `platformio.ini`, new `lib/otaManifest/`, `modules/otaUpdateManager/` | WO1 |
| **[ ] [wo4-release-channels](wo4-release-channels/WORKORDER.md)** | Release channel subscription: `otaChannel` in OTA config, GitHub API channel-filtered queries, implicit downgrade logic, `lastCheckedChannel` tracking | ⬜ READY | `modules/otaUpdateManager/otaUpdateManager.cpp`, `lib/githubClient/` | WO1, WO3 |
| **[ ] [wo5-config-lifecycle](wo5-config-lifecycle/WORKORDER.md)** | Three-Tier config lifecycle: config manifest (`/config_manifest.json`), slot-aware pruning, golden config on blessing, integration with `ConfigManager` | ⬜ READY | new `lib/configManifest/` or extend `modules/configManager/`, integration with `otaUpdateManager::markAppValid()` | WO2 |
| **[ ] [wo6-web-portal-ota](wo6-web-portal-ota/WORKORDER.md)** | Web portal OTA UI: channel dropdown, "Check Now" button, "Rollback" button, firmware version display + partition info | ⬜ READY | `data/web/`, `modules/otaUpdateManager/` (route additions) | WO4 |
| **[ ] [wo7-ci-pipeline](wo7-ci-pipeline/WORKORDER.md)** | GitHub Actions CI: matrix build, hash generation, unsigned artifact upload; local signing script; `manifest.json` generator | ⬜ READY | new `.github/workflows/build.yml`, new `scripts/deploy_release.py`, new `scripts/generate_manifest.py`, new `scripts/generate_keys.sh` | WO1, WO3 |
| **[ ] [wo8-documentation](wo8-documentation/WORKORDER.md)** | Update design spec §13.3 with SemVer, channels, manifest, rollback, config lifecycle. Add OTA Developer Guide section. | ⬜ READY | `docs/design_specification.md`, new `docs/OtaDeveloperGuide.md` | All above |
| **[ ] [wo9-e2e-validation](wo9-e2e-validation/WORKORDER.md)** | End-to-end validation: CI build → local sign → release → device OTA → rollback → channel change → config preservation | ⬜ READY | Integration test scripts, validation checklist | All above |

### Conflict Analysis

```
WO1 (SemVer) ──── WO3 (Multi-Arch) ──── WO4 (Channels) ──── WO6 (Web Portal)
                                                              │
WO2 (Rollback) ── WO5 (Config Lifecycle) ─────────────────────┘
                                                              
WO1 + WO3 ──── WO7 (CI Pipeline)                             
                                                              
All ──── WO8 (Documentation) ──── WO9 (E2E Validation)
```

- **WO1 + WO2**: Can run in PARALLEL (no file overlap — semver lib vs Application.cpp)
- **WO3**: Depends on WO1 (needs `FirmwareVersion` types for manifest parsing)
- **WO4**: Depends on WO1 + WO3 (channels use packed version + manifest)
- **WO5**: Depends on WO2 (golden config tied to blessing call)
- **WO6**: Depends on WO4 (portal needs channel field + version endpoints)
- **WO7**: Depends on WO1 + WO3 (CI needs version extraction + multi-arch matrix)
- **WO8**: Depends on all above (documents final state)
- **WO9**: Depends on all above (validates the full chain)

**Recommended execution order**: `WO1 ‖ WO2` → `WO3` → `WO4 ‖ WO5` → `WO6 ‖ WO7` → `WO8` → `WO9`

---

*Created by Principal Architect — 2026-06-01*
