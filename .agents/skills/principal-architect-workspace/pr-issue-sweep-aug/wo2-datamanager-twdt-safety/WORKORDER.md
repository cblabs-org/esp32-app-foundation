---
name: issue-sweep-aug-wo2
description: Handoff instructions for Work Order 2 — DataManager TWDT safety
complexity: standard
---

# WO2 — DataManager TWDT Safety

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Harden `DataManager`'s fetch loop so a blocking `IDataSource::fetchUpdate()` cannot trigger the Task Watchdog Timer (TWDT) and boot-loop the device — **without trading the loud failure for a silent one.** Closes **#128**.

## Context
`DataManager::workerTaskLoop()` registers with TWDT at line 130, but only calls `esp_task_wdt_reset()` at line 137 — **after** `executeLoop()` returns. If a consumer's `fetchUpdate()` blocks (unreachable host, TLS handshake), the watchdog fires and resets the device. This turns a configuration mistake into an unrecoverable boot loop.

**The fix must not simply unsubscribe the watchdog around the fetch.** That inverts the watchdog's purpose — it leaves the task unprotected during all actual work, and trades a loud crash for a silent hang where a stalled fetch blocks the queue forever with no error, no recovery, and no reboot. The correct approach is to **keep the watchdog subscribed, feed it periodically from a separate mechanism during the fetch, and detect genuinely hung fetches.**

## Strict Boundary Box
- **ONLY** modify the files listed below.
- Do NOT change `DataManager`'s threading model, task priority, or queue architecture.
- Do NOT add new interfaces or change `IDataSource`'s method signatures (only add documentation).
- Do NOT touch any consumer `IDataSource` implementations.
- **Do NOT remove the existing `esp_task_wdt_add(NULL)` at line 130.** The entire TWDT lifecycle depends on this initial registration. Removing it causes `esp_task_wdt_reset()` to fail with `ESP_ERR_NOT_FOUND`.

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (rule `04-mcp-tooling.md`).
- Build verification: `build_project` targeting `crowpanel-128-rotary`.
- Native tests: `run_tests` with environment `native`.

## Logging Convention
**`DataManager.cpp` uses `ESP_LOG*` exclusively** (7 calls, zero `Log.*`). All new log statements in this file MUST use `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE` with the existing `TAG`. Do NOT use `Log.warningln()` or any ArduinoLog calls.

## Native Build Note
The existing TWDT calls at lines 130 and 137 are **not guarded** by `#ifdef ESP_PLATFORM` — they compile on native because `test/stubs/platform/esp_task_wdt.h` provides the stubs. Any new TWDT calls you add will resolve the same way. No `#ifdef` guards are needed.

## Line-Item Instructions

### 1. Feed the watchdog during the fetch (not unsubscribe)

- [ ] **`modules/DataManager/DataManager.cpp`** — Add a software timer or secondary task that feeds the TWDT periodically while `fetchUpdate()` is in progress.

  **Recommended approach — timer-fed watchdog:**

  Use an `esp_timer` (one-shot or periodic) that calls `esp_task_wdt_reset()` on the DataManager's task handle at a cadence shorter than the TWDT timeout (typically 5s, so feed every ~2s). Start the timer before `fetchUpdate()`, stop it after.

  ```cpp
  // Before the fetch:
  _fetchStartMs = millis();
  startWdtFeederTimer();     // periodic esp_timer that feeds the TWDT

  // The fetch (line 202):
  UpdateStatus result = fetchTarget->fetchUpdate();

  // After the fetch:
  stopWdtFeederTimer();
  _fetchStartMs = 0;
  esp_task_wdt_reset();      // one final feed after stopping the timer
  ```

  The timer callback needs the task handle — store it from `xTaskGetCurrentTaskHandle()` during `workerTaskLoop()` init (after line 130).

  **Alternative approach — if `esp_timer` feels heavy:** A simple `volatile uint32_t _fetchStartMs` member set before the call and cleared after is the minimum. This gives external observability (see item 3) even without the timer-fed watchdog. In that case, keep the existing TWDT subscription and accept that a genuinely infinite hang will eventually trigger the watchdog reset — which is actually the correct outcome for an unrecoverable hang.

  Choose whichever approach you judge simpler and more robust. Either way, the key invariant is: **the task stays subscribed to the TWDT at all times.**

### 2. Detect and report hung fetches

- [ ] **`modules/DataManager/DataManager.cpp`** — Add a `volatile uint32_t _fetchStartMs = 0` member (or use `unsigned long`) to `DataManager`. Set it to `millis()` before calling `fetchUpdate()`, clear it to 0 after.

  Add a public method for external observability:
  ```cpp
  /**
   * Returns the duration in ms of the currently-running fetch, or 0 if idle.
   * Safe to call from any task (member is volatile).
   */
  uint32_t getFetchElapsedMs() const {
      uint32_t start = _fetchStartMs;
      return (start > 0) ? (millis() - start) : 0;
  }
  ```

- [ ] **`modules/DataManager/DataManager.cpp`** (line 202 area, `executeLoop`)
  Log a warning if `fetchUpdate()` takes longer than 15 seconds:
  ```cpp
  unsigned long fetchStart = millis();
  _fetchStartMs = fetchStart;
  UpdateStatus result = fetchTarget->fetchUpdate();
  _fetchStartMs = 0;
  unsigned long fetchDuration = millis() - fetchStart;
  if (fetchDuration > 15000) {
      ESP_LOGW(TAG, "fetchUpdate() for source took %lu ms — consider reducing client timeout",
               fetchDuration);
  }
  ```

  Note: this warning only fires when the fetch eventually returns. The `_fetchStartMs` member provides visibility for the never-returns case via `getFetchElapsedMs()`.

### 3. Document the contract in IDataSource.hpp

- [ ] **`include/interfaces/IDataSource.hpp`** (around line 62, `fetchUpdate()`)
  Add a duration-guidance comment. Word it as queue-blocking guidance, **not** as an enforced limit (nothing enforces it after this change):

  ```cpp
  /**
   * Fetch updated data from the external source.
   *
   * @note Called from the DataManager worker task. This is a shared queue —
   *       a long-running fetch blocks all other registered data sources until
   *       it returns. Set client-level timeouts (e.g. HTTPClient::setTimeout())
   *       to bound network operations. As a guideline, implementations should
   *       aim to return within 30 seconds; fetches exceeding 15 s will log a
   *       warning.
   *
   * @return UpdateStatus indicating the result of the fetch operation.
   */
  ```

## Verification Requirement
1. `pio run -e crowpanel-128-rotary` — must compile cleanly.
2. Native test suite must pass (`pio test -e native`) — TWDT stubs handle all calls.
3. Verify that `esp_task_wdt_add(NULL)` at line 130 is still present and untouched.
4. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
✅ **Complete** — committed as `95ac59c8` on `issue-sweep-aug/wo2-datamanager-twdt-safety`.

### Changes Made
1. **`modules/DataManager/DataManager.hpp`** — Added `volatile uint32_t _fetchStartMs = 0` private member, `uint32_t getFetchElapsedMs() const` public method for external observability, and `#include <Arduino.h>` for `millis()` in the inline method.
2. **`modules/DataManager/DataManager.cpp`** (executeLoop) — Instrumented fetch with `_fetchStartMs` timing, added `esp_task_wdt_reset()` immediately after `fetchUpdate()` returns, and added `ESP_LOGW` warning when a fetch exceeds 15 seconds (naming the source and duration). All new log calls use `ESP_LOGW`/`ESP_LOGI` with existing `TAG`.
3. **`include/interfaces/IDataSource.hpp`** — Expanded `fetchUpdate()` doc comment with queue-blocking guidance, `HTTPClient::setTimeout()` recommendation, and the 15s warning threshold.

### Decisions Made
- **Chose the simpler approach** (volatile `_fetchStartMs` + post-fetch WDT reset) over the `esp_timer` feeder. Rationale: no `esp_timer` stub needed for native tests, less code surface, and the WO explicitly sanctions the watchdog firing on genuinely infinite hangs as the correct outcome. The key invariant — task stays subscribed to TWDT at all times — is preserved.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.

