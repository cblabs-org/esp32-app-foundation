---
complexity: trivial
---

# WO2 — DataManager Stack Watermark Monitoring

## Role
You are a senior embedded C++ / FreeRTOS engineer. You are adding stack watermark telemetry to an existing worker task.

## Parent
Project: `pr-122-dram-psram-optimization` — [PROJECT.md](../PROJECT.md)

## Objective
Add `uxTaskGetStackHighWaterMark` logging to the DataManager worker task, matching the pattern already used by `DeepStackExecutor` (see `lib/DisplayServiceHelper/DisplayServiceHelper.cpp` for reference). This is a **data-gathering** step — we need empirical watermark data before deciding whether to reduce the 12 KB stack or migrate it to PSRAM.

## Context
- The DataManager worker loop is at `modules/DataManager/DataManager.cpp:125-141`.
- The task runs `IDataSource::fetchUpdate()` which may invoke TLS (mbedTLS) — this is the deep-stack consumer.
- The `DeepStackExecutor` logs its watermark after each work item with:
  ```cpp
  ESP_LOGI(TAG, "PNG Worker stack high water mark: %lu bytes (expected peak ~10KB)",
           (unsigned long)uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t));
  ```
  Note: `uxTaskGetStackHighWaterMark` returns words, multiply by `sizeof(StackType_t)` to get bytes.
- We want to log the watermark periodically (e.g., every 60 seconds or every N fetch cycles) — not on every 100ms loop iteration, as that would spam the log.

## Strict Boundary Box — do NOT touch
- Do NOT change the task stack size (12288).
- Do NOT change `xTaskCreate` to `xTaskCreateWithCaps`.
- Do NOT move the task to PSRAM.
- Do NOT modify any other module.
- Do NOT flash, tag, or open a PR.

## Line-Item Instructions

- [ ] **`modules/DataManager/DataManager.cpp`** — Add a periodic stack watermark log inside `workerTaskLoop()`. Log once after the first successful fetch, then every 60 seconds thereafter. Use `ESP_LOGI` with TAG. Example pattern:
  ```cpp
  // Inside workerTaskLoop, after executeLoop():
  static uint32_t lastWatermarkLog = 0;
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  if (lastWatermarkLog == 0 || (now - lastWatermarkLog) >= 60000) {
      UBaseType_t hwm = uxTaskGetStackHighWaterMark(nullptr);
      ESP_LOGI(TAG, "DataManager worker stack high water mark: %lu bytes (of 12288)",
               (unsigned long)(hwm * sizeof(StackType_t)));
      lastWatermarkLog = now;
  }
  ```

## Verification Requirement
- `pio run -e crowpanel-128-rotary` compiles.
- `pio test -e native` — full suite green (the FreeRTOS task stub runs synchronously in native tests; `uxTaskGetStackHighWaterMark` is stubbed and won't crash).

## Completion Report
<!-- Written by the worker at task completion -->
