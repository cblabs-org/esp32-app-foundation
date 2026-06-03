---
name: pr-16-wo1
description: Handoff instructions for Work Order 1 — Firmware changes for backup system overhaul
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the ESP32 backup system. The root bug is that `/api/config/backup` is registered after `/api/config` in ESPAsyncWebServer — prefix matching causes `/api/config` to swallow all sub-path requests, so the backup and restore handlers have never fired. Also extend `OTAUpdateManager` to implement `IConfigurable` so OTA config is backed up, and add a `_meta` envelope to the backup output.

## Strict Boundary Box

- **Only touch these files:**
  - `modules/ConfigManager/ConfigManager.cpp`
  - `modules/ConfigManager/ConfigManager.hpp`
  - `modules/OTAUpdateManager/OTAUpdateManager.hpp`
  - `modules/OTAUpdateManager/OTAUpdateManager.cpp`
  - `modules/Application/Application.cpp`
- Do NOT touch any web portal `.js` files — that is WO2 scope
- Do NOT touch any test files — that is WO3 scope
- Do NOT modify any other modules
- Do NOT create new files outside the above list

## House Style Requirements

- Follow the project's documented house style for all code (see `SKILL.md` for house-style-docs skill)
- All new methods MUST have full Doxygen docstrings matching existing module patterns
- Preserve all existing comments and docstrings that are unrelated to your changes

## Checklist

### 1. `modules/ConfigManager/ConfigManager.cpp` — Route Registration Order

- [ ] In `registerRoutes()`, reorder so specific sub-paths are registered BEFORE the generic parent:
  ```
  1. /api/config/backup   (HTTP_GET)
  2. /api/config/restore  (HTTP_POST)
  3. /api/config          (HTTP_GET)   ← must be last
  ```

### 2. `modules/ConfigManager/ConfigManager.cpp` — Streaming Response

- [ ] In the `/api/config/backup` handler, replace the intermediate `String` buffer approach with `AsyncResponseStream`:
  ```cpp
  AsyncResponseStream* stream = request->beginResponseStream("application/json");
  stream->addHeader("Content-Disposition", "attachment; filename=\"config-backup.json\"");
  serializeJson(doc, *stream);
  request->send(stream);
  ```

### 3. `modules/ConfigManager/ConfigManager.cpp` + `.hpp` — `_meta` Envelope

- [ ] Add a private helper method `buildBackupMeta(JsonObject& meta)` to `ConfigManager` that populates:
  - `backup_version`: hardcoded `1`
  - `firmware_version`: from `FIRMWARE_VERSION` build define (use `"unknown"` if not defined)
  - `device_id`: chip MAC address — use `WiFi.macAddress()` on hardware, `"00:00:00:00:00:00"` on NATIVE_BUILD
  - `device_type`: chip model — use `ESP.getChipModel()` on hardware, `"native"` on NATIVE_BUILD
  - `datetime`: ISO-8601 string from `time()` if available (check `time(nullptr) > 1000000000`), else `"unknown"`
  - `timezone`: read from the `_meta_timezone` member (populated from TimeManager — see note below), else `"UTC"`
  - `module_count`: `_moduleCount`
- [ ] In `exportAllConfig()`, add `_meta` as the first key in the output document, populated by `buildBackupMeta()`

  > **Note on timezone**: The ConfigManager does not have a direct reference to TimeManager. The simplest approach is to add a `void setMetaTimezone(const char* tz)` method that any module (TimeManager's `applyConfig()`) can call on the ConfigManager to register the TZ string. For now, if this is too complex, default to `"UTC"` with a TODO comment — do NOT couple ConfigManager to TimeManager directly.

- [ ] In `importAllConfig()`, add an explicit log when a key is not matched to any module (e.g. `_meta`) — already skipped by the existing loop logic, but add: `ESP_LOGD(TAG, "Skipping unknown key '%s' during import", ns);`

- [ ] Update `ConfigManager.hpp` docstrings for `exportAllConfig()` and `importAllConfig()` to document the `_meta` key behaviour

### 4. `modules/OTAUpdateManager/OTAUpdateManager.hpp` — Add IConfigurable

- [ ] Add `#include "../../include/interfaces/IConfigurable.hpp"` (guarded with `#ifndef NATIVE_BUILD` / `#else` only if needed — check if the interface is already native-compatible)
- [ ] Change class declaration: `class OTAUpdateManager : public IWebContributor, public IConfigurable`
- [ ] Add private config member fields:
  ```cpp
  char _channel[16] = "stable";   ///< OTA release channel ("stable" or "beta")
  bool _autoCheck   = true;       ///< Whether to auto-check for updates on boot
  ```
- [ ] Declare the five IConfigurable overrides in the public section (with Doxygen docstrings):
  - `const char* getConfigNamespace() const override;`
  - `int getSchemaVersion() const override;`
  - `void applyConfig(const JsonObject& section) override;`
  - `void serializeConfig(JsonObject& section) const override;`
  - `bool migrateConfig(JsonObject& section, int fromVersion) override;` (optional — return true)
  - `void resetConfig() override;`
- [ ] Update the class header docstring to list the new IConfigurable methods

### 5. `modules/OTAUpdateManager/OTAUpdateManager.cpp` — Implement IConfigurable

- [ ] Implement `getConfigNamespace()` → return `"ota"`
- [ ] Implement `getSchemaVersion()` → return `1`
- [ ] Implement `serializeConfig()`:
  ```cpp
  section["channel"]   = _channel;
  section["autoCheck"] = _autoCheck;
  ```
- [ ] Implement `applyConfig()`:
  ```cpp
  const char* ch = section["channel"] | "stable";
  strlcpy(_channel, ch, sizeof(_channel));
  _autoCheck = section["autoCheck"] | true;
  ```
- [ ] Implement `resetConfig()`:
  ```cpp
  strlcpy(_channel, "stable", sizeof(_channel));
  _autoCheck = true;
  ```
- [ ] Implement `migrateConfig()` → log migration attempt, return `true`

### 6. `modules/Application/Application.cpp` — Register OTAUpdateManager

- [ ] Add `_configManager.registerModule(&_otaUpdateManager);` in the registration block alongside `_wifiManager`, `_mcpServer`, `_timeManager`
- [ ] Ensure OTA is registered BEFORE `beginAll()` is called

## Verification Requirement

After implementing all changes:

1. Run `pio run -e esp32s3` — firmware MUST build with zero errors and zero new warnings
2. Run `pio test -e native` — all existing tests MUST pass
3. Read through your changes once more to confirm:
   - Route order: backup → restore → config (specific before generic)
   - `_meta` appears as first key in backup output
   - OTAUpdateManager implements all 5 IConfigurable methods
   - OTAUpdateManager is registered in Application.cpp before `beginAll()`

Once all checks pass, **STOP and send a message back to the Principal Architect** reporting:
- Build result
- Test result
- Any issues or surprises encountered
- Whether the `datetime`/`timezone` approach was simplified (and how)

Do NOT distill or commit until the Principal Architect explicitly approves.

---

## Status: COMPLETE ✅

- **Completed:** 2026-06-03
- **Commit:** `4c67b43` on branch `pr-16-backup-overhaul-wo1-firmware`
- **Branch:** `pr-16-backup-overhaul`
- **Build:** esp32s3nano — SUCCESS (Flash 58.5%, RAM 15.5%)
- **Flash:** SUCCESS — device at 192.168.20.186
- **Live endpoint:** `GET /api/config/backup` returns `_meta` envelope + 5 module sections (display, wifi, mcp, time, ota)

### Simplification notes

- **datetime/timezone:** `datetime` field uses `time()` + ISO-8601 format; `timezone` defaults to `"UTC"` via a TODO comment. `setMetaTimezone()` is wired and ready for TimeManager to call — no direct coupling introduced.
- **OTA allocation order:** Heap-allocated OTAUpdateManager moved from Step 13 → Step 7 (before `beginAll()`); `begin(*this)` still fires at Step 13 as before.
- **Native tests:** All 17 pre-existing ERRORs (x86_64/arm64 GoogleTest mismatch) — zero regressions from WO1 changes (confirmed via `git stash` baseline run).
