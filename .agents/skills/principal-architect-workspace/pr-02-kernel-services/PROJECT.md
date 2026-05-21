# PROJECT.md — PR-02: Kernel Services

**Program:** ESP32 App Foundation  
**Branch:** `pr-02-kernel-services`  
**Status:** PLANNING  
**Principal Architect Session:** 30aaf023-db75-4615-bf03-5b7bf9e9bacc *(inherited)*  
**Last Updated:** 2026-05-21  

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
| WO1 | QUEUED | `pr-02-wo1-lowlevel-libs` | [wo1-lowlevel-libs/WORKORDER.md](wo1-lowlevel-libs/WORKORDER.md) |
| WO2 | QUEUED | `pr-02-wo2-configmanager` | [wo2-configmanager/WORKORDER.md](wo2-configmanager/WORKORDER.md) |
| WO3 | QUEUED | `pr-02-wo3-keymanager` | [wo3-keymanager/WORKORDER.md](wo3-keymanager/WORKORDER.md) |
| WO4 | QUEUED | `pr-02-wo4-wifimanager` | [wo4-wifimanager/WORKORDER.md](wo4-wifimanager/WORKORDER.md) |
| WO5 | QUEUED | `pr-02-wo5-timemanager` | [wo5-timemanager/WORKORDER.md](wo5-timemanager/WORKORDER.md) |
| WO6 | QUEUED | `pr-02-wo6-otaupdatemanager` | [wo6-otaupdatemanager/WORKORDER.md](wo6-otaupdatemanager/WORKORDER.md) |

---

## Git Branch Map

```
main
  └─ pr-02-kernel-services            ← PA feature branch (integration target)
       pr-02-wo1-lowlevel-libs        ← to be created
       pr-02-wo2-configmanager        ← to be created
       pr-02-wo3-keymanager           ← to be created
       pr-02-wo4-wifimanager          ← to be created
       pr-02-wo5-timemanager          ← to be created
       pr-02-wo6-otaupdatemanager     ← to be created
```

---

*Principal Architect — PR-02 — 2026-05-21*
