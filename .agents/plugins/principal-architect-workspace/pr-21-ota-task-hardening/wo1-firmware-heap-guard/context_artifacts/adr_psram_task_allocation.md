# ADR: PSRAM Stack Allocation for OTA FreeRTOS Task

**Date:** 2026-06-11  
**Status:** Accepted  
**WO:** pr-21-wo1 — Heap Guard + PSRAM Stack Allocation

## Context

The OTA FreeRTOS task was previously created with `xTaskCreate()` using a hardcoded
stack size of `16384 * 2 = 32768` bytes allocated from internal DRAM. On the ESP32-S3
Nano, which has 8 MB of PSRAM, this unnecessarily pressures the 320 KB internal DRAM
pool. If DRAM is low (e.g. after a previous failed OTA attempt), the task creation
silently fails with a generic "Failed to create OTA task" log that provides no
diagnostic value.

## Decision

1. **Extract stack size as a named constant** (`OTA_TASK_STACK_BYTES = 32768`) in the
   header so it is visible to both the task launcher and the status API.

2. **Replace `xTaskCreate` with `xTaskCreateWithCaps`** using
   `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`. On the S3/S3-Nano this allocates the 32 KB
   task stack from PSRAM instead of DRAM, freeing internal DRAM for WiFi, TLS, and heap
   allocations. On boards without PSRAM the IDF allocator falls back to DRAM
   transparently — no conditional compilation required.

3. **Add a pre-flight DRAM headroom check** that logs the actual free DRAM and the
   required threshold before attempting task creation. The log fires at WARN level only
   when DRAM < OTA_TASK_STACK_BYTES + OTA_TASK_DRAM_HEADROOM_BYTES (32768 + 8192 =
   40960 bytes), giving operators an actionable diagnosis message even if the task
   ultimately succeeds via PSRAM.

4. **Expose `dram_free_bytes` and `ota_stack_bytes`** in the `/api/ota status` response
   so the WO2 portal warning widget has the data it needs without an additional round-trip.

## Alternatives Rejected

- **Conditional `#ifdef CONFIG_SPIRAM`**: Would require sdkconfig introspection and
  produce divergent code paths. The IDF allocator's transparent fallback makes this
  unnecessary.
- **Reducing stack size**: TLS certificate chains and redirect chains require a minimum
  of 12–16 KB; 32 KB is the safe minimum with headroom. Reducing risks stack overflows
  on complex cert chains.
- **Pre-allocating PSRAM in `begin()`**: Would hold PSRAM continuously even when no OTA
  is in progress. Per-invocation allocation is more efficient.

## Consequences

- OTA task stack no longer occupies internal DRAM on PSRAM-equipped boards.
- Log messages now include exact byte counts for diagnostic clarity.
- Status API gains two new fields consumed by WO2's portal memory-warning widget.
- `freertos/idf_additions.h` is now included in the non-native build path — this is an
  IDF-internal header, verified to compile cleanly against ESP-IDF 5.5.4.
