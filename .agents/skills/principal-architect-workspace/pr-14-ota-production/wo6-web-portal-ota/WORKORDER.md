---
name: pr-14-wo6-web-portal-ota
description: Handoff instructions for Work Order 6 — Web Portal OTA UI
---

# WO6 — Web Portal OTA UI (Reduced Scope)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Activate the existing PR-14 stubs in `web/portal/ota.js`, add a rollback action with confirmation token on the server side, wire new API endpoints, and display version/channel/board information. This is a **reduced scope** WO — the portal already has hidden stubs for channel selection and manifest info.

## Strict Boundary Box

- ✅ You MAY modify `web/portal/ota.js` (activate stubs, add rollback confirmation, wire endpoints)
- ✅ You MAY modify `modules/OTAUpdateManager/OTAUpdateManager.cpp` (extend API routes)
- ❌ Do NOT modify `modules/Application/` — completed in WO2
- ❌ Do NOT modify `lib/OTADownloadEngine/` — completed in WO2
- ❌ Do NOT modify `lib/OTAManifest/` — completed in WO3
- ❌ Do NOT modify `lib/GithubClient/` — completed in WO4
- ❌ Do NOT modify `lib/ConfigManifest/` — completed in WO5
- ❌ Do NOT modify `modules/ConfigManager/` — completed in WO5
- ❌ Do NOT create branches or manage git

## Project House Style

- Standard license header on new files
- PascalCase for C++ files/types
- JavaScript follows existing `ota.js` patterns (IIFE, `window.apiFetch`, DOM manipulation)

## Context: Current State of ota.js

The portal `ota.js` already has:
1. **Firmware info card** — displays version, board, partition from `GET /api/system/info`
2. **Hidden channel dropdown** — `#ota-channel-wrapper` with `style="display:none;"` containing a select with stable/beta/dev options
3. **Hidden manifest info** — `#ota-manifest-info` with `style="display:none;"`
4. **"Check for Updates" button** — posts `{ action: "checkGithub" }`, starts status polling
5. **Progress bar** — polls `{ action: "status" }`, shows downloading/verifying/complete/error states
6. **Manual file upload** — posts to `/update` endpoint
7. **Rollback button** — has client-side armed confirmation (3-second window), posts `{ action: "rollback" }`

## Context: Current API Routes (OTAUpdateManager.cpp registerRoutes())

- `GET /api/ota` → `{ "status": "ready" }`
- `POST /api/ota { "action": "rollback" }` → triggers `rollbackFirmware()` (⚠️ no confirmation token!)
- `POST /api/ota { "action": "markValid" }` → calls `markAppValid()`
- `POST /api/ota { "action": "checkGithub" }` → checks + performs update
- `POST /api/ota { "action": "status" }` → `{ "state": "...", "lastError": "..." }`

## Implementation Checklist

### 1. Activate Channel Dropdown in ota.js

- [ ] Remove `style="display:none;"` from `#ota-channel-wrapper` (or change to `display:block`)
- [ ] After page loads / tab activates, fetch current channel from the status endpoint and set the dropdown to match
- [ ] Add an event listener on the `#ota-channel` select:
  ```javascript
  document.getElementById('ota-channel').addEventListener('change', async (e) => {
    const channel = e.target.value;
    try {
      await window.apiFetch('/api/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'setChannel', channel: channel })
      });
      // Show success feedback
    } catch (error) {
      // Show error feedback
    }
  });
  ```

### 2. Activate Manifest Info in ota.js

- [ ] Remove `style="display:none;"` from `#ota-manifest-info` (or change to `display:block`)
- [ ] When firmware info loads, populate `#ota-manifest-info` with:
  - Board ID (from `BOARD_ID` macro — returned via status endpoint)
  - Current channel
  - Auto-check enabled/disabled

### 3. Enhance the Rollback Button with Typed Confirmation

- [ ] Replace the current 3-second armed confirmation with a **typed confirmation dialog**:
  - On first click, show a prompt/modal asking the user to type "ROLLBACK"
  - If the user types "ROLLBACK" (case-sensitive), send the API request with `{ "action": "rollback", "confirm": "ROLLBACK" }`
  - If the user cancels or types wrong, do nothing
- [ ] You can use a simple `prompt()` dialog for this — no need for a custom modal:
  ```javascript
  const input = prompt('Type ROLLBACK to confirm firmware rollback:');
  if (input === 'ROLLBACK') {
    // Send the request
  }
  ```

### 4. Extend the `status` API endpoint (OTAUpdateManager.cpp)

- [ ] In `registerRoutes()`, extend the `"status"` action response to include:
  ```cpp
  resp["version"] = String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH);
  resp["channel"] = _channel;
  resp["boardId"] = BOARD_ID;
  resp["autoCheck"] = _autoCheck;
  ```

### 5. Add `setChannel` API action (OTAUpdateManager.cpp)

- [ ] In `registerRoutes()`, add a new action handler:
  ```cpp
  } else if (strcmp(action, "setChannel") == 0) {
      const char* ch = doc["channel"] | "";
      if (strlen(ch) > 0 && strlen(ch) < sizeof(_channel)) {
          strlcpy(_channel, ch, sizeof(_channel));
          // Persist via ConfigManager
          if (app_ != nullptr) {
              app_->getConfigManager().saveModuleConfig(this);
          }
          ESP_LOGI(TAG, "Channel changed to: %s", _channel);
          request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
          request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid channel\"}");
      }
      return;
  ```

### 6. Add confirmation token to `rollback` API action (OTAUpdateManager.cpp)

- [ ] In `registerRoutes()`, modify the existing `"rollback"` handler to require confirmation:
  ```cpp
  if (strcmp(action, "rollback") == 0) {
      const char* confirm = doc["confirm"] | "";
      if (strcmp(confirm, "ROLLBACK") != 0) {
          request->send(403, "application/json",
                        "{\"status\":\"error\",\"msg\":\"Missing or invalid confirmation token\"}");
          return;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      this->rollbackFirmware();
      return;
  }
  ```

### 7. Update firmware info display in ota.js

- [ ] Enhance the `loadFirmwareInfo()` function to also display the channel and auto-check status
- [ ] If the status endpoint returns version/channel/board, use those to populate the firmware info card with richer data

## Verification

- [ ] `pio run -e esp32dev` — hardware build succeeds
- [ ] `pio run -e esp32s3nano` — hardware build succeeds
- [ ] `pio test -e native` — full suite passes (zero regressions)
- [ ] Visual review: the channel dropdown is visible and functional
- [ ] Visual review: rollback requires typing "ROLLBACK"
- [ ] Visual review: manifest info section shows board ID and channel

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All builds succeed
2. Tests pass
3. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.

---

## Execution Status

**Status:** ✅ COMPLETE  
**Executed by:** Session `80b887dc-8226-4e2b-bafb-cb83a13f0c7f`  
**Date:** 2026-06-05  
**Branch:** `pr-14-ota-production-wo6-web-portal-ota`

### Checklist Results

- [x] Activate channel dropdown in ota.js
- [x] Activate manifest info in ota.js
- [x] Enhance rollback button with typed confirmation
- [x] Extend `status` API endpoint (OTAUpdateManager.cpp)
- [x] Add `setChannel` API action (OTAUpdateManager.cpp)
- [x] Add confirmation token to `rollback` API action (OTAUpdateManager.cpp)
- [x] Update firmware info display in ota.js

### Verification Results

- [x] `pio run -e esp32dev` — **PASSED** (RAM: 15.0%, Flash: 88.4%)
- [x] `pio run -e esp32s3nano` — **PASSED** (RAM: 15.7%, Flash: 59.6%)
- [x] `pio test -e native` — **518/518 tests PASSED** (zero regressions)

### Issues / Escalation

None.
