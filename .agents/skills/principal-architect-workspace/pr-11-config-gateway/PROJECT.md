# PR-11: ConfigManager-as-Gateway Refactoring

## Objective

Refactor `ConfigManager` from a passive `SystemConfig` struct holder into a centralised configuration gateway that owns all file I/O, schema versioning, migration orchestration, and backup/restore coordination — while maintaining the hybrid multi-file storage pattern optimised for ESP32 memory constraints.

## Architectural Context

**Design session:** Conversation `a9c36bb4-efe4-4b53-b51c-7ec5449e3ca8`
**Implementation plan:** See session artifacts for full plan with all 12 resolved decisions (A1-A4, B1-B3, C1-C2, D1-D2).
**Embedded review:** Approved — hybrid multi-file storage, fixed-size registry, self-describing `_v` versioning.

### Key Decisions

| Decision | Resolution |
|----------|------------|
| Storage model | Hybrid: multi-file (`/mod/<namespace>.json`), one per module. ConfigManager owns all file I/O. |
| Version tracking | Self-describing `_v` field in each config file (no separate meta file) |
| Version semantics | Schema version (code-level), NOT save-count. Bumped when firmware changes require schema migration. |
| Registry | Fixed-size `iConfigurable*[MAX_CONFIGURABLES]` array (no `std::vector`, no `std::map`) |
| Secrets | Passwords/keys via `KeyManager` (AES-256). NOT in config files. |
| Domain ownership | Hostname → WiFi, Timezone → TimeManager, OTA settings → OtaUpdateManager |
| Navigation | `IWebContributor` gets `getParentTab()` + `getTabOrder()` for primary tabs + segmented sub-nav |
| Save pattern | Per-endpoint. ConfigManager orchestrates backup/restore only. |

## Work Order Ledger

- **[x] [wo1-interface-refactor](wo1-interface-refactor/WORKORDER.md):** Redesign `iConfigurable` interface (6 methods), refactor `ConfigManager` to gateway, update `IWebContributor` with navigation hierarchy, migrate all implementors (`WifiManager`, `U8g2DisplayModule`), update `Application.cpp` boot sequence. Register `ConfigManager` + `otaUpdateManager` as web contributors.
- **[ ] [wo2-core-service-ownership](wo2-core-service-ownership/WORKORDER.md):** Promote core domain services (WifiManager, otaUpdateManager, TimeManager, KeyManager) to Application-owned members. Wire into boot sequence and ConfigManager gateway. Add navigation hierarchy to OTA. Make TimeManager ITickable.

## Branch State

| Branch | Base | Status |
|--------|------|--------|
| `pr-11-config-gateway` | `main` | Feature PR branch |
| `pr-11-config-gateway-wo1-interface-refactor` | `pr-11-config-gateway` | ✅ Complete — all tests pass, both HW targets build |
| `pr-11-config-gateway-wo2-core-service-ownership` | `pr-11-config-gateway` | 🔄 Active |

---

*Created by Principal Architect — 2026-05-26*
