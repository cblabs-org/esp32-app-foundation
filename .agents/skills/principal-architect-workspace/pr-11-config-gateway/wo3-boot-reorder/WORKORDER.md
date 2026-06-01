---
name: pr-11-config-gateway-wo3
description: Handoff instructions for Work Order 3 — Reorder Application::begin() boot sequence to respect the dependency tree.
---

# WO3: Boot Sequence Reordering

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Reorder `Application::begin()` so that services initialise in dependency order. The current boot sequence has two critical ordering bugs:
1. **WifiManager.begin() is called BEFORE ConfigManager.beginAll()** — WiFi starts with no persisted config (SSID, hostname)
2. **EventBus.begin() is called late** — services can't publish boot-time events

## Context Bootstrap

1. Read `.agents/project_index.md` for project architecture orientation
2. Load `.agents/skills/house-style-docs/SKILL.md` before writing any code
3. You are on branch `pr-11-config-gateway-wo3-boot-reorder` in the `esp32-app-foundation` repo

## Strict Boundary Box

- **DO NOT** change any interfaces, class members, or public APIs
- **DO NOT** add new features or services
- **DO NOT** modify any files other than `Application.cpp` and `Application.hpp` (docstring only)
- **ONLY** reorder the existing calls in `Application::begin()`

## Required Boot Order

The dependency tree dictates this exact order:

```
EventBus           ← no deps, pure in-memory — enables boot-time event publishing
LittleFS           ← foundation for all persistence
FrameworkConfig    ← store boot-time defaults
DeviceCrypto       ← needs filesystem for key material
KeyManager         ← needs DeviceCrypto
ConfigManager      ← register modules, then beginAll() loads /mod/*.json
  ↓ (all persisted config now loaded)
WifiManager.begin  ← has its SSID/hostname from ConfigManager
TimeManager.begin  ← needs network for NTP (after WiFi)
DataManager.begin  ← background data workers
OTA.begin          ← needs network + time
WebServer + MCP    ← all routes registered, all contributors ready
Lock               ← no further registrations
```

## Execution Checklist

- [ ] **[MODIFY] `modules/application/Application.cpp` — Reorder `begin()`**

  The new `begin()` should follow this exact step numbering:

  ```
  Step 1:  Boot banner — ESP_LOGI with APP_FOUNDATION_VERSION
  Step 2:  State → BOOTING
  Step 3:  EventBus.begin() — FIRST service to init. Zero deps. Enables boot-time event publishing.
  Step 4:  Mount LittleFS (with NATIVE_BUILD guard)
  Step 5:  Store FrameworkConfig internally
  Step 6:  DeviceCrypto.begin() + KeyManager.begin(&_deviceCrypto)
  Step 7:  ConfigManager — register all framework iConfigurable modules:
           _configManager.registerModule(&_wifiManager);
           (downstream modules are registered by app code before begin() is called)
  Step 8:  ConfigManager.beginAll() — loads all persisted config from /mod/*.json
           ALL persisted config is now available before any service starts.
  Step 9:  State → WIFI_SETUP
  Step 10: WifiManager.begin(hostname) — now has persisted SSID/hostname from step 8
  Step 11: State → APP_SETUP
  Step 12: TimeManager.begin(timezone) — after WiFi, needs network for NTP
  Step 13: DataManager.begin()
  Step 14: OTA — heap-allocate and begin(*this)
  Step 15: Register core web contributors (WiFi, ConfigManager, OTA, MCP) and tickables (WiFi, TimeManager)
  Step 16: WebServerManager.begin() + McpServer.begin()
  Step 17: _compactAndLock()
  Step 18: Publish system.heap event + _reportHeap()
  Step 19: State → RUNNING
  ```

  **IMPORTANT notes:**
  - Move `_eventBus.begin()` from its current position (step 9) to immediately after BOOTING state (new step 3)
  - Move `_configManager.registerModule(&_wifiManager)` and `_configManager.beginAll()` to BEFORE `_wifiManager.begin()` — this is the critical fix
  - `_wifiManager.setConfigManager(&_configManager)` must stay before `registerModule` (DI injection)
  - Keep the existing `_wifiManager.setEventBus(&_eventBus)` call if it exists — it should be before `_wifiManager.begin()` but after `_eventBus.begin()` now
  - Keep all existing NATIVE_BUILD guards exactly as they are
  - Keep all existing ESP_LOGI/ESP_LOGE log lines — adjust step numbers in comments
  - Preserve the heap event publish and _reportHeap() call at the end

- [ ] **[MODIFY] `modules/application/Application.hpp` — Update docstring**
  
  Update the `begin()` docstring step list to match the new ordering. This is the comment block above `bool begin(const FrameworkConfig& config)`.

## Verification Requirement

1. Run `pio run -e native` — all unit tests must pass
2. Run `pio run -e esp32dev` — firmware must compile
3. Run `pio run -e esp32s3nano` — firmware must compile

**CRITICAL:** Once all verification gates pass, commit with message: `fix(app): reorder boot sequence — EventBus first, config before WiFi [PR-11/WO3]` and send a message back to me with results. Do NOT distill until I explicitly approve.
