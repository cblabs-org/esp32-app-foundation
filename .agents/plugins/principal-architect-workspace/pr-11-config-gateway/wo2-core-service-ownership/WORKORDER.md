---
name: pr-11-config-gateway-wo2
description: Handoff instructions for Work Order 2 — Promote core domain services (WifiManager, otaUpdateManager, TimeManager, KeyManager) to Application-owned members and wire them into the ConfigManager gateway.
---

# WO2: Core Service Ownership — Promote Domain Services to Application Members

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Application currently owns only infrastructure services (ConfigManager, EventBus, DataManager, WebServerManager, McpServer). The core domain services — WifiManager, otaUpdateManager, TimeManager, and KeyManager — exist as modules but are NOT owned by Application. Since every app needs WiFi, OTA, time, and key management, these must be Application-owned members, wired into the boot sequence, and registered with ConfigManager where applicable.

## Context Bootstrap

1. Read `.agents/project_index.md` for project architecture orientation
2. Read `.agents/plugins/principal-architect-workspace/pr-11-config-gateway/PROJECT.md` for project context
3. Load `.agents/skills/house-style-docs/SKILL.md` before writing any code
4. You are on branch `pr-11-config-gateway` in the `esp32-app-foundation` repo

## Strict Boundary Box

- **DO NOT** touch WiFi API endpoints or frontend files (JS/CSS/HTML) — that is P0 scope
- **DO NOT** implement KeyManager integration for WiFi credentials — that is P0 scope
- **DO NOT** touch U8g2DisplayModule — it is genuinely optional/downstream
- **DO NOT** modify ConfigManager itself — it is correct and complete
- **DO NOT** create new branches
- **DO NOT** change any interfaces (iConfigurable, IWebContributor) — they are final

## Current State

### What Application owns now (members in Application.hpp):
```
ConfigManager _configManager;
EventBus _eventBus;
DataManager _dataManager;
WebServerManager _webServerManager;
McpServer _mcpServer;
```

### What's missing — core services that exist as modules but aren't Application members:
| Module | Class | Interfaces | Should be owned? |
|--------|-------|-----------|-----------------|
| `wifiManager` | `WifiManager` | `ITickable`, `iConfigurable`, `IWebContributor` | **YES** — every app needs WiFi |
| `otaUpdateManager` | `otaUpdateManager` | `IWebContributor` | **YES** — every app needs OTA |
| `timeManager` | `TimeManager` | (none currently) | **YES** — every app needs time |
| `keyManager` | `KeyManager` | (none currently) | **YES** — every app needs secure credentials |
| `u8g2Display` | `U8g2DisplayModule` | `ITickable`, `IStatusReporter`, `iConfigurable` | **NO** — optional, display-dependent |

### Important notes about each module:

**WifiManager:**
- Has `setConfigManager(ConfigManager* cm)` setter (DI pattern)
- Has `setEventBus(EventBus* eb)` setter
- Implements `iConfigurable` (namespace "wifi", version 1)
- Implements `IWebContributor` (tab "WiFi")  
- Implements `ITickable`
- `begin()` currently not called — line 88 of Application.cpp says `// TODO: wifiManager.begin(_config.hostname);`

**otaUpdateManager:**
- Has `begin(Application& app)` — takes Application reference
- Implements `IWebContributor` (tab "OTA")
- Does NOT implement `iConfigurable` yet — OTA settings (firmwareUpdatesEnabled, otaQuietHour) are currently in FrameworkConfig. Eventually they should move here, but that's stretch scope for this WO.
- Has a global `extern otaUpdateManager otaManager;` declaration — this global should be REMOVED when Application owns it
- Needs `getParentTab()` → `"System"` and `getTabOrder()` → `30` to match navigation plan

**TimeManager:**
- Simple service: `begin(timezone, ntpServers...)`, `tick()`, `setTimezone()`, `getLocalTime()`, `isTimeSet()`
- Does NOT implement any interfaces yet
- Should implement `ITickable` (it has a `tick()` method)
- Does NOT need `iConfigurable` yet (timezone comes from FrameworkConfig for now)

**KeyManager:**
- Requires `DeviceCrypto*` via `begin(DeviceCrypto*)`
- Does NOT implement any interfaces
- Pure service — no tick, no web routes, no config

## Execution Checklist

### 1. Add core services as Application members

- [ ] **[MODIFY] `modules/application/Application.hpp`**
  Add the four new members to the "Core services" section:
  ```cpp
  // Core services
  ConfigManager _configManager;
  EventBus _eventBus;
  DataManager _dataManager;
  WebServerManager _webServerManager;
  McpServer _mcpServer;
  // Core domain services
  WifiManager _wifiManager;
  otaUpdateManager _otaManager;
  TimeManager _timeManager;
  KeyManager _keyManager;
  ```
  Add the necessary includes (forward declarations preferred where possible, but these modules need full includes since they're owned by value):
  ```cpp
  #include "../wifiManager/wifiManager.hpp"
  #include "../otaUpdateManager/otaUpdateManager.hpp"
  #include "../timeManager/timeManager.hpp"
  #include "../keyManager/keyManager.hpp"
  ```
  Add public accessors following existing pattern:
  ```cpp
  WifiManager& getWifiManager();
  otaUpdateManager& getOtaManager();
  TimeManager& getTimeManager();
  KeyManager& getKeyManager();
  ```

### 2. Wire services in Application::begin()

- [ ] **[MODIFY] `modules/application/Application.cpp`**
  
  Update the boot sequence. The order matters — dependencies must be initialised before dependents:

  **Step 6 — KeyManager + WiFi bring-up (replace the TODO):**
  ```cpp
  // --- Step 6: Core domain services ---
  // KeyManager must init before WiFi (WiFi will need it for credential storage in P0)
  _keyManager.begin(/* DeviceCrypto* — check if Application has one, or create */);
  
  // WifiManager: inject dependencies, register with ConfigManager, then begin
  _wifiManager.setEventBus(&_eventBus);
  _wifiManager.setConfigManager(&_configManager);
  _configManager.registerModule(&_wifiManager);
  _wifiManager.begin(_config.hostname);
  ```
  
  **IMPORTANT about KeyManager:** KeyManager needs a `DeviceCrypto*` pointer. Check if DeviceCrypto is already instantiated somewhere. If Application doesn't have one, you may need to add a `DeviceCrypto _deviceCrypto;` member too, or defer KeyManager init with a comment. Check `keyManager.cpp` to understand the dependency.

  **Step 8 — After ConfigManager.beginAll():**
  ```cpp
  _configManager.beginAll();
  ```
  Note: `_wifiManager` is already registered before this, so `beginAll()` will load its config.

  **Step 9 — Remaining services:**
  ```cpp
  _dataManager.begin();
  _eventBus.begin();
  
  // TimeManager — after WiFi since it needs network for NTP
  _timeManager.begin(_config.timezone);
  
  // OTA — takes Application reference
  _otaManager.begin(*this);
  ```

  **Web contributors — register all core contributors:**
  ```cpp
  _webContributors.push_back(&_wifiManager);
  _webContributors.push_back(&_configManager);
  _webContributors.push_back(&_otaManager);
  _webContributors.push_back(&_mcpServer);
  ```

  **Tickables — register WifiManager and TimeManager:**
  ```cpp
  _tickables.push_back(&_wifiManager);
  // TimeManager doesn't implement ITickable interface, but has tick() — 
  // either add ITickable to TimeManager or call _timeManager.tick() directly in Application::tick()
  ```

  **IMPORTANT about TimeManager tick:** TimeManager has a `tick()` method but does NOT implement `ITickable`. You have two options:
  - **(A)** Make TimeManager implement ITickable and add it to `_tickables` — cleaner
  - **(B)** Call `_timeManager.tick()` directly in `Application::tick()` — simpler
  Choose option (A) for consistency.

### 3. Update otaUpdateManager for navigation hierarchy

- [ ] **[MODIFY] `modules/otaUpdateManager/otaUpdateManager.hpp`**
  Add navigation methods to match the agreed hierarchy (OTA is a sub-tab of System):
  ```cpp
  const char* getParentTab() const override { return "System"; }
  int getTabOrder() const override { return 30; }
  ```

- [ ] **[MODIFY/CHECK] Remove global `extern otaUpdateManager otaManager;`**
  Line 276 of `otaUpdateManager.hpp` declares a global instance. Since Application now owns `_otaManager`, this global is vestigial. Check if anything references `otaManager` globally. If so, update those references to go through Application. If nothing uses it, remove the declaration and any corresponding definition.

### 4. Make TimeManager implement ITickable

- [ ] **[MODIFY] `modules/timeManager/timeManager.hpp`**
  Add ITickable inheritance:
  ```cpp
  #include "../../include/interfaces/ITickable.hpp"
  
  class TimeManager : public ITickable {
  ```
  The existing `void tick()` method already satisfies the interface — just needs the `override` keyword:
  ```cpp
  void tick() override;
  ```

### 5. Update FrameworkConfig

- [ ] **[MODIFY] `include/FrameworkConfig.hpp`**
  FrameworkConfig still exists and contains hostname, timezone, firmwareUpdatesEnabled, otaQuietHour. For now, keep it as a boot-time configuration struct — it provides initial values before ConfigManager loads persisted state. Add a comment explaining this:
  ```cpp
  /**
   * @brief Boot-time framework configuration.
   * 
   * Provides initial/default values for core services before ConfigManager
   * loads persisted state. Once ConfigManager::beginAll() runs, persisted
   * values (if any) override these defaults via applyConfig().
   * 
   * In a downstream app, main.cpp populates this struct and passes it to
   * Application::begin().
   */
  ```

### 6. Update Application docstring

- [ ] **[MODIFY] `modules/application/Application.hpp`**
  Update the begin() docstring to accurately reflect the new boot sequence with all core services.

### 7. Handle DeviceCrypto dependency

- [ ] **[INVESTIGATE]** Check `modules/keyManager/keyManager.cpp` to see what `begin()` needs. Check if `DeviceCrypto` is a class that can be default-constructed or if it needs configuration. If simple, add `DeviceCrypto _deviceCrypto;` to Application. If complex (e.g., needs hardware crypto init), add a TODO comment and defer.

### 8. Update tests

- [ ] **[MODIFY] `test/test_unit/test_application/test_application.cpp`**
  Update tests to account for new members. At minimum:
  - Test that `getWifiManager()` returns a valid reference
  - Test that `getOtaManager()` returns a valid reference
  - Test that `getTimeManager()` returns a valid reference
  - Test that after `begin()`, WiFiManager is registered with ConfigManager

### 9. Clean up main.cpp

- [ ] **[CHECK] `src/main.cpp`**
  If main.cpp references the global `otaManager` or creates its own WifiManager, remove those — Application owns them now. main.cpp should ONLY create `Application app;` and call `app.begin(config)` / `app.tick()`.

## Verification Requirement

1. Run `pio run -e native` — all unit tests must pass
2. Run `pio run -e esp32dev` — firmware must compile for hardware target
3. Run `pio run -e esp32s3nano` — firmware must compile for S3 target
4. Grep: no remaining `extern otaUpdateManager otaManager` references
5. Grep: confirm `_configManager.registerModule(&_wifiManager)` exists in Application.cpp

**CRITICAL:** Once all verification gates pass, commit with message: `feat(app): promote core domain services to Application-owned members [PR-11/WO2]` and send a message back to me with results. Do NOT distill until I explicitly approve.
