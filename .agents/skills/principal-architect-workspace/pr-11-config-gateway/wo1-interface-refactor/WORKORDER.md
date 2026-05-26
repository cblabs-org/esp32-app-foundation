---
name: pr-11-config-gateway-wo1
description: Handoff instructions for Work Order 1 — iConfigurable interface redesign, ConfigManager gateway refactoring, IWebContributor navigation hierarchy, and all implementor migrations.
---

# WO1: ConfigManager-as-Gateway — Interface Refactor & Migration

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Refactor `ConfigManager` from a passive `SystemConfig` struct holder into the single gateway for all configuration file I/O, with self-describing schema versioning, migration orchestration, and backup/restore coordination. Update all implementors to the new `iConfigurable` interface. Add navigation hierarchy to `IWebContributor`.

## Context Bootstrap

1. Read `.agents/project_index.md` for project architecture orientation
2. Read `.agents/skills/principal-architect-workspace/pr-11-config-gateway/PROJECT.md` for project context
3. You are on branch `pr-11-config-gateway-wo1-interface-refactor`

## Strict Boundary Box

- **DO NOT** touch WiFi API endpoints (P0 scope — separate WO)
- **DO NOT** touch `wifi.js`, `system.js`, or `ota.js` frontend files
- **DO NOT** touch `app.css` or any CSS files
- **DO NOT** implement backup/restore API routes yet (just the C++ `exportAllConfig()`/`importAllConfig()` methods)
- **DO NOT** implement the server-side heap guard (P1 scope)
- **DO NOT** create new branches or manage git beyond committing your work
- **DO** load `.agents/skills/embedded-systems/SKILL.md` and `.agents/skills/house-style-docs/SKILL.md` before writing any code

## Architectural Decisions (Pre-Resolved)

These decisions were made in the design session and are **NOT open for re-evaluation**:

### Storage Model
- **Hybrid multi-file**: Each module has its own file at `/mod/<namespace>.json`
- ConfigManager owns ALL file I/O — modules NEVER touch LittleFS for config
- Modules provide `serializeConfig()` / `applyConfig()` callbacks

### Version Tracking
- Each config file contains a `_v` integer field (self-describing, no separate meta file)
- Version is the **schema version** (code-level), NOT a save counter
- On boot, ConfigManager compares stored `_v` vs `module.getSchemaVersion()`
- If stored < declared → call `module.migrateConfig(section, storedVersion)` → rewrite file

### Memory Constraints (Per Embedded Review)
- Registry: Fixed-size `iConfigurable* _modules[MAX_CONFIGURABLES]` array (MAX=8). No `std::vector`, no `std::map`
- Namespace IDs: `const char*` compile-time literals. No heap strings.
- Per-module save: ~200-500 byte JsonDocument. Serialize directly to `File` stream (no intermediate `String`)
- Export (backup): master JsonDocument ~2-3KB heap. Only during user-initiated backup. Acceptable.

### Domain Ownership Changes
- **Remove `SystemConfig` struct** from ConfigManager
- **Hostname** → moves to WifiManager (namespace `"wifi"`)
- **Timezone** → should move to TimeManager but TimeManager may not implement iConfigurable yet. If so, leave timezone in WifiManager for now with a `// TODO: move to TimeManager when it implements iConfigurable` comment.
- **OTA settings** (`firmwareUpdatesEnabled`, `otaQuietHour`) → move to OtaUpdateManager (namespace `"ota"`)
- **Remove `FrameworkConfig` struct** — it was a pass-through adapter for settings that now live in individual modules

### Navigation Hierarchy
- `IWebContributor` gets `getParentTab()` and `getTabOrder()` virtual methods
- `getParentTab() = nullptr` → top-level tab (bottom nav)
- `getParentTab() = "System"` → sub-section under System tab
- `getTabOrder()` → integer ordering within same level (lower = first)

## Execution Checklist

### 1. Interface Files

- [ ] **[MODIFY] `include/interfaces/iConfigurable.hpp`**
  Replace the current 3-method interface with the 6-method version:
  ```cpp
  class iConfigurable {
  public:
      virtual ~iConfigurable() = default;
      virtual const char* getConfigNamespace() const = 0;
      virtual int getSchemaVersion() const = 0;
      virtual void applyConfig(const JsonObject& section) = 0;
      virtual void serializeConfig(JsonObject& section) const = 0;
      virtual bool migrateConfig(JsonObject& section, int fromVersion) { return true; }
      virtual void resetConfig() = 0;
  };
  ```
  Include `<ArduinoJson.h>` for `JsonObject`. Keep existing header comment style. Update the docstring.

- [ ] **[MODIFY] `include/interfaces/IWebContributor.hpp`**
  Add two new virtual methods with defaults:
  ```cpp
  virtual const char* getParentTab() const { return nullptr; }
  virtual int getTabOrder() const { return 100; }
  ```
  Update docstring to explain the navigation model.

### 2. ConfigManager Refactor

- [ ] **[MODIFY] `modules/configManager/configManager.hpp`**
  - Remove `SystemConfig` struct entirely
  - Remove `FrameworkConfig getFrameworkConfig()` method
  - Remove `_config` member and `getConfig()`/`getMutableConfig()` accessors
  - Add `static constexpr uint8_t MAX_CONFIGURABLES = 8;`
  - Add `iConfigurable* _modules[MAX_CONFIGURABLES] = {};` and `uint8_t _moduleCount = 0;`
  - Add public methods: `registerModule()`, `beginAll()`, `saveModuleConfig()`, `exportAllConfig()`, `importAllConfig()`
  - Add private: `loadModuleConfig()`, `static getConfigPath()`
  - Keep `IWebContributor` implementation but update `getParentTab()` → `"System"`, `getTabOrder()` → `25`
  - Keep `requestReload()` / `checkAndClearReload()` mechanism
  - Remove `#include "FrameworkConfig.hpp"` — delete or repurpose `FrameworkConfig.hpp`

- [ ] **[MODIFY] `modules/configManager/configManager.cpp`**
  Complete rewrite of implementation:
  - `getConfigPath(ns, buf, size)`: formats `/mod/<namespace>.json` (hardware) or `mod_<namespace>.json` (NATIVE_BUILD)
  - `registerModule(module)`: bounds-check `_moduleCount < MAX_CONFIGURABLES`, store pointer
  - `beginAll()`: iterate `_modules[]`, call `loadModuleConfig()` on each
  - `loadModuleConfig(module)`: open file → deserialize → read `_v` (default 0 if missing) → compare with `getSchemaVersion()` → call `migrateConfig()` if needed → call `applyConfig()` → rewrite file if migrated
  - `saveModuleConfig(module)`: create JsonDocument → call `serializeConfig()` → set `doc["_v"] = module->getSchemaVersion()` → serialize directly to File stream (not String). **CRITICAL: use `serializeJson(doc, file)` not `String`**
  - `exportAllConfig(outDoc)`: iterate modules, for each create a nested object keyed by namespace, call `serializeConfig()`, add `_v`
  - `importAllConfig(inDoc)`: iterate modules, extract their namespace section, validate, call `applyConfig()`, call `saveModuleConfig()`
  - `registerRoutes()`: keep existing `/api/config` GET/POST but update to work without SystemConfig (ConfigManager no longer owns any domain settings — GET returns list of registered namespaces, POST is reserved for future use)
  - Preserve NATIVE_BUILD guards for testing

### 3. Module Migrations

- [ ] **[MODIFY] `modules/wifiManager/wifiManager.hpp`**
  - Remove `iConfigurable` old methods (`loadConfig()`, `saveConfig()`)
  - Add new interface methods: `getConfigNamespace()` → `"wifi"`, `getSchemaVersion()` → `1`
  - Add `applyConfig()`, `serializeConfig()` declarations
  - Keep `resetConfig()` (still part of interface)
  - Add hostname storage: wifi already has `char currentHostname[33]` — this becomes part of the WiFi config section
  - Add `IWebContributor` navigation: `getTabOrder()` → `10` (WiFi is primary/first tab)

- [ ] **[MODIFY] `modules/wifiManager/wifiManager.cpp`**
  - Remove `loadConfig()` implementation (direct LittleFS read of `/mod/wifiManager.json`)
  - Remove `saveConfig()` implementation (direct LittleFS write)
  - Add `applyConfig(const JsonObject& section)`: read `ssid` and `hostname` from section into member variables. Password is NOT in config (comes from KeyManager — leave a `// TODO: P0 — retrieve password from KeyManager` comment for now; do NOT implement KeyManager integration, that's P0 scope)
  - Add `serializeConfig(JsonObject& section) const`: write `ssid` and `hostname` to section
  - In `updateWiFi()`: instead of calling `saveConfig()`, need to get ConfigManager to save. For now, accept a ConfigManager pointer (add `void setConfigManager(ConfigManager* cm)` setter + `ConfigManager* _configManager = nullptr` member), then call `_configManager->saveModuleConfig(this)`. Forward-declare ConfigManager.
  - In `begin()`: remove the `loadConfig()` call — ConfigManager will call `applyConfig()` during `beginAll()`

- [ ] **[MODIFY] `modules/u8g2Display/u8g2DisplayModule.hpp`**
  - Same pattern: add `getConfigNamespace()` → `"display"`, `getSchemaVersion()` → `1`
  - Add `applyConfig()`, `serializeConfig()` declarations

- [ ] **[MODIFY] `modules/u8g2Display/u8g2DisplayModule.cpp`**
  - Remove direct LittleFS file I/O from `loadConfig()` and `saveConfig()`
  - Implement `applyConfig()` / `serializeConfig()` to read/write the same fields the old loadConfig/saveConfig used
  - Keep the existing config fields (brightness, flip, etc.)

### 4. Application Boot Sequence

- [ ] **[MODIFY] `modules/application/Application.hpp`**
  - Remove `_configurables` vector (`std::vector<iConfigurable*>`)
  - Remove `registerConfigurable()` method
  - Add `ConfigManager _configManager;` member (Application owns it)
  - Add `ConfigManager& getConfigManager()` accessor
  - Remove `FrameworkConfig _config` member if it was only used for pass-through
  - Add `_webContributors.push_back(&_configManager)` in begin()
  - Ensure `_otaManager` is also pushed to `_webContributors` if not already

- [ ] **[MODIFY] `modules/application/Application.cpp`**
  - In `begin()`, replace Step 8 (the `_configurables` loop calling `loadConfig()`) with:
    ```cpp
    _configManager.registerModule(&_wifiManager);
    // Register other iConfigurable modules...
    _configManager.beginAll();
    ```
  - Register ConfigManager as web contributor: `_webContributors.push_back(&_configManager);`
  - Wire WifiManager's ConfigManager pointer: `_wifiManager.setConfigManager(&_configManager);`
  - Remove `_compactAndLock()` call on `_configurables` (vector no longer exists)
  - Update `_compactAndLock()` to remove `_configurables.shrink_to_fit()` line
  - Update the log line in `_compactAndLock()` to remove `configurables=%u`
  - If `FrameworkConfig` is still needed for hostname/timezone pass-through during boot, keep a minimal version. If not, remove it.

- [ ] **[MODIFY/DELETE] `include/FrameworkConfig.hpp`**
  - If hostname/apName are now fully owned by WifiManager config, and timezone by TimeManager, this struct may be vestigial. Evaluate whether Application::begin() still needs it as a parameter. If so, keep a minimal version with just `hostname`, `apName`, `timezone` as boot-time overrides. If not, delete.

### 5. WebServerManager Tab Response

- [ ] **[MODIFY] `modules/webServer/webServer.cpp`**
  - Update the `/api/tabs` endpoint to include `parent` and `order` fields from each contributor:
    ```json
    [{"id": "wifi", "name": "WiFi", "parent": null, "order": 10}, ...]
    ```
  - Generate `id` from tab name (lowercase, spaces → hyphens)

### 6. Unit Tests

- [ ] **[MODIFY] `test/test_unit/test_application/test_application.cpp`**
  - Update `MockConfigurable` to implement the new 6-method `iConfigurable` interface
  - Update tests that call `registerConfigurable()` to use `_configManager.registerModule()` pattern
  - Add test: `beginAll()` calls `applyConfig()` on all registered modules
  - Add test: `saveModuleConfig()` calls `serializeConfig()` and writes file
  - Add test: version mismatch triggers `migrateConfig()`

- [ ] **[MODIFY] `test/test_unit/test_u8g2Display/test_config.cpp`**
  - Update to test via new `applyConfig()` / `serializeConfig()` interface

### 7. Documentation

- [ ] Update header docstrings in all modified files following house-style conventions
- [ ] Ensure all new public methods have `@brief`, `@param`, `@return` documentation

## Verification Requirement

1. Run `pio run -e native` — all unit tests must pass
2. Run `pio run -e esp32dev` — firmware must compile for hardware target
3. Run `pio run -e esp32s3nano` — firmware must compile for S3 target
4. Verify no direct LittleFS access remains in any `iConfigurable` implementor (grep for `LittleFS.open` in module config methods)

**CRITICAL:** Before distilling or committing, you MUST send a message back to the Principal Architect reporting success/failure and any issues encountered. Only after explicit approval should you invoke the `@distillery` skill.
