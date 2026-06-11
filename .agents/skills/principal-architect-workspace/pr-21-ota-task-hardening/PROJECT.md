# PR-21: OTA Task Creation Hardening

**PR Number:** 21  
**Project Name:** `pr-21-ota-task-hardening`  
**Base Branch:** `main`  
**Status:** 📋 PLANNING — awaiting user approval  
**Architect Session:** 8c58afa2-8ca2-4a85-b127-3fb94f785271  

---

## Problem Statement

During the PR-19 live test (`34de947b`), the S3 Nano device lost PSRAM due to sdkconfig drift.
With only ~35 KB DRAM free, the OTA FreeRTOS task (32 KB stack, `OTAUpdateManager.cpp` ~line 360)
could not be created — `xTaskCreate` returned `pdFAIL`. The device logged:
> "Failed to create OTA FreeRTOS task"

with no diagnostic information (no heap size reported, no threshold stated).

This created a **self-heal deadlock**: the firmware defect that caused memory loss could not be
fixed via OTA because OTA itself required the memory it couldn't allocate. Recovery required
direct USB flash.

The sdkconfig root cause is fixed (PR-19). This PR hardens OTA so that even in degraded-memory
conditions: (a) the failure is diagnosed clearly, and (b) the task succeeds by allocating its
stack from PSRAM when DRAM is insufficient.

---

## Branch Strategy

```
main
 └── pr-21-ota-task-hardening            ← Feature PR branch
      ├── pr-21-ota-task-hardening/wo1-firmware-heap-guard   ← WO1
      └── pr-21-ota-task-hardening/wo2-portal-memory-warning ← WO2 (bases on WO1)
```

WO2 stacks on WO1 (depends on the new readiness fields in the firmware API).
They must be serialised: WO1 merged into feature branch → then WO2 branches off.

---

## Work Order Ledger

| # | Status | Branch | Topic | Link |
|---|--------|--------|-------|------|
| WO1 | ✅ MERGED `a58d046` | `pr-21-ota-task-hardening-wo1-heap-guard` | Heap guard + PSRAM stack allocation | **[wo1-firmware-heap-guard/WORKORDER.md](wo1-firmware-heap-guard/WORKORDER.md)** |
| WO2 | 🔄 IN PROGRESS | `pr-21-ota-task-hardening-wo2-portal-warning` | Portal low-memory OTA warning | **[wo2-portal-memory-warning/WORKORDER.md](wo2-portal-memory-warning/WORKORDER.md)** |

---

## Architectural Decisions

### ADR-1: Use `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM)` rather than `xTaskCreate`
The ESP-IDF FreeRTOS port (Arduino-ESP32) exposes `xTaskCreateWithCaps()` which accepts a
`MemoryCaps` bitmask for stack placement. Using `MALLOC_CAP_SPIRAM` allocates the 32 KB stack
in PSRAM on S3 Nano, eliminating DRAM pressure from a known fixed-size allocation. On boards
without PSRAM the call falls back silently (PSRAM allocation returns NULL and the scheduler
assigns DRAM — this is the standard ESP-IDF behaviour, verified in esp-idf/freertos docs).

### ADR-2: Expose readiness via the existing `POST /api/ota { "action": "status" }` response
Rather than a dedicated `/api/ota/readiness` endpoint, we add two fields to the existing status
response: `dram_free_bytes` and `ota_stack_bytes`. This avoids a new route, keeps all OTA
state in one payload, and `ota.js` already calls `status` on page load — no extra round trip.

### ADR-3: Define `OTA_TASK_STACK_BYTES` as a named constant in `OTAUpdateManager.hpp`
The current code embeds `16384 * 2` as a magic number in `performUpdate()`. WO1 extracts this
as `OTA_TASK_STACK_BYTES = 32768` and `OTA_TASK_DRAM_HEADROOM_BYTES = 8192` (safety margin).
Both constants are used in the pre-flight guard and exposed in the status API, decoupling the
portal warning threshold from the firmware internals.

---

## Verification Plan

- WO1: Native build must compile clean. ESP32 target build must compile clean.
  PlatformIO `build_project` for both `native` and `esp32s3nano` environments.
- WO2: Portal OTA tab must render the warning banner when `dram_free_bytes < ota_stack_bytes`,
  and must NOT disable the Check for Updates button. Verified via mock-server Playwright test
  or manual inspection of the `loadStatus()` code path.

---

*Created: 2026-06-11 by PA session 8c58afa2*
