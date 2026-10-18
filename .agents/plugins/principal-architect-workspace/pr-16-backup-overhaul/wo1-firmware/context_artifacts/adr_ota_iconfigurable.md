# ADR: OTAUpdateManager as IConfigurable — Design Decisions

## Context

`OTAUpdateManager` was the only core framework module not implementing `IConfigurable`. This meant OTA settings (release channel, auto-check preference) were not backed up or restored with the device config.

## Decision 1: Fields to Persist

**Fields chosen:**
- `channel` (char[16]): `"stable"` or `"beta"` — release channel for GitHub update checks
- `autoCheck` (bool): `true` — whether to check for updates on boot

**Why these two only?** All other OTA state (current firmware version, OTA progress, error strings) is runtime state, not persisted config. These are the only user preferences that need to survive a reboot or device restore.

## Decision 2: Namespace = "ota", Schema Version = 1

**Namespace:** `"ota"` maps to `/mod/ota.json` on LittleFS.

**Schema v1:** Initial schema. `migrateConfig()` is a no-op that logs the migration attempt — ready for future expansion.

## Decision 3: Heap Allocation Timing (Step 7 vs Step 13)

**Problem:** `OTAUpdateManager` was heap-allocated at Step 13 (after `beginAll()` at Step 8). This meant OTA could not be registered with ConfigManager before `beginAll()`, so OTA config was never loaded on boot.

**Decision:** Move `new OTAUpdateManager()` from Step 13 to Step 7 (the module registration block, before Step 8 `beginAll()`). Call `begin(*this)` at Step 13 as before.

**Why this is safe:** The `OTAUpdateManager` constructor is trivial (no side effects). `begin(*this)` still fires at the correct moment (after WiFi and Time are initialized, which OTA needs for network access). The heap allocation itself does not depend on WiFi or Time.

**Why the header is forward-declared in Application.hpp:** To avoid Library Dependency Finder (LDF) circular-include issues between Application and OTAUpdateManager. This is pre-existing design and was preserved.

## Decision 4: Timezone Coupling — setMetaTimezone() vs Direct Access

**Problem:** The `_meta` envelope needs a `timezone` field. TimeManager knows the configured timezone, but ConfigManager must not `#include` TimeManager (Hollywood Principle, dependency inversion).

**Decision:** Add `ConfigManager::setMetaTimezone(const char* tz)` — an opt-in registration method. TimeManager can call this inside its `applyConfig()` after resolving the timezone string. If nobody calls it, the default is `"UTC"`.

**Why not a direct accessor?** Coupling ConfigManager to TimeManager creates a circular dependency (ConfigManager manages TimeManager's config). The opt-in registration pattern keeps the dependency arrow pointing one way: TimeManager → ConfigManager.

**Current state:** `setMetaTimezone()` is implemented and declared. It is NOT yet called by TimeManager — that is future scope (WO2 or a follow-up). The live backup shows `"timezone": "UTC"` which is the correct safe default.
