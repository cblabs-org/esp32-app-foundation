---
name: pr-11-config-gateway-wo4
description: Handoff instructions for Work Order 4 — Add IStatusReporter boot progress calls to Application::begin().
---

# WO4: Boot Progress via IStatusReporter

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add `_statusReporter->reportStatus()` calls at each milestone in `Application::begin()` so that an optional display module (registered before boot via `registerStatusReporter()`) can show real-time boot progress. When no display is registered, the existing `NullStatusReporter` absorbs all calls at zero cost.

## Context Bootstrap

1. Read `.agents/project_index.md` for project architecture orientation
2. Load `.agents/skills/house-style-docs/SKILL.md` before writing any code
3. You are on branch `pr-11-config-gateway-wo4-boot-progress` in the `esp32-app-foundation` repo

## Strict Boundary Box

- **DO NOT** change any interfaces (IStatusReporter, iConfigurable, IWebContributor)
- **DO NOT** modify any module other than Application
- **DO NOT** add new class members or public methods
- **DO NOT** touch display modules, WiFi, OTA, or test files
- **ONLY** add `_statusReporter->reportStatus()` and `_statusReporter->clearStatus()` calls to the existing `Application::begin()` method, and update the docstrings.

## Current State

`Application::begin()` already has:
- `_statusReporter` pointer (defaults to `&_nullReporter`, a `NullStatusReporter`)
- `registerStatusReporter(IStatusReporter* reporter)` public method
- Downstream apps can call `app.registerStatusReporter(&display)` BEFORE `app.begin()`

The `IStatusReporter` interface:
```cpp
class IStatusReporter {
public:
    virtual void reportStatus(const char* message, float progress = -1.0f) = 0;
    virtual void reportError(const char* errorMessage) = 0;
    virtual void clearStatus() = 0;
};
```

## Execution Checklist

### 1. Add boot progress calls to Application::begin()

- [ ] **[MODIFY] `modules/application/Application.cpp`**

  Add `_statusReporter->reportStatus()` calls at each boot milestone. Use progress values from 0.0 to 1.0 that reflect meaningful phases. The messages should be short, human-readable, and informative.

  Insert these calls at the following points in the existing boot sequence (do NOT reorder anything — the order from WO3 is correct):

  ```
  After Step 2 (State → BOOTING):
    _statusReporter->reportStatus("Initializing...", 0.0f);

  After Step 3 (EventBus.begin()):
    _statusReporter->reportStatus("Event system ready", 0.05f);

  After Step 4 (LittleFS mount success):
    _statusReporter->reportStatus("Filesystem mounted", 0.10f);

  After Step 5 (FrameworkConfig stored):
    _statusReporter->reportStatus("Configuration defaults loaded", 0.15f);

  After Step 6 (DeviceCrypto + KeyManager):
    _statusReporter->reportStatus("Security services ready", 0.20f);

  After Step 8 (ConfigManager.beginAll()):
    _statusReporter->reportStatus("Module configuration loaded", 0.35f);

  At Step 9 (WIFI_SETUP state):
    _statusReporter->reportStatus("Connecting to WiFi...", 0.40f);

  After Step 10 (WifiManager.begin() returns):
    _statusReporter->reportStatus("WiFi initialized", 0.55f);

  After Step 12 (TimeManager.begin()):
    _statusReporter->reportStatus("Time synchronization started", 0.65f);

  After Step 13 (DataManager.begin()):
    _statusReporter->reportStatus("Data services ready", 0.70f);

  After Step 14 (OTA.begin()):
    _statusReporter->reportStatus("Update service ready", 0.80f);

  After Step 16 (WebServer + MCP begin):
    _statusReporter->reportStatus("Web portal starting...", 0.90f);

  After Step 17 (_compactAndLock()):
    _statusReporter->reportStatus("Boot complete", 1.0f);

  Before Step 19 (State → RUNNING):
    _statusReporter->clearStatus();
  ```

  **IMPORTANT:**
  - Place each call on a SINGLE line immediately after the corresponding step completes
  - Do NOT wrap these in #ifndef NATIVE_BUILD — they should work in both modes (NullStatusReporter handles native)
  - Keep the existing ESP_LOGI/ESP_LOGE log lines — the reportStatus calls are IN ADDITION to them
  - If LittleFS mount FAILS, call `_statusReporter->reportError("Filesystem mount failed");` before returning false

### 2. Update docstring

- [ ] **[MODIFY] `modules/application/Application.hpp`**

  Update the `begin()` docstring to mention that boot progress is reported via IStatusReporter at each milestone. Add a note explaining that downstream apps can register a display module as IStatusReporter before calling begin() to enable visual boot progress. Something like:

  ```
  * Boot progress is reported via IStatusReporter at each milestone.
  * Downstream apps can call registerStatusReporter() before begin()
  * to enable visual boot progress on a display. When no reporter is
  * registered, the NullStatusReporter fallback absorbs all calls.
  ```

### 3. Add inline comment explaining the pattern

- [ ] **[MODIFY] `modules/application/Application.cpp`**

  Add a brief block comment near the top of `begin()` (after the boot banner, before the first reportStatus call) explaining the boot progress pattern:

  ```cpp
  // Boot progress: IStatusReporter receives progress updates at each
  // milestone. Downstream apps register a display module before begin()
  // to show visual boot progress. Default NullStatusReporter is a no-op.
  ```

## Verification Requirement

1. Run `pio run -e native` — all unit tests must pass
2. Run `pio run -e esp32dev` — firmware must compile
3. Run `pio run -e esp32s3nano` — firmware must compile

**CRITICAL:** Once all verification gates pass, commit with message: `feat(app): add IStatusReporter boot progress milestones to begin() [PR-11/WO4]` and send a message back to me with results. Do NOT distill until I explicitly approve.
