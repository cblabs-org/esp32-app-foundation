---
name: pr-14-wo5-config-lifecycle
description: Handoff instructions for Work Order 5 — Three-Tier Config Lifecycle
---

# WO5 — Three-Tier Config Lifecycle

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Implement a Three-Tier config lifecycle (Active + Previous + Golden) managed by a `ConfigManifest`. The manifest tracks which config file versions are in each slot per module namespace. This enables:
- **Golden**: the last-known-good config (snapshotted when firmware is blessed)
- **Previous**: the config from before the last OTA (for rollback recovery)
- **Active**: the currently running config
- **Pruning**: orphaned config files not referenced by any slot are deleted

## Strict Boundary Box

- ✅ You MAY create new files in `lib/ConfigManifest/`
- ✅ You MAY create test files in `test/test_unit/`
- ✅ You MAY modify `modules/ConfigManager/ConfigManager.hpp` and `.cpp`
- ✅ You MAY modify `modules/OTAUpdateManager/OTAUpdateManager.cpp` (snapshot calls only)
- ❌ Do NOT modify `modules/Application/` — completed in WO2
- ❌ Do NOT modify `lib/OTADownloadEngine/` — completed in WO2
- ❌ Do NOT modify `lib/OTAManifest/` — completed in WO3
- ❌ Do NOT modify `lib/GithubClient/` — completed in WO4
- ❌ Do NOT modify `lib/SemVer/` — completed in WO1
- ❌ Do NOT modify `web/portal/` — that's WO6
- ❌ Do NOT create branches or manage git

## Project House Style

- PascalCase for files/types, standard license header
- Doxygen comments on all public methods
- `#pragma once`, `#ifndef NATIVE_BUILD` guards
- Prefer `char[]` over Arduino `String`

## Context: Current Config System

**ConfigManager** stores per-module config at `/mod/<namespace>.json`. Each file has a `_v` field for schema version. The registry is a `std::vector<IConfigurable*>` that is locked after `beginAll()`.

The config manifest is a **separate file** (`/config_manifest.json`) that tracks the lifecycle slots. It does NOT replace the per-module config files — it operates alongside them.

**Key integration points:**
- `ConfigManager::beginAll()` — after loading all modules, update the Active slot
- `ConfigManager::saveModuleConfig()` — if firmware is blessed, also update Golden
- `OTAUpdateManager::performUpdateTask()` — before downloading, snapshot Active → Previous
- `OTAUpdateManager::markAppValid()` — after blessing, snapshot Active → Golden

## Implementation Checklist

### 1. Create `lib/ConfigManifest/ConfigManifest.hpp`

- [ ] Standard license header, `#pragma once`
- [ ] Constants:
  ```cpp
  static constexpr size_t kMaxModules = 16;
  static constexpr size_t kMaxNamespaceLen = 24;
  ```
- [ ] `struct ModuleSlot`:
  ```cpp
  struct ModuleSlot {
      char ns[kMaxNamespaceLen];   ///< Module namespace (e.g., "ota", "wifi")
      int activeVersion;           ///< Schema version of the Active config
      int previousVersion;         ///< Schema version before last OTA
      int goldenVersion;           ///< Schema version of last-known-good config
  };
  ```
- [ ] `class ConfigManifest`:
  ```cpp
  class ConfigManifest {
  public:
      bool load();                  ///< Load from /config_manifest.json
      bool save() const;            ///< Save to /config_manifest.json
      
      void updateActive(const char* ns, int schemaVersion);
      void updatePrevious(const char* ns, int schemaVersion);
      void updateGolden(const char* ns, int schemaVersion);
      
      const ModuleSlot* getSlot(const char* ns) const;
      
      int getActiveVersion(const char* ns) const;
      int getPreviousVersion(const char* ns) const;
      int getGoldenVersion(const char* ns) const;
      
      void snapshotActiveToPrevious();  ///< Copy all Active → Previous
      void snapshotActiveToGolden();    ///< Copy all Active → Golden
      
  private:
      ModuleSlot _slots[kMaxModules] = {};
      uint8_t _slotCount = 0;
      
      ModuleSlot* findOrCreate(const char* ns);
  };
  ```
- [ ] Note: file I/O uses LittleFS on hardware, file stubs on native. Use `#ifndef NATIVE_BUILD` guards in the `.cpp`.

### 2. Create `lib/ConfigManifest/ConfigManifest.cpp`

- [ ] `#include <ArduinoJson.h>` for JSON serialization
- [ ] Implement `load()`:
  - On hardware: open `/config_manifest.json` with LittleFS
  - On native: try to open `config_manifest.json` in working directory (or return `false` gracefully if not found — first boot scenario)
  - Parse JSON: iterate `"modules"` array, populate `_slots[]` with `ns`, `activeVersion`, `previousVersion`, `goldenVersion`
  - Return `true` on success
- [ ] Implement `save()`:
  - Serialize `_slots[0..slotCount-1]` to JSON under `"modules"` array
  - Write to `/config_manifest.json`
- [ ] Implement `findOrCreate()`:
  - Linear search `_slots[]` by `ns` using `strcmp()`
  - If not found and `_slotCount < kMaxModules`, create new slot with zeroed versions
  - Return pointer or nullptr if full
- [ ] Implement `updateActive/Previous/Golden()`:
  - Call `findOrCreate(ns)`, set the corresponding version field
- [ ] Implement `getSlot()` / `getActiveVersion()` etc.:
  - Linear search, return value or -1 if not found
- [ ] Implement `snapshotActiveToPrevious()` / `snapshotActiveToGolden()`:
  - Iterate all slots, copy `activeVersion → previousVersion` (or `goldenVersion`)

### 3. Create `test/test_unit/test_config_manifest/test_config_manifest.cpp`

- [ ] GoogleTest suite `ConfigManifestTest`
- [ ] Test: `updateActive("ota", 2)` then `getActiveVersion("ota")` returns 2
- [ ] Test: `updateGolden("wifi", 1)` then `getGoldenVersion("wifi")` returns 1
- [ ] Test: `getActiveVersion("unknown")` returns -1
- [ ] Test: `snapshotActiveToPrevious()` copies all active to previous
- [ ] Test: `snapshotActiveToGolden()` copies all active to golden
- [ ] Test: `findOrCreate` with kMaxModules exceeded returns nullptr gracefully
- [ ] Test: multiple modules tracked independently
- [ ] Test: update same namespace multiple times — latest value retained

### 4. Integrate with ConfigManager

- [ ] In `ConfigManager.hpp`, add:
  ```cpp
  #include "../../lib/ConfigManifest/ConfigManifest.hpp"
  ```
  And a private member:
  ```cpp
  ConfigManifest _manifest;
  ```

- [ ] In `ConfigManager.cpp`, in `beginAll()`:
  - After loading all module configs, call `_manifest.load()`
  - After each module's `applyConfig()` succeeds, call:
    ```cpp
    _manifest.updateActive(module->getConfigNamespace(), module->getSchemaVersion());
    ```
  - After all modules loaded, call `_manifest.save()`

- [ ] Add a public method:
  ```cpp
  ConfigManifest& getManifest() { return _manifest; }
  ```
  This lets OTAUpdateManager access the manifest for snapshotting.

### 5. Integrate with OTAUpdateManager

- [ ] In `OTAUpdateManager.cpp`, in `performUpdateTask()`:
  - **Before** the download starts (after all guards pass, before the WiFiClientSecure creation), call:
    ```cpp
    app_->getConfigManager().getManifest().snapshotActiveToPrevious();
    app_->getConfigManager().getManifest().save();
    ESP_LOGI(TAG, "Config manifest: Active → Previous snapshot saved");
    ```

- [ ] In `OTAUpdateManager.cpp`, in `markAppValid()`:
  - **After** `esp_ota_mark_app_valid_cancel_rollback()`, call:
    ```cpp
    app_->getConfigManager().getManifest().snapshotActiveToGolden();
    app_->getConfigManager().getManifest().save();
    ESP_LOGI(TAG, "Config manifest: Active → Golden snapshot saved (firmware blessed)");
    ```

## Verification

- [ ] `pio test -e native -f test_config_manifest` — all manifest tests pass
- [ ] `pio test -e native` — full suite passes (zero regressions)
- [ ] `pio run -e esp32dev` — hardware build succeeds
- [ ] `pio run -e esp32s3nano` — hardware build succeeds

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All tests pass
2. Both hardware builds succeed
3. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.
