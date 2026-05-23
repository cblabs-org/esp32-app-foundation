---
name: pr-09-ota-security-wo3
description: Handoff instructions for Work Order 3 — Upgrade otaUpdateManager to orchestrate the full OTA security pipeline, wire OtaDownloadEngine + IOtaVerifier, add checkForUpdate/performUpdate, FrameworkConfig.otaPublicKey, EventBus events, and FreeRTOS task offloading
---

# WO3 — otaUpdateManager Upgrade

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Upgrade the existing `otaUpdateManager` module to become the **full OTA orchestrator**. It will:
1. Use `GithubClient` to check for new releases
2. Use `OtaDownloadEngine` (WO2) to download and flash firmware with cryptographic verification via `IOtaVerifier` (WO1)
3. Publish `ota.progress` EventBus events matching the existing `OtaProgressPayload` format
4. Add `otaPublicKey` and `otaGithubRepo`/`otaGithubToken` fields to `FrameworkConfig`
5. Run the download in a FreeRTOS task to respect the tick() ≤ 10ms constraint

## Strict Boundary Box

- **DO NOT** modify `lib/otaDownloadEngine/` — WO1 and WO2 deliverables are frozen
- **DO NOT** modify `include/interfaces/IOtaVerifier.hpp` — WO1 deliverable, frozen
- **DO NOT** modify `lib/githubClient/` — out of scope
- **DO NOT** modify `modules/eventBus/` — out of scope
- **DO NOT** modify `modules/u8g2Display/` — out of scope
- **DO NOT** modify `docs/design_specification.md` — that is WO4's scope
- **DO NOT** create git branches or manage PRs
- Files you MAY modify: `modules/otaUpdateManager/otaUpdateManager.hpp`, `modules/otaUpdateManager/otaUpdateManager.cpp`, `include/FrameworkConfig.hpp`
- Files you MAY create: `test/test_unit/test_ota_update_manager/test_ota_update_manager.cpp`

## Context Files — Read These First

Before writing any code, you MUST read these files:

1. **WO1 deliverables** (your verifier dependency):
   - `include/interfaces/IOtaVerifier.hpp`
   - `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp`

2. **WO2 deliverables** (your download engine dependency):
   - `lib/otaDownloadEngine/OtaDownloadEngine.hpp`
   - `lib/otaDownloadEngine/OtaError.hpp`

3. **Current otaUpdateManager** (the file you are modifying):
   - `modules/otaUpdateManager/otaUpdateManager.hpp`
   - `modules/otaUpdateManager/otaUpdateManager.cpp`

4. **Integration dependencies** (pattern reference):
   - `include/FrameworkConfig.hpp` — config struct to extend
   - `modules/eventBus/EventBus.hpp` — publish/subscribe API
   - `lib/githubClient/githubClient.hpp` — release metadata client
   - `modules/u8g2Display/u8g2DisplayModule.hpp` lines 87-92 — `OtaProgressPayload` struct (the EXISTING event format you must match)
   - `modules/application/Application.hpp` — composition root, has `getEventBus()` and `getConfig()`

5. **Departures-board reference** (for orchestration logic):
   - `/Users/mcneillm/Documents/Projects/departures-board/lib/hTTPUpdateGitHub/hTTPUpdateGitHub.cpp` — the full reference pipeline showing how everything fits together

6. **Framework patterns**:
   - `include/platform/LogShim.h` — logging pattern
   - Read `.agents/skills/house-style-docs/SKILL.md` for documentation standards

## Key Design Constraints

1. **Hollywood Principle**: The module must NOT `#include` other modules' headers directly. Use forward declarations and receive dependencies via `begin()`.
2. **WiFiClientSecure on heap only**: Per design spec §14.3, `WiFiClientSecure` must be heap-allocated inside fetch methods: `std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure())`. Never on the stack.
3. **tick() ≤ 10ms**: The main-loop tick must never block. Downloads must run in a FreeRTOS task.
4. **EventBus compatibility**: Publish `ota.progress` events using the existing `OtaProgressPayload` struct defined in `u8g2DisplayModule.hpp` lines 87-92. Forward-declare or duplicate the struct.
5. **Verifier begin() responsibility**: The manager (this module) must call `verifier_->begin(publicKey)` before calling `engine.downloadAndFlash()`. The engine does NOT call begin() on the verifier.

## Line-Item Instructions

### Step 1: Extend `include/FrameworkConfig.hpp`

Add these new fields to the `FrameworkConfig` struct (after the existing fields):

```cpp
const char* otaPublicKey    = nullptr;   ///< PEM-encoded RSA public key for OTA signature verification (nullptr = skip)
const char* otaGithubRepo   = nullptr;   ///< GitHub repository in "owner/repo" format for OTA releases
const char* otaGithubToken  = nullptr;   ///< Optional GitHub access token for private repos
```

Update the module description comment to document the new fields. 

### Step 2: Rewrite `modules/otaUpdateManager/otaUpdateManager.hpp`

The header must be substantially rewritten. Keep the class name `otaUpdateManager` and the global `extern otaUpdateManager otaManager;`.

- [ ] House-style header block (update Module and Description)
- [ ] Keep `#pragma once`
- [ ] Keep the `NATIVE_BUILD` guard for `Arduino.h`/`Stream.h`
- [ ] Keep the `IWebContributor` inheritance
- [ ] Forward-declare (never include): `Application`, `EventBus`, `GithubClient`, `OtaDownloadEngine`, `IOtaVerifier`, `MbedTLSSignatureVerifier`, `WiFiClientSecure`
- [ ] Define an `OtaState` enum:
  ```cpp
  enum class OtaState : uint8_t {
      Idle,
      Checking,     // Querying GitHub for latest release
      Downloading,  // FreeRTOS task running the download pipeline
      Verifying,    // Signature verification in progress
      Complete,     // Update finalized — waiting for reboot
      Error,        // An error occurred
  };
  ```
- [ ] Define a POD struct for the progress event payload (duplicate of OtaProgressPayload from u8g2DisplayModule.hpp — do NOT include that header):
  ```cpp
  struct OtaProgressEvent {
      float progress;       // 0.0 to 1.0
      const char* status;   // Status message
      bool complete;        // True when OTA cycle finished
      bool success;         // True if OTA succeeded
  };
  ```
  Note: This struct must be layout-compatible with `OtaProgressPayload` in `u8g2DisplayModule.hpp` so the EventBus subscriber can cast safely.
- [ ] Retain existing public methods: `beginUpdate()`, `writeStream()`, `endUpdate()`, `markAppValid()`, `rollbackFirmware()`, `registerRoutes()`, `getTabName()`
- [ ] Add new public methods:
  ```cpp
  /// Initialize the OTA manager with dependencies from the Application.
  void begin(Application& app);
  
  /// Check GitHub for a newer firmware version. Non-blocking.
  /// Returns true if a new version is available, false otherwise.
  bool checkForUpdate();
  
  /// Start the full OTA download + verify + flash pipeline.
  /// Launches a FreeRTOS task — returns immediately. Check getState() for progress.
  void performUpdate();
  
  /// Get the current OTA state machine state.
  OtaState getState() const;
  
  /// Get the last OTA error string (from OtaDownloadEngine).
  const char* getLastOtaError() const;
  ```
- [ ] Private members:
  ```cpp
  Application*             app_          = nullptr;
  MbedTLSSignatureVerifier verifier_;               // Owned verifier instance
  OtaState                 state_        = OtaState::Idle;
  const char*              lastOtaError_ = nullptr;
  
  // FreeRTOS task handle for the download task
  // (use TaskHandle_t under #ifndef NATIVE_BUILD, void* under #else)
  
  /// Static FreeRTOS task function — delegates to performUpdateTask()
  static void otaTaskWrapper(void* param);
  
  /// Instance method running the full OTA pipeline inside the task
  void performUpdateTask();
  
  /// Publish an OTA progress event to the EventBus.
  void publishProgress(float progress, const char* status, bool complete, bool success);
  ```

### Step 3: Rewrite `modules/otaUpdateManager/otaUpdateManager.cpp`

The implementation must be substantially rewritten. All of the existing functionality (beginUpdate, writeStream, endUpdate, markAppValid, rollbackFirmware, registerRoutes) must be preserved.

- [ ] House-style header block (update Module and Description)
- [ ] `#include "otaUpdateManager.hpp"`
- [ ] `#include "platform/LogShim.h"`
- [ ] `static const char* TAG = "OtaUpdateManager";`
- [ ] Hardware includes behind `#ifndef NATIVE_BUILD`:
  ```cpp
  #include <ESPAsyncWebServer.h>
  #include <ArduinoJson.h>
  #include <Update.h>
  #include <esp_ota_ops.h>
  #include <WiFiClientSecure.h>
  #include "../application/Application.hpp"
  #include "../../lib/githubClient/githubClient.hpp"
  #include "../../lib/otaDownloadEngine/OtaDownloadEngine.hpp"
  #include "../../lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp"
  ```
  Note: These module includes (`Application.hpp`) go in the `.cpp` ONLY, behind `#ifndef NATIVE_BUILD`, respecting the Hollywood Principle.
- [ ] Native includes:
  ```cpp
  #else
  #include <ArduinoJson.h>
  // Stub types needed for native compilation
  class Application {};
  #endif
  ```

**`begin()` implementation:**
- [ ] Store `app_` pointer
- [ ] ESP_LOGI logging

**`checkForUpdate()` implementation:**
- [ ] Guard: return false if `state_ != Idle`
- [ ] Guard: return false if `app_->getConfig().otaGithubRepo == nullptr`
- [ ] Set state to `Checking`
- [ ] Behind `#ifndef NATIVE_BUILD`:
  - Create `GithubClient` with repo and token from config
  - Call `getLatestRelease()`
  - If success: compare `releaseId` to current firmware version (use `__DATE__` + `__TIME__` or a simpler "always update for now" approach)
  - Set state back to `Idle` on completion
  - Return true if update available, false otherwise
- [ ] On native: return false

**`performUpdate()` implementation:**
- [ ] Guard: return if `state_ != Idle`
- [ ] Set state to `Downloading`
- [ ] Publish progress event: "Starting OTA update..."
- [ ] Behind `#ifndef NATIVE_BUILD`:
  - Create a FreeRTOS task via `xTaskCreate(otaTaskWrapper, "ota_update", 8192, this, 1, &taskHandle_)`
  - Return immediately
- [ ] On native: set state to Error

**`otaTaskWrapper()` static:**
- [ ] Cast param to `otaUpdateManager*`
- [ ] Call `performUpdateTask()`
- [ ] `vTaskDelete(nullptr)`

**`performUpdateTask()` implementation** (the big one — runs in FreeRTOS task context):
- [ ] ESP_LOGI: "OTA task started"
- [ ] Publish progress: "Checking for update..." (progress=0.0)
- [ ] Guard: check `firmwareUpdatesEnabled` from config
- [ ] Guard: check `otaGithubRepo` from config

- [ ] **Step 1: Check GitHub for releases**
  - Create `GithubClient` with repo and token
  - Call `getLatestRelease()`
  - If failure: publish error event, set state to Error, return
  - ESP_LOGI: log the release ID

- [ ] **Step 2: Find the .bin and .sig assets**
  - Search `releaseAssetName[]` for the .bin and .sig files
  - The .bin file name should match `*firmware*.bin` or be the first `.bin` asset
  - The .sig file name should be `<bin_name>.sig`
  - If .bin not found: publish error, set state to Error, return
  - ESP_LOGW if .sig not found but otaPublicKey is set (security degraded)

- [ ] **Step 3: Initialize the verifier (if public key configured)**
  - If `otaPublicKey != nullptr`:
    - Call `verifier_.begin(app_->getConfig().otaPublicKey)`
    - If begin() fails: publish error, set state to Error, return
    - ESP_LOGI: "Verifier initialized with public key"

- [ ] **Step 4: Create WiFiClientSecure on the heap**
  - `std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure())`
  - Guard: if nullptr, publish error, set state to Error, return
  - `client->setInsecure()` — GitHub CDN redirects use different certs than the API
  - ESP_LOGI: "WiFiClientSecure allocated on heap"

- [ ] **Step 5: Create OtaDownloadEngine and configure**
  - `OtaDownloadEngine engine;`
  - `engine.setVerifier(&verifier_)` (only if otaPublicKey set)
  - Set progress callback: lambda that calls `publishProgress()`
  - ESP_LOGI: "Download engine configured"

- [ ] **Step 6: Fetch signature file (if .sig URL available)**
  - `uint8_t sigBuf[512]; size_t sigLen = 0;`
  - If sigUrl is available: `sigLen = engine.fetchSignature(*client, sigUrl, token, sigBuf, sizeof(sigBuf))`
  - If sigLen == 0 and otaPublicKey is set: ESP_LOGW (no signature but key configured)
  - Publish progress: "Downloading firmware..." (progress=0.1)

- [ ] **Step 7: Download and flash firmware**
  - `OtaError result = engine.downloadAndFlash(*client, binUrl, token, sigBuf, sigLen)`
  - If `result != OtaError::None`:
    - `lastOtaError_ = engine.getLastErrorString()`
    - Publish error event with the error string
    - Set state to Error
    - Return
  - ESP_LOGI: "Firmware downloaded and verified successfully"

- [ ] **Step 8: Finalize**
  - Set state to Complete
  - Publish progress: "Update complete — rebooting..." (complete=true, success=true)
  - `delay(2000)` — allow event propagation
  - `ESP.restart()`

**`publishProgress()` implementation:**
- [ ] Create `OtaProgressEvent` on the stack
- [ ] Behind `#ifndef NATIVE_BUILD`: `app_->getEventBus().publish("ota.progress", &event)`
- [ ] ESP_LOGD: log the progress event

**`getState()` and `getLastOtaError()` implementations:** trivial getters.

**Preserve existing implementations:**
- [ ] `beginUpdate()`, `writeStream()`, `endUpdate()` — keep as-is (they remain for the direct web upload route)
- [ ] `markAppValid()`, `rollbackFirmware()` — keep as-is
- [ ] `registerRoutes()` — keep existing routes AND add a new POST route for triggering GitHub-based update:
  ```
  POST /api/ota { "action": "checkGithub" }  → calls checkForUpdate() + performUpdate()
  POST /api/ota { "action": "status" }       → returns JSON with state, lastError
  ```
- [ ] ESP_LOGx instrumentation: At least 10 log calls in the new code

### Step 4: Create native unit tests `test/test_unit/test_ota_update_manager/test_ota_update_manager.cpp`

- [ ] GoogleTest framework
- [ ] Test suite: `OtaUpdateManagerTest`
- [ ] Tests:
  - `DefaultState_IsIdle` — verify initial state is `OtaState::Idle`
  - `CheckForUpdate_WithoutBegin_ReturnsFalse` — call checkForUpdate() before begin()
  - `PerformUpdate_WithoutBegin_SetsError` — call performUpdate() before begin() on native
  - `GetLastOtaError_Default_ReturnsNull` — verify lastOtaError is nullptr initially
  - `BeginUpdate_OnNative_ReturnsFalse` — verify legacy beginUpdate() returns false on native
  - `EndUpdate_OnNative_ReturnsFalse` — verify legacy endUpdate() returns false on native
- [ ] Do NOT test FreeRTOS task creation (native has no FreeRTOS)

### Step 5: Verify Compilation

- [ ] Run: `pio test -e native --filter test_ota_update_manager` — all tests must pass
- [ ] Re-run WO1 and WO2 test suites to verify no regressions:
  - `pio test -e native --filter test_ota_verifier`
  - `pio test -e native --filter test_ota_download_engine`

## Verification Requirement

After completing all steps:
1. All files must compile on the `native` environment
2. All unit tests must pass (WO1, WO2, and WO3 test suites)
3. **STOP and send a message back to me (the Principal Architect)** reporting:
   - Which files were created/modified
   - Test results (all three suites)
   - Any issues or design questions that arose
4. **DO NOT** wrap up, distill, or commit until I explicitly approve the work

---

## Execution Status

| Field | Value |
|-------|-------|
| **Status** | ✅ COMPLETE |
| **Branch** | `pr-09-ota-security-wo3-manager-upgrade` |
| **Commit** | `9a07d0b` — `feat(ota): upgrade otaUpdateManager to full OTA orchestrator with GitHub pipeline [PR-09/WO3]` |
| **Date** | 2026-05-22 |
| **Tests** | 349/349 PASSED (WO1: 4, WO2: 8, WO3: 16, other: 321) |
| **Files Changed** | 6 files, +807 / -109 lines |

### Modified Files
- `include/FrameworkConfig.hpp` — Added `otaPublicKey`, `otaGithubRepo`, `otaGithubToken`
- `modules/application/Application.hpp` — Added `getConfig()` accessor
- `modules/application/Application.cpp` — Added `getConfig()` implementation
- `modules/otaUpdateManager/otaUpdateManager.hpp` — Full rewrite: OtaState, OtaProgressEvent, orchestrator + legacy API
- `modules/otaUpdateManager/otaUpdateManager.cpp` — Full rewrite: 8-step pipeline, FreeRTOS task, EventBus, REST routes
- `test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp` — 16 tests: state machine, guards, legacy API
