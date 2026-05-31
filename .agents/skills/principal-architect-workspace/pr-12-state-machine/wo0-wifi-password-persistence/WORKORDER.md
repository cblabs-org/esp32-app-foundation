---
name: pr-12-wo0
description: Handoff instructions for Work Order 0 — WiFi Password Persistence
---

# WO0 — WiFi Password Persistence

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Wire WiFi password storage through `KeyManager` so credentials persist across reboots. Currently `WifiManager::serializeConfig()` deliberately excludes the password (there is a `TODO: P0` comment). The password must be stored/retrieved via the existing `KeyManager` AES-256 encryption facility.

## Strict Boundary Box

- ✅ You MAY modify: `modules/wifiManager/wifiManager.hpp`, `modules/wifiManager/wifiManager.cpp`, `modules/application/Application.cpp`
- ❌ You MUST NOT modify: `AppState.hpp`, `IConfigGate.hpp`, `DataManager`, any display code, `ConfigManager`
- ❌ You MUST NOT create new files, rename files, or restructure modules

## Context

- `KeyManager` is owned by `Application` and accessible via `app.getKeyManager()`
- `KeyManager` provides: `addKey(id, value)`, `getKeyById(id)`, `removeKey(id)`
- `WifiManager` already receives `Application*` in its `begin()` method or can be given a `KeyManager*` via dependency injection
- The password is currently stored in `_password` member but never serialised
- Look at how `WifiManager::updateWiFi()` handles the password — that's where `KeyManager::addKey()` should be called
- Look at `WifiManager::applyConfig()` — that's where `KeyManager::getKeyById()` should retrieve the stored password

## Line-Item Checklist

- [ ] Read `modules/wifiManager/wifiManager.hpp` fully to understand the class structure
- [ ] Read `modules/wifiManager/wifiManager.cpp` fully to understand `updateWiFi()`, `applyConfig()`, `serializeConfig()`
- [ ] Read `modules/application/Application.cpp` to understand how WifiManager is initialised
- [ ] Read `modules/keyManager/keyManager.hpp` to understand the KeyManager API
- [ ] Add `KeyManager*` dependency to WifiManager (either via `begin()` parameter or setter)
- [ ] In `updateWiFi()`: after storing password in `_password`, also call `_keyManager->addKey("wifi_password", password)` to persist via encryption
- [ ] In `applyConfig()`: retrieve stored password via `_keyManager->getKeyById("wifi_password")` and set `_password`
- [ ] Remove the `TODO: P0` comment about password persistence
- [ ] In `Application.cpp`: pass `_keyManager` to WifiManager during boot sequence
- [ ] Verify: `pio test -e native --filter test_application` — all existing tests still pass
- [ ] Verify: `pio test -e native --filter test_wifiManager` — if exists, all tests pass

## Verification Requirement

After completing all items, run the native test suite to verify no regressions. Then **STOP and send a message back to the Principal Architect** reporting:
1. What files were modified
2. Whether tests pass
3. Any issues or decisions made

Do NOT wrap up, distill, or commit until the Principal Architect explicitly approves.
