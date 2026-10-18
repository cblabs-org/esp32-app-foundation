# PR-14 — OTA Production System

## Holistic Architectural Shift

Transform the foundation's prototype OTA subsystem (PR-09: download engine + verifier + manager) into a production-grade firmware update pipeline. Adds SemVer comparison, release channels, multi-architecture manifests, config lifecycle management, auto-rollback wiring, web portal OTA UI, CI/CD, and comprehensive documentation.

### Foundation: What's Already Delivered

| Component | Status | Location (PascalCase) |
|-----------|--------|----------------------|
| `IOTAVerifier` interface | ✅ PR-09 | `include/interfaces/IOTAVerifier.hpp` |
| `MbedTLSSignatureVerifier` | ✅ PR-09 | `lib/OTADownloadEngine/MbedTLSSignatureVerifier.hpp/.cpp` |
| `OTADownloadEngine` | ✅ PR-09 | `lib/OTADownloadEngine/OTADownloadEngine.hpp/.cpp` |
| `OTAError` enum | ✅ PR-09 | `lib/OTADownloadEngine/OTAError.hpp/.cpp` |
| `OTAUpdateManager` orchestrator | ✅ PR-09 | `modules/OTAUpdateManager/OTAUpdateManager.hpp/.cpp` |
| FreeRTOS task-based download | ✅ PR-09 | `OTAUpdateManager::performUpdateTask()` |
| EventBus `ota.progress` | ✅ PR-09 | `OTAProgressEvent` payload |
| `markAppValid()` / `rollbackFirmware()` | ✅ PR-09 | Methods exist but NOT wired into boot |
| `sdkconfig.defaults` rollback | ✅ PR-09 | `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` |
| `IConfigurable` interface | ✅ PR-11 | `include/interfaces/IConfigurable.hpp` |
| Per-module `_v` schema versioning | ✅ PR-11 | `ConfigManager::loadModuleConfig()` |
| `migrateConfig(section, fromVersion)` | ✅ PR-11 | Schema migration on boot |
| `ConfigManager` gateway (dynamic vector) | ✅ PR-11 + `2e7d764` | `modules/ConfigManager/ConfigManager.hpp/.cpp` |
| **OTAUpdateManager as IConfigurable** | ✅ **PR-16** | `_channel[16]`, `_autoCheck`, schema v1, namespace `"ota"` |
| **OTA portal stubs** | ✅ **PR-16** | `web/portal/ota.js` — hidden channel dropdown, manifest info placeholder |
| Design spec §13.3 | ✅ PR-09 | OTA pipeline documentation |

### Design Decisions (Agreed with User)

| Decision | Outcome | Rationale |
|----------|---------|-----------|
| **Versioning** | SemVer 2.0 (`MAJOR.MINOR.PATCH`) + 32-bit packed integer | Zero-allocation MCU comparison; replaces lexicographic string compare |
| **Release channels** | `dev`/`beta`/`stable` via GitHub pre-release flags | Channel stored in OTA module config; implicit downgrade on channel change |
| **Multi-architecture** | Compile-time `BOARD_ID`; per-board assets in `manifest.json` | `firmware-esp32dev.bin`, `firmware-esp32s3nano.bin` |
| **Signing** | RSA-2048 app-level (unchanged — PR-09) | Local-only signing, no keys in CI |
| **Public key** | Compiled-in only (no NVS override) | NVS override creates attack vector; physical re-flash for key rotation |
| **Auto-rollback** | `markAppValid()` call in boot — already enabled in sdkconfig | Wire into `Application::begin()` after step 17 |
| **Anti-rollback** | Software-based via NVS (`security_version`) | Reversible; appropriate for hobbyist device; no eFuses |
| **Config preservation** | Three-Tier Lifecycle (Active + Previous + Golden) via `config_manifest.json` | Versioned filenames preserved (per-module `_v`); manifest controls pruning |
| **CI/CD** | GitHub Actions builds only; signing is local | CI produces unsigned artifacts; developer signs and publishes |
| **Channel downgrade** | Implicit via channel change | OTA allows `remote < local` when channel changed |
| **Web portal** | Channel dropdown + "Check Now" + "Rollback to Previous" | Three OTA actions in System tab |
| **Departures-board** | Read-only reference for ideas | All work targets esp32-app-foundation |
| **Fail-closed OTA** | 🔴 Assessment fix — abort on missing `.sig` when key configured | Fixes the single highest-risk security finding |
| **TWDT panic** | 🟠 Assessment fix — `CONFIG_ESP_TASK_WDT_PANIC=y` | Hung OTA task → panic → reboot → auto-rollback |
| **Rollback auth** | 🟠 Assessment fix — confirmation token for rollback endpoint | Destructive action requires `"confirm": "ROLLBACK"` |

---

## Work Order Ledger

| WO | Topic | Status | Key Files | Dependencies |
|----|-------|--------|-----------|--------------|
| **[ ] [wo1-semver-engine](wo1-semver-engine/WORKORDER.md)** | SemVer 2.0 engine: `FirmwareVersion` struct, packed 32-bit comparison, `parseVersionTag()`, version injection build script | ⬜ READY | new `lib/SemVer/`, `platformio.ini` build_flags | None |
| **[ ] [wo2-rollback-wiring](wo2-rollback-wiring/WORKORDER.md)** | Wire `markAppValid()` into boot; NVS `security_version`; **fix fail-open OTA verification** (🔴 assessment); add `CONFIG_ESP_TASK_WDT_PANIC` + TWDT subscription for OTA task | ⬜ READY | `modules/Application/Application.cpp`, `modules/OTAUpdateManager/OTAUpdateManager.cpp`, `lib/OTADownloadEngine/OTADownloadEngine.cpp`, `sdkconfig.defaults` | None |
| **[ ] [wo3-multi-arch-manifest](wo3-multi-arch-manifest/WORKORDER.md)** | Multi-architecture support: `BOARD_ID` build injection, `manifest.json` schema + parser, architecture-aware asset selection | ⬜ READY | `platformio.ini`, new `lib/OTAManifest/`, `modules/OTAUpdateManager/` | WO1 |
| **[ ] [wo4-release-channels](wo4-release-channels/WORKORDER.md)** | **Reduced scope**: Extend existing IConfigurable (schema v1→v2), add `lastCheckedChannel`, GitHub API channel-filtered queries, implicit downgrade logic | ⬜ READY | `modules/OTAUpdateManager/OTAUpdateManager.hpp/.cpp`, `lib/GithubClient/` | WO1, WO3 |
| **[ ] [wo5-config-lifecycle](wo5-config-lifecycle/WORKORDER.md)** | Three-Tier config lifecycle: config manifest (`/config_manifest.json`), slot-aware pruning, golden config on blessing, integration with `ConfigManager` | ⬜ READY | new `lib/ConfigManifest/`, `modules/ConfigManager/`, `modules/OTAUpdateManager/` | WO2 |
| **[ ] [wo6-web-portal-ota](wo6-web-portal-ota/WORKORDER.md)** | **Reduced scope**: Activate existing PR-14 stubs in `ota.js`, add rollback **with confirmation token** (🟠 assessment), wire new API actions | ⬜ READY | `web/portal/ota.js`, `modules/OTAUpdateManager/` | WO4 |
| **[ ] [wo7-ci-pipeline](wo7-ci-pipeline/WORKORDER.md)** | GitHub Actions CI: matrix build, hash generation, unsigned artifact upload; local signing script; `manifest.json` generator | ⬜ READY | new `.github/workflows/build.yml`, new `scripts/deploy_release.py`, `scripts/generate_manifest.py`, `scripts/generate_keys.sh` | WO1, WO3 |
| **[ ] [wo8-documentation](wo8-documentation/WORKORDER.md)** | Update design spec §13.3; add OTA Developer Guide with **production hardening** section (Flash Encryption, Secure Boot, `setInsecure()` rationale) and **security gap register** | ⬜ READY | `docs/design_specification.md`, new `docs/OTADeveloperGuide.md` | All above |
| **[ ] [wo9-e2e-validation](wo9-e2e-validation/WORKORDER.md)** | End-to-end: CI build → local sign → release → OTA → fail-closed test → rollback → channel change → config preservation | ⬜ READY | Integration test scripts, validation checklist | All above |

### Conflict Analysis

```
WO1 (SemVer) ──── WO3 (Multi-Arch) ──── WO4 (Channels) ──── WO6 (Web Portal)
                                                              │
WO2 (Rollback) ── WO5 (Config Lifecycle) ─────────────────────┘
                                                              
WO1 + WO3 ──── WO7 (CI Pipeline)                             
                                                              
All ──── WO8 (Documentation) ──── WO9 (E2E Validation)
```

- **WO1 + WO2**: Can run in PARALLEL (no file overlap — `lib/SemVer/` vs `Application.cpp` + `OTADownloadEngine`)
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
*Updated: 2026-06-06 — stash@{4} (pr-14 OTA workspace edits) reviewed and merged; PascalCase upstream version retained as canonical*
