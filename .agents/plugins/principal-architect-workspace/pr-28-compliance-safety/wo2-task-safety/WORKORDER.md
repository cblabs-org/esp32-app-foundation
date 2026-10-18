---
name: pr28-wo2
description: Handoff instructions for Work Order 2 — FreeRTOS task safety hardening
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Harden all 4 custom FreeRTOS tasks across the codebase:
1. Register long-running tasks with the Task Watchdog Timer (TWDT)
2. Upgrade `DataManager` stack from 8KB → 12KB
3. Switch `cert_gen` and `DataManager` tasks to `xTaskCreateWithCaps` with DRAM
4. Add return-value checks for `xTaskCreate`/`xTaskCreateWithCaps` calls

# Strict Boundary Box

- **DO** modify only the 3 files listed in the checklist.
- **DO NOT** change any module interfaces, public APIs, or header files.
- **DO NOT** rename variables or refactor code beyond the specific changes listed.
- **DO NOT** touch TAG definitions or naming conventions.
- **DO NOT** modify the OTA task creation in `OTAUpdateManager.cpp` — it already
  uses `xTaskCreateWithCaps` with DRAM caps and checks the return value. It only
  needs TWDT registration.

# Background: TWDT Pattern

```cpp
#include "esp_task_wdt.h"

// At task entry:
esp_task_wdt_add(NULL);

// In periodic loops (download loops, fetch cycles):
esp_task_wdt_reset();

// Before task cleanup / vTaskDelete:
esp_task_wdt_delete(NULL);
```

TWDT calls must be wrapped in `#ifndef NATIVE_BUILD` guards since the TWDT API
is not available in native test builds.

# Checklist

## A. OTAUpdateManager — TWDT Registration Only

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp`** — `performUpdateTask()` method
  - Add `#include "esp_task_wdt.h"` at top (inside `#ifndef NATIVE_BUILD` guard if needed)
  - At the **start** of `performUpdateTask()`: add `esp_task_wdt_add(NULL);` (guarded)
  - In the **download progress loop** (inside `OTADownloadEngine` callback or similar): add `esp_task_wdt_reset();`
  - **Before every exit path** (success reboot, error cleanup, `vTaskDelete`): add `esp_task_wdt_delete(NULL);`
  - Wrap all TWDT calls in `#ifndef NATIVE_BUILD`

## B. DataManager — Stack Upgrade + DRAM Caps + TWDT

- [ ] **`modules/DataManager/DataManager.cpp` L64** — Task creation
  ```cpp
  // BEFORE:
  xTaskCreate(workerTaskLoop, "DataManager", 8192, this, 5, &workerTask);
  // AFTER:
  BaseType_t result = xTaskCreateWithCaps(
      workerTaskLoop,
      "DataManager",
      12288,                                    // 12KB — safe for TLS
      this,
      5,
      &workerTask,
      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT     // DRAM stack
  );
  if (result != pdPASS) {
      ESP_LOGE(TAG, "Failed to create DataManager worker task");
      // Set appropriate error state
  }
  ```
  - Add `#include "esp_private/esp_task_wdt.h"` or `"esp_task_wdt.h"` (check which is available)

- [ ] **`modules/DataManager/DataManager.cpp`** — `workerTaskLoop()` method
  - At task entry: add `esp_task_wdt_add(NULL);` (guarded)
  - Inside the fetch loop (after each `fetchUpdate()` call): add `esp_task_wdt_reset();`
  - Before `vTaskDelete` / task exit: add `esp_task_wdt_delete(NULL);`

## C. AuthManager — cert_gen Tasks: DRAM Caps + Return Check

- [ ] **`modules/AuthManager/AuthManager.cpp` L701** — First `cert_gen` task
  ```cpp
  // BEFORE:
  xTaskCreate(
      [](void* param) { ... },
      "cert_gen", 12288, this, 1, nullptr
  );
  // AFTER:
  BaseType_t certResult = xTaskCreateWithCaps(
      [](void* param) {
          auto* self = static_cast<AuthManager*>(param);
          self->generateAndStoreCert("esp32-device");
          self->_certGenRunning.store(false);
          vTaskDelete(nullptr);
      },
      "cert_gen",
      12288,
      this,
      1,
      nullptr,
      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
  );
  if (certResult != pdPASS) {
      ESP_LOGE(TAG, "Failed to create cert_gen task");
      _certGenRunning.store(false);  // Reset guard so future attempts aren't blocked
  }
  ```

- [ ] **`modules/AuthManager/AuthManager.cpp` L858** — Second `cert_gen` task
  Apply the same pattern as L701:
  - `xTaskCreate` → `xTaskCreateWithCaps` with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`
  - Check return value
  - On failure: log error and `_certGenRunning.store(false)`

  Note: The `cert_gen` tasks are **short-lived** (self-deleting after cert generation).
  TWDT registration is **optional** for these — the task runs once and exits.
  If you add TWDT, add `esp_task_wdt_add(NULL)` at task entry and
  `esp_task_wdt_delete(NULL)` before `vTaskDelete(nullptr)`. But it's acceptable
  to skip TWDT for self-deleting tasks and add a comment explaining why.

# Important Notes

- `xTaskCreateWithCaps` is an ESP-IDF 5.x API. Verify it's available in the
  project's ESP-IDF version (it should be — OTAUpdateManager already uses it).
- All TWDT and `xTaskCreateWithCaps` calls must be guarded with
  `#ifndef NATIVE_BUILD` since they're not available in native test builds.
- The `DataManager` task is a **long-running worker loop** — it MUST register
  with TWDT and feed the watchdog periodically.
- Preserve all existing comments and documentation.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
