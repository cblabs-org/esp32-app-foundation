# PROJECT.md — PR-02: Kernel Services

**Program:** ESP32 App Foundation  
**Branch:** `pr-02-kernel-services`  
**Status:** ✅ MERGED to `main`  
**Principal Architect Session:** 30aaf023-db75-4615-bf03-5b7bf9e9bacc *(inherited)*  
**Last Updated:** 2026-06-02 (retrospective update)  

---

## Objective

Establish all "always-present" kernel services. Each is a self-contained unit wired into the `Application` hub via accessor methods.
This includes `deviceCrypto`, `logger`, `md5Utils`, `githubClient`, `configManager`, `keyManager`, `wifiManager`, `timeManager`, and `otaUpdateManager`.
All code must be authored fresh; no copying wholesale from `departures-board`, though patterns can be referenced.

---

## Architectural Context

| Service | Path | Key Notes |
|---|---|---|
| `deviceCrypto` | `lib/deviceCrypto/` | Hardware MAC key derivation, AES-256 |
| `logger` | `lib/logger/` | ESP-IDF wrapper + `LOG_SECURE()` macro |
| `md5Utils` | `lib/md5Utils/` | Streaming MD5 for OTA verification |
| `githubClient` | `lib/githubClient/` | HTTPS + cert validation |
| `configManager` | `modules/configManager/` | LittleFS `etc/config.json`, versioning, `requestReload()` |
| `keyManager` | `modules/keyManager/` | Encrypted registry, `addKey/getKeyById/deleteKey` |
| `wifiManager` | `modules/wifiManager/` | AP captive portal, `AppState` transitions |
| `timeManager` | `modules/timeManager/` | NTP, POSIX timezone, non-blocking |
| `otaUpdateManager` | `modules/otaUpdateManager/` | Streaming MD5, rollback, bless AFTER WiFi+LittleFS |

---

## Work Order Ledger

| # | Status | Branch | Link |
|---|---|---|---|
| WO1 | **✅ MERGED** | ~~`pr-02-wo1-lowlevel-libs`~~ → `pr-02-kernel-services` | [wo1-lowlevel-libs/WORKORDER.md](wo1-lowlevel-libs/WORKORDER.md) |
| WO2 | **✅ MERGED** | ~~`pr-02-wo2-configmanager`~~ → `pr-02-kernel-services` | [wo2-configmanager/WORKORDER.md](wo2-configmanager/WORKORDER.md) |
| WO3 | **✅ MERGED** | ~~`pr-02-wo3-keymanager`~~ → `pr-02-kernel-services` | [wo3-keymanager/WORKORDER.md](wo3-keymanager/WORKORDER.md) |
| WO4 | **✅ MERGED (HW Verified)** | ~~`pr-02-wo4-wifimanager`~~ → `pr-02-kernel-services` | [wo4-wifimanager/WORKORDER.md](wo4-wifimanager/WORKORDER.md) |
| WO5 | **✅ MERGED** | ~~`pr-02-wo5-timemanager`~~ → `pr-02-kernel-services` | [wo5-timemanager/WORKORDER.md](wo5-timemanager/WORKORDER.md) |
| WO6 | **✅ MERGED (HW Pending)** | ~~`pr-02-wo6-otaupdatemanager`~~ → `pr-02-kernel-services` | [wo6-otaupdatemanager/WORKORDER.md](wo6-otaupdatemanager/WORKORDER.md) |

---

## Git Branch Map

```
main  ← all PR-02 work merged, branch deleted
```

---

## Distillation Artifacts

- [context_bridge.md](context_bridge.md) — PR-02 Kernel Services final state distillation
- [context_artifacts/](context_artifacts/) — ADRs and graveyards:
  - `adr_kernel_services.md` — Core architectural decisions
  - `graveyard_kernel_services.md` — Rejected approaches
  - `adr_wifi_state_machine.md` — WiFi state machine design (from hardware verification session)
  - `adr_webserver_deferred_start.md` — WebServer deferred start via EventBus
  - `adr_captive_portal.md` — Captive portal with DNS hijack
  - `graveyard_native_build_stubs.md` — Native build stub unification
  - `state_files_changed.md` — WiFi integration file manifest

---

*Principal Architect — PR-02 — Retrospectively updated 2026-06-02*
