---
name: pr-14-wo2-rollback-wiring
description: Handoff instructions for Work Order 2 — Auto-Rollback Wiring + Fail-Closed OTA Fix
---

# WO2 — Auto-Rollback Wiring + Fail-Closed OTA Fix

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Three critical changes:
1. **Wire `markAppValid()` into the boot sequence** — the self-test barrier that blesses the firmware after a successful boot.
2. **Fix the fail-open OTA verification vulnerability** — make signature verification **fail closed** when a public key is configured.
3. **Enable task watchdog panic** — ensure a hung OTA download triggers a reboot → auto-rollback.

## Strict Boundary Box

- ✅ You MAY modify `modules/Application/Application.cpp` (insert `markAppValid()` call only)
- ✅ You MAY modify `modules/OTAUpdateManager/OTAUpdateManager.cpp` and `.hpp` (security_version, TWDT subscription)
- ✅ You MAY modify `lib/OTADownloadEngine/OTADownloadEngine.cpp` (fail-closed fix)
- ✅ You MAY modify `sdkconfig.defaults` (TWDT panic flag)
- ✅ You MAY modify `platformio.ini` (SECURITY_VERSION build flag)
- ✅ You MAY create test files in `test/test_unit/`
- ❌ Do NOT modify `lib/SemVer/` — that's WO1 (completed)
- ❌ Do NOT modify `modules/ConfigManager/` — that's WO5
- ❌ Do NOT modify `web/portal/` — that's WO6
- ❌ Do NOT modify `lib/GithubClient/` — that's WO4
- ❌ Do NOT create branches or manage git
- ❌ Do NOT modify `.agents/` files except this WORKORDER checklist

## Project House Style

- PascalCase for files and types
- Standard license header on all new files ("ESP32 App Foundation (c) 2026 CB Labs")
- Doxygen `@brief` / `@param` / `@return` on all public methods
- `#pragma once` for headers
- `#ifndef NATIVE_BUILD` guards for ESP-IDF APIs

## Context: Current Code State

**Application.cpp boot sequence** (relevant section at lines 216-226):
```cpp
// --- Step 17: Lock all registration vectors ---
_compactAndLock();
_statusReporter->reportStatus("Boot complete", 1.0f);

// --- Step 18: Publish system heap event + heap report ---
#ifndef NATIVE_BUILD
    uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    _eventBus.publish("system.heap", &freeHeap);
#endif
    _reportHeap();
```
The `markAppValid()` call must go BETWEEN step 17 (`_compactAndLock()`) and step 18 (heap report).

**OTADownloadEngine.cpp fail-open vulnerability** (lines 389-404 in `performUpdateTask()`):
```cpp
if (sigUrl.isEmpty() && cfg.otaPublicKey != nullptr) {
    ESP_LOGW(TAG, "No .sig asset found but otaPublicKey is configured — security degraded");
}
```
...and in `downloadAndFlash()` (lines 393-404):
```cpp
if (verifier_ != nullptr && sigBuf != nullptr && sigLen > 0) {
    if (!verifier_->verify(sigBuf, sigLen)) { ... }
} else if (verifier_ == nullptr) {
    ESP_LOGW(TAG, "Skipping signature verification — no verifier configured");
}
```
Both paths allow unsigned firmware when a key IS configured but signature is missing.

## Implementation Checklist

### 1. Fix Fail-Closed OTA in `performUpdateTask()` (OTAUpdateManager.cpp)

- [ ] At line ~389 (after the `.sig` URL search loop), change the `sigUrl.isEmpty()` warning to an **abort**:
  ```cpp
  if (sigUrl.isEmpty() && cfg.otaPublicKey != nullptr) {
      ESP_LOGE(TAG, "SECURITY ABORT: No .sig asset found but otaPublicKey is configured — refusing unsigned firmware");
      publishProgress(0.0f, "Missing signature — update rejected", true, false);
      state_ = OTAState::Error;
      lastOtaError_ = "Missing .sig file — update rejected (fail-closed)";
      return;
  }
  ```
- [ ] At line ~446 (after `fetchSignature` returns), change the `sigLen == 0` warning to an **abort**:
  ```cpp
  if (sigLen == 0 && cfg.otaPublicKey != nullptr) {
      ESP_LOGE(TAG, "SECURITY ABORT: Signature fetch returned 0 bytes — refusing update");
      publishProgress(0.0f, "Signature fetch failed — update rejected", true, false);
      state_ = OTAState::Error;
      lastOtaError_ = "Signature empty — update rejected (fail-closed)";
      return;
  }
  ```

### 2. Fix Fail-Closed in `downloadAndFlash()` (OTADownloadEngine.cpp)

- [ ] At step 10 (line ~393), change the verification condition to fail closed:
  ```cpp
  // --- Step 10: Cryptographic signature verification ---
  if (verifier_ != nullptr) {
      // Fail closed: verifier is set, signature MUST be present
      if (sigBuf == nullptr || sigLen == 0) {
          ESP_LOGE(TAG, "SECURITY ABORT: Verifier configured but no signature provided — aborting");
          Update.abort();
          http.end();
          lastError_ = OTAError::SignatureFetchFailed;
          return lastError_;
      }
      if (!verifier_->verify(sigBuf, sigLen)) {
          ESP_LOGE(TAG, "Cryptographic signature verification FAILED — update discarded securely");
          Update.abort();
          http.end();
          lastError_ = OTAError::SignatureVerifyFailed;
          return lastError_;
      }
      ESP_LOGI(TAG, "Cryptographic signature verification PASSED");
  } else {
      ESP_LOGW(TAG, "No OTA public key — signature verification SKIPPED (dev mode)");
  }
  ```

### 3. Insert `markAppValid()` into Application.cpp

- [ ] In `Application::begin()`, after the `_compactAndLock()` call (line 218) and the `reportStatus("Boot complete")` (line 219), insert:
  ```cpp
  // --- Step 17b: Bless firmware (auto-rollback barrier) ---
  // If the firmware crashes before reaching this point, the bootloader
  // automatically reverts to the previous partition. This barrier covers:
  // display bootstrap, EventBus, LittleFS, Crypto, Config, WiFi, Time,
  // Data, OTA, Web, display integration, and registry lock.
  #ifndef NATIVE_BUILD
      _otaManager->markAppValid();
      ESP_LOGI(TAG, "✅ Firmware self-test PASSED — partition blessed");
  #endif
  ```

### 4. Add NVS `security_version` to OTAUpdateManager

- [ ] In `OTAUpdateManager.hpp`, add a private method declaration:
  ```cpp
  void checkSecurityVersion();
  ```
- [ ] In `OTAUpdateManager.cpp`, implement `checkSecurityVersion()`:
  ```cpp
  void OTAUpdateManager::checkSecurityVersion() {
  #ifndef NATIVE_BUILD
      #include <Preferences.h>  // Local include — only needed here
      Preferences prefs;
      prefs.begin("ota_security", false);
      uint32_t stored = prefs.getUInt("sec_ver", 0);
      
      if (SECURITY_VERSION > stored) {
          prefs.putUInt("sec_ver", SECURITY_VERSION);
          ESP_LOGI(TAG, "Security version updated: %u → %u", stored, SECURITY_VERSION);
      } else {
          ESP_LOGD(TAG, "Security version OK: stored=%u compiled=%u", stored, SECURITY_VERSION);
      }
      prefs.end();
  #endif
  }
  ```
  Note: Move the `#include <Preferences.h>` to the top with other platform includes, NOT inside the function.
- [ ] Call `checkSecurityVersion()` from `markAppValid()`, BEFORE the `esp_ota_mark_app_valid_cancel_rollback()` call

### 5. Add TWDT Subscription for OTA Task

- [ ] In `OTAUpdateManager.cpp`, at the start of `performUpdateTask()` (after the `ESP_LOGI(TAG, "OTA task started")` line), subscribe to TWDT:
  ```cpp
  #include "esp_task_wdt.h"  // Add to platform includes at top
  esp_task_wdt_add(NULL);  // Subscribe current task to TWDT
  ```
- [ ] In the chunked download loop inside `OTADownloadEngine.cpp` (the `while (totalWritten < ...)` loop), add TWDT feeding after each successful chunk write:
  ```cpp
  esp_task_wdt_reset();  // Feed watchdog after each chunk
  ```
  Note: This requires `#include "esp_task_wdt.h"` in OTADownloadEngine.cpp, guarded with `#ifndef NATIVE_BUILD`
- [ ] At the end of `performUpdateTask()`, before `ESP.restart()`, unsubscribe:
  ```cpp
  esp_task_wdt_delete(NULL);  // Unsubscribe before reboot
  ```

### 6. Modify `sdkconfig.defaults`

- [ ] Add `CONFIG_ESP_TASK_WDT_PANIC=y` — this makes the watchdog trigger a panic+reboot instead of just logging

### 7. Modify `platformio.ini`

- [ ] Add `-DSECURITY_VERSION=1` to the common `build_flags` section (same place where `VERSION_MAJOR` etc. were added by WO1)

### 8. Create Unit Tests `test/test_unit/test_ota_fail_closed/test_ota_fail_closed.cpp`

- [ ] GoogleTest suite `OTAFailClosedTest`
- [ ] NOTE: The actual `downloadAndFlash()` and `performUpdateTask()` methods are behind `#ifndef NATIVE_BUILD`, so the native tests can only verify the **logic design** through the OTAError enum and related interfaces
- [ ] Test: verify `OTAError::SignatureFetchFailed` has the expected integer value (-10)
- [ ] Test: verify `otaErrorToString(OTAError::SignatureFetchFailed)` returns a non-null string
- [ ] Test: verify `OTAError::SignatureVerifyFailed` is distinct from `SignatureFetchFailed`
- [ ] Test: verify the `OTADownloadEngine` native stub returns `HttpConnectionFailed` (proving the stub compiles and the interface contract holds)
- [ ] If feasible: create a testable helper function `shouldAbortOnMissingSig(bool verifierSet, bool sigPresent, size_t sigLen)` that encapsulates the fail-closed logic, and test it independently

## Verification

- [ ] `pio test -e native` — all existing + new tests pass (zero regressions)
- [ ] `pio run -e esp32dev` — hardware build succeeds
- [ ] `pio run -e esp32s3nano` — hardware build succeeds
- [ ] Grep for `"security degraded"` in `OTAUpdateManager.cpp` — should NOT appear (replaced with abort)
- [ ] Grep for `"Skipping signature verification"` in `OTADownloadEngine.cpp` — should NOT appear (replaced with fail-closed)

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All tests pass
2. Both hardware builds succeed
3. The fail-open vulnerability is closed
4. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.
