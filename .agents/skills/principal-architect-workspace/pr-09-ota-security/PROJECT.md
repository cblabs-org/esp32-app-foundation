# PR-09 — OTA Security Pipeline

## Holistic Architectural Shift

The current `otaUpdateManager` module is a thin wrapper around ESP32's `Update.h`
library — it exposes `beginUpdate/writeStream/endUpdate/markAppValid/rollbackFirmware`
but has **no download logic, no GitHub integration, no redirect handling, no binary
verification, and no cryptographic signature validation**.

This project closes the gap by porting the departures-board's sophisticated
`HTTPUpdateGitHub` + `MbedTLSOtaVerifier` pipeline into the framework as a
properly decomposed, interface-driven architecture:

```
┌─────────────────────────────────────────────────────────┐
│ FrameworkConfig.otaPublicKey  (app-injected PEM)        │
└──────────────┬──────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────┐
│ otaUpdateManager (module)                               │
│  ├── checkForUpdate() → GithubClient                    │
│  ├── performUpdate()  → OtaDownloadEngine               │
│  │     ├── redirect-aware HTTP GET (≤5 hops)            │
│  │     ├── .sig file fetch + redirect handling           │
│  │     ├── binary header check (0xE9)                   │
│  │     ├── partition space validation                    │
│  │     ├── chunked streaming write → Update.h           │
│  │     ├── per-chunk IOtaVerifier::update()             │
│  │     └── IOtaVerifier::verify(sig) post-download      │
│  ├── OtaError enum + human-readable messages            │
│  └── EventBus: ota.progress                             │
└─────────────────────────────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────────────┐
│ IOtaVerifier (interface)                                │
│  └── MbedTLSSignatureVerifier (default impl)            │
│       RSA + SHA256 via mbedtls                          │
└─────────────────────────────────────────────────────────┘
```

## Key Architectural Constraints

1. **Hollywood Principle** — No cross-module includes
2. `IOtaVerifier` → `include/interfaces/`
3. `OtaDownloadEngine` → `lib/otaDownloadEngine/` (utility, not module)
4. `MbedTLSSignatureVerifier` → `lib/otaDownloadEngine/` (default verifier impl)
5. `WiFiClientSecure` heap-allocated via `std::unique_ptr<>` (spec rule 14.3)
6. `tick() ≤ 10ms` — downloads in FreeRTOS task or non-blocking
7. `OtaDownloadEngine` accepts any `WiFiClient&` passed by caller
8. All new code must have native unit tests (`#ifndef NATIVE_BUILD` guards)
9. Public key injected via `FrameworkConfig.otaPublicKey` — never hardcoded

## Reference Source Files

| File | Location | Purpose |
|------|----------|---------|
| Current OTA manager | `modules/otaUpdateManager/otaUpdateManager.hpp/.cpp` | Thin wrapper to upgrade |
| GithubClient | `lib/githubClient/githubClient.hpp/.cpp` | Release metadata (already exists) |
| FrameworkConfig | `include/FrameworkConfig.hpp` | Config struct (needs `otaPublicKey` field) |
| Departures-board OTA | `departures-board:lib/hTTPUpdateGitHub/hTTPUpdateGitHub.hpp/.cpp` | Complete reference pipeline |
| MbedTLSOtaVerifier | `departures-board:hTTPUpdateGitHub.cpp:48-93` | Reference verifier class |
| Design spec §13.3 | `docs/design_specification.md:1428` | Needs crypto verification docs |
| Interface patterns | `include/interfaces/ITickable.hpp`, `IWebContributor.hpp` | Style reference |

---

## Work Order Ledger

| WO | Topic | Status | Files Touched | Dependencies |
|----|-------|--------|---------------|--------------|
| **[x] [wo1-interface-verifier](wo1-interface-verifier/WORKORDER.md)** | `IOtaVerifier` interface + `MbedTLSSignatureVerifier` default impl + native unit tests | ✅ MERGED `09be1ef` | `include/interfaces/IOtaVerifier.hpp`, `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp/.cpp`, `test/test_unit/test_ota_verifier.cpp` | None |
| **[x] [wo2-download-engine](wo2-download-engine/WORKORDER.md)** | `OtaDownloadEngine` — redirect-aware download, sig fetch, header check, partition validation, chunked streaming + verifier integration | ✅ MERGED `0b6cd07` | `lib/otaDownloadEngine/OtaDownloadEngine.hpp/.cpp`, `lib/otaDownloadEngine/OtaError.hpp/.cpp`, `test/test_unit/test_ota_download_engine.cpp` | WO1 |
| **[x] [wo3-manager-upgrade](wo3-manager-upgrade/WORKORDER.md)** | Upgrade `otaUpdateManager` — wire download engine + verifier, add `checkForUpdate()`/`performUpdate()`, structured errors, ESP_LOGx, EventBus, FrameworkConfig `otaPublicKey` | ✅ MERGED `9a07d0b` | `modules/otaUpdateManager/otaUpdateManager.hpp/.cpp`, `include/FrameworkConfig.hpp`, `modules/application/Application.hpp/.cpp`, `test/test_unit/test_otaUpdateManager.cpp` | WO1, WO2 |
| **[x] [wo4-spec-update](wo4-spec-update/WORKORDER.md)** | Update design specification §13.3 with full cryptographic verification pipeline documentation | ✅ MERGED `c6ebb51` | `docs/design_specification.md` | WO1, WO2, WO3 |

### Conflict Analysis

- **WO1 → WO2**: Sequential. WO2 depends on `IOtaVerifier` from WO1.
- **WO2 → WO3**: Sequential. WO3 wires the download engine from WO2.
- **WO3 → WO4**: Sequential. WO4 documents the final architecture.
- **No parallel execution possible** — strict linear dependency chain.

---

*Created by Principal Architect — 2026-05-22*
