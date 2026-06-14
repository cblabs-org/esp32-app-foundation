---
title: "PR-21 WO1 — Firmware Heap Guard + PSRAM Stack Allocation"
distilled_at: "2026-06-11T20:52:00Z"
original_plan_id: "pr-21-ota-task-hardening"
branch: "pr-21-ota-task-hardening-wo1-heap-guard"
commit: "175dfc8"
status: "COMPLETE"
artifacts:
  - context_artifacts/adr_psram_task_allocation.md
  - context_artifacts/graveyard_native_build.md
---

## Executive Summary

WO1 is fully complete and approved. `OTAUpdateManager::performUpdate()` now uses
`xTaskCreateWithCaps(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` to allocate its 32 KB
FreeRTOS task stack in PSRAM on the ESP32-S3 Nano, with a pre-flight DRAM headroom
log warning when free DRAM drops below 40 KB. Two new fields (`dram_free_bytes`,
`ota_stack_bytes`) are exposed in the `/api/ota status` response for WO2 portal use.

## End State (as of commit 175dfc8)

### `OTAUpdateManager.hpp` additions (before class declaration)
```cpp
static constexpr uint32_t OTA_TASK_STACK_BYTES = 32768;
static constexpr uint32_t OTA_TASK_DRAM_HEADROOM_BYTES = 8192;
```

### `OTAUpdateManager.cpp` — new includes (inside #ifndef NATIVE_BUILD)
```cpp
#include <inttypes.h>
#include <freertos/idf_additions.h>
```

### `OTAUpdateManager.cpp` — performUpdate() task creation
```cpp
uint32_t freeDram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
uint32_t required = OTA_TASK_STACK_BYTES + OTA_TASK_DRAM_HEADROOM_BYTES;
if (freeDram < required) { ESP_LOGW(TAG, "Low DRAM..."); }
BaseType_t result = xTaskCreateWithCaps(
    otaTaskWrapper, "ota_update", OTA_TASK_STACK_BYTES,
    this, 5, &taskHandle_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
);
```

### `OTAUpdateManager.cpp` — status handler (two new fields)
```cpp
resp["dram_free_bytes"]  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
resp["ota_stack_bytes"]  = OTA_TASK_STACK_BYTES;
```

## Verification Results

| Check | Result |
|-------|--------|
| native run_tests | ✅ 525/525 PASS |
| esp32s3nano build | ✅ SUCCESS (0 errors, 0 new warnings) |
| Flash to device | ✅ Written & verified |
| Device boot | ✅ Clean — `Firmware self-test PASSED`, heap 8.6 MB |
| /api/ota status HTTP | ⚠️ Not live-tested (host not on AP network) — verified by code inspection |

## Next Steps (for WO2 — Portal Memory Warning)

WO2 consumes the two new status fields:
- `dram_free_bytes` — current free internal DRAM (bytes)
- `ota_stack_bytes` — constant `32768` — the required stack for the OTA task

WO2 worker should:
1. Read `wo2-portal-memory-warning/WORKORDER.md`
2. Branch is `pr-21-ota-task-hardening-wo2-portal-warning` (or similar)
3. Add a warning widget to the OTA tab that fires when
   `dram_free_bytes < ota_stack_bytes + 8192`

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_psram_task_allocation.md` — Why xTaskCreateWithCaps was chosen over xTaskCreate, why stack size stays at 32 KB, and why no `#ifdef CONFIG_SPIRAM` was needed
- `context_artifacts/graveyard_native_build.md` — The `pio build -e native` linker failure, the freertos/idf_additions.h resolution, and the HTTP API unreachable-from-host issue with mitigation guidance
