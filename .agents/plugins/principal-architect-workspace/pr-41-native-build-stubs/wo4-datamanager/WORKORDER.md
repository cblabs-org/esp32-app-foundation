---
name: pr41-wo4
description: Handoff instructions for Work Order 4 — Migrate DataManager module
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from the DataManager module. The FreeRTOS stub headers provide mutex, task, and watchdog API surfaces.

## Strict Boundary Box

- **DO** modify only files in `modules/DataManager/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

- [ ] **`DataManager.hpp`** — Remove `#ifndef NATIVE_BUILD` guard around FreeRTOS includes and the `#else` block that provides `typedef` stubs for `TaskHandle_t` and `SemaphoreHandle_t`. Include the FreeRTOS headers unconditionally (they resolve to stubs in native). Remove `#ifdef NATIVE_BUILD` guards around `setSimulatedTimeMs()` and `_simulatedTimeMs` — keep the test helper available unconditionally (or remove if the stub `millis()` makes it redundant).
- [ ] **`DataManager.cpp`** — Remove all guards:
  - Include guards for `<Arduino.h>`, `esp_task_wdt.h`, `<freertos/idf_additions.h>`
  - `MutexLock` helper: remove guards around `xSemaphoreTake()`/`xSemaphoreGive()`
  - Constructor: remove guard around `xSemaphoreCreateMutex()`
  - Destructor: remove guards around `vTaskDelete()`/`vSemaphoreDelete()`
  - `begin()`: remove guard around `xTaskCreateWithCaps()` worker task creation
  - `processNext()`: remove `millis()` guard (use stub millis)
  - Task body: remove `esp_task_wdt_add()`/`reset()`/`vTaskDelay()` guards
- [ ] **`millis()` handling**: The current native code uses `_simulatedTimeMs` as a `millis()` replacement. With the stub `millis()` returning 0, decide whether `_simulatedTimeMs` is still needed for test controllability. If tests set simulated time, keep the mechanism. If not, simplify to use `millis()` unconditionally.
- [ ] **`xTaskCreateWithCaps` stub**: Verify the stub calls the task function directly (synchronously). DataManager's worker task runs in a loop — the stub may need to call it once and return, or provide a mechanism for test-driven stepping. Check existing test behaviour.
- [ ] **Verify**: `pio test -e native` — all DataManager tests pass
- [ ] **Verify**: `pio run -e esp32s3nano` — hardware build succeeds

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update - Completed
DataManager module has been unified. NATIVE_BUILD guards are removed. FreeRTOS stubs have been updated to support synchronous loop execution via C++ exceptions in vTaskDelay. Both native and esp32s3nano tests passed and changes have been committed.
