---
name: pr-11-config-gateway-wo1b
description: Handoff instructions for Work Order 1b — Fix wiring gaps in Application::begin() for ConfigManager gateway integration.
---

# WO1b: ConfigManager Wiring Fixes

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix 4 wiring gaps discovered during post-WO1 audit. The ConfigManager gateway pattern was built correctly but the pieces were not connected in `Application::begin()`.

## Context Bootstrap

1. Read `.agents/project_index.md` for project architecture orientation
2. You are on branch `pr-11-config-gateway-wo1-interface-refactor` in the `esp32-app-foundation` repo at `/Users/mcneillm/Documents/Projects/esp32-app-foundation`
3. Load `.agents/skills/house-style-docs/SKILL.md` before writing any code

## Strict Boundary Box

- **DO NOT** touch any files outside the scope listed below
- **DO NOT** refactor ConfigManager itself — it is correct
- **DO NOT** touch WiFi API endpoints, frontend JS/CSS, or test files (unless tests break)
- **DO NOT** create new branches or manage git beyond committing

## Gaps to Fix

### Gap 1 (CRITICAL): No `registerModule()` calls in `Application::begin()`

**Problem:** `_configManager.beginAll()` is called at line 98 of `Application.cpp`, but no modules are registered first. The registry is empty, so `beginAll()` does nothing.

**Fix:** Before the `_configManager.beginAll()` call (line 98), add `registerModule()` calls for every `iConfigurable` module that Application owns. You need to discover which members of Application implement `iConfigurable`. Check:
- `_wifiManager` — should be `iConfigurable` (namespace `"wifi"`)
- Look for any display module member (U8g2DisplayModule) — but Application may not own it directly (it's typically registered by downstream apps). Only register modules that Application actually owns as members.

The code should look like:
```cpp
// --- Step 8: ConfigManager — register and load all module configs ---
_configManager.registerModule(&_wifiManager);
// Note: U8g2DisplayModule is registered by downstream apps via getConfigManager()
_configManager.beginAll();
```

**IMPORTANT:** Check `Application.hpp` to see which members actually exist. Do NOT register modules that aren't members of Application. The U8g2DisplayModule is typically owned by the downstream app (e.g., departures-board), not by Application itself.

### Gap 2 (CRITICAL): `setConfigManager()` never called on WifiManager

**Problem:** WifiManager has `void setConfigManager(ConfigManager* cm)` (line 192 of `wifiManager.hpp`) and uses `_configManager->saveModuleConfig(this)` at line 366 of `wifiManager.cpp`. But Application never calls `setConfigManager()`.

**Fix:** In `Application::begin()`, before `_configManager.registerModule(&_wifiManager)`, add:
```cpp
_wifiManager.setConfigManager(&_configManager);
```

This follows the same DI pattern as `setEventBus()`.

### Gap 3 (MEDIUM): `otaUpdateManager` not registered as web contributor

**Problem:** The implementation plan required registering `otaUpdateManager` as a web contributor. Currently only `_configManager` and `_mcpServer` are pushed to `_webContributors` (lines 105-106 of Application.cpp).

**Fix:** Check if Application owns an OTA manager member. If it does, add:
```cpp
_webContributors.push_back(&_otaManager);  // or whatever the member name is
```

If Application does NOT own an OTA manager as a member, skip this and leave a comment:
```cpp
// Note: otaUpdateManager is registered by downstream apps via registerWebContributor()
```

### Gap 4 (MEDIUM): Update docstring for Application::begin()

**Problem:** The docstring at line 86 of `Application.hpp` still says `TODO: wifiManager.begin() (PR-02)`. After the fixes above, step 8 should accurately describe what happens.

**Fix:** Update the step list in the `begin()` docstring to accurately reflect the actual boot sequence after your changes. Remove "TODO" from the WiFi step if WiFi is still deferred (keep it but make the comment accurate).

## Verification Requirement

1. Run `pio run -e native` — all unit tests must pass
2. Run `pio run -e esp32dev` — firmware must compile for hardware target
3. Run `pio run -e esp32s3nano` — firmware must compile for S3 target

**CRITICAL:** Once all verification gates pass, commit with message: `fix(config): wire ConfigManager gateway into Application boot sequence [PR-11/WO1b]` and send a message back to me with results.
