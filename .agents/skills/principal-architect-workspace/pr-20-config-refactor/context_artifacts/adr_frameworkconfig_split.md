# ADR — FrameworkConfig Two-Layer Split (PR-20)

## Decision
`FrameworkConfig` conflated two concerns. Split them by **ownership layer**:
- **KEEP** (compile-time secrets the framework cannot know): `otaPublicKey`, `otaGithubRepo`, `otaGithubToken`.
- **MOVE** (runtime module defaults masquerading as framework config) into each owning module's `resetConfig()` factory default: `hostname`, `timezone`, `mcpReadOnly`, `firmwareUpdatesEnabled`, `otaQuietHour`.

**End state:** `FrameworkConfig` = the 3 OTA secrets only.

## Why
`main.cpp` was configuring module *runtime* behaviour through a framework-level constant struct — the wrong layer (false coupling between app entrypoint and module internals). Module defaults belong in the module that owns the persisted config.

## Key sizing insight (drove the WO decomposition)
"Move a field into `resetConfig()`" only works if the field is (or becomes) a **persisted** module config (`serializeConfig`/`applyConfig`/`resetConfig`). The modules were **not uniform**, so the work split on that axis:
- **Already persisted → clean move (WO2a):** `hostname` (WiFiManager), `timezone` (TimeManager), `mcpReadOnly` (MCPServer).
- **Not persisted → new persistence (WO2b):** `firmwareUpdatesEnabled` (OTAUpdateManager) — was read *live* from `app_->getConfig()`.

## WO decomposition
- **WO1 — investigation only:** clarify `appconfig.js` (turned out to be the Display tab controller, misnomer — *complements*, zero overlap), resolve the `portal-tab-wip` stash, map portal exposure per field.
- **WO2a — already-persisted moves:** the 3 clean moves + cross-module hostname re-sourcing + tests.
- **WO2b — OTA new persistence:** persist `firmwareUpdatesEnabled` as `updatesEnabled` + full OTA-tab toggle; delete dead `otaQuietHour`; final slim.

Rationale for the split: WO2-as-one-order was `architect`-tier with a wide blast radius; the 2-way split yielded two tight, independently-verifiable `standard` orders (serialized — they share `FrameworkConfig.hpp` + `Application.cpp`).

## Resolved design decisions (settled with user)
1. **Hostname → WiFiManager-owned**, but the **app names the default** via a WiFiManager-scoped API: `setDefaultHostname(...)` called from `main.cpp` through `app.getWifiManager()` *before* `app.begin()`. NOT via a reintroduced god-struct. `resetConfig()` falls back to that default (framework fallback `"esp32-device"`).
2. **timezone → TimeManager** `resetConfig()` default `"GMT0"`; `begin()` drops its param. No app override (main never set it).
3. **mcpReadOnly → MCPServer** `resetConfig()` = **`true`** (secure-by-default, preserved). MCPServer **shed its `FrameworkConfig` dependency entirely** — `begin()` dropped the `FrameworkConfig*` param, gained a `WifiManager*` for the status-endpoint hostname (single source of truth).
4. **MCPServer + DiagnosticsBoard re-source hostname from WiFiManager** (`getHostname()`), not FrameworkConfig.
5. **otaQuietHour was DEAD** (zero code readers; `checkHour` already covers OTA scheduling) → **deleted**, not moved. Stale docs corrected.
6. **firmwareUpdatesEnabled → persisted `updatesEnabled`** (OTA config), default `true`, with **config schema v4 migration** and a **full OTA-tab toggle** (POST `/api/ota` + status + `ota.js`) mirroring the existing `autoCheck` field. The OO-review's broader `app_`-removal refactor was kept **out of scope**.

## Verification (end state)
Native unit tests **525/525 PASSED, 0 ERRORED**; `esp32dev` device build **SUCCESS**; web-test pre-build hook passed; portal asset regenerated.
