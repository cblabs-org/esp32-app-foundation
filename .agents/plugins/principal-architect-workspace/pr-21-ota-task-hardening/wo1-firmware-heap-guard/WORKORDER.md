---
name: pr-21-wo1
description: Handoff instructions for Work Order 1 — Heap guard + PSRAM stack allocation in OTAUpdateManager
complexity: standard
---

# pr-21-ota-task-hardening / WO1 — Firmware Heap Guard + PSRAM Stack Allocation

**Active branch:** `pr-21-ota-task-hardening-wo1-heap-guard`

You are an expert senior software engineer and execution-focused worker agent.
**DO NOT trigger or act as the principal-architect.**
I need you to completely execute Work Order 1.

---

## Objective

Harden `OTAUpdateManager::performUpdate()` so that when DRAM is insufficient to create the
OTA FreeRTOS task, (a) the failure is diagnosed with an actionable log message that includes
actual free heap and the required size, and (b) the task is created with its stack in PSRAM
when available, eliminating DRAM pressure entirely.

---

## Strict Boundary Box

- **Touch ONLY**: `modules/OTAUpdateManager/OTAUpdateManager.hpp` and
  `modules/OTAUpdateManager/OTAUpdateManager.cpp`.
- **Do NOT** modify any other module, test file, portal file, or platformio.ini.
- **Do NOT** make architectural decisions — the design is specified below.
- **Do NOT** branch. You will be spawned into an already-branched workspace
  (`pr-21-ota-task-hardening-wo1-heap-guard`).

---

## Checklist

### Step 1 — Extract the stack size as named constants (`.hpp`)

In `OTAUpdateManager.hpp`, immediately after the `#pragma once` block and includes, add two
`constexpr` constants inside the file's anonymous namespace or at file scope (before the class
declaration):

```cpp
/** Stack size (bytes) for the OTA FreeRTOS task. TLS requires 12–16 KB minimum;
 *  32 KB provides comfortable headroom for certificate chains and redirect chains.
 *  Used in performUpdate() and exposed via the status API for portal diagnostics. */
static constexpr uint32_t OTA_TASK_STACK_BYTES = 32768;

/** Minimum free DRAM headroom required before attempting an OTA task launch.
 *  If free DRAM < (OTA_TASK_STACK_BYTES + OTA_TASK_DRAM_HEADROOM_BYTES),
 *  the pre-flight guard will log a diagnostics message and attempt PSRAM fallback. */
static constexpr uint32_t OTA_TASK_DRAM_HEADROOM_BYTES = 8192;
```

### Step 2 — Add a pre-flight DRAM check + PSRAM stack allocation (`performUpdate()` in `.cpp`)

Locate the `xTaskCreate(...)` call in `performUpdate()` (approximately line 360). Replace the
entire `xTaskCreate` call and its error check with the following logic (adapt style to match
surrounding code):

```cpp
#ifndef NATIVE_BUILD
    // --- Pre-flight: DRAM headroom check ---
    uint32_t freeDram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t required = OTA_TASK_STACK_BYTES + OTA_TASK_DRAM_HEADROOM_BYTES;
    if (freeDram < required) {
        ESP_LOGW(TAG,
            "Low DRAM before OTA task launch — free: %" PRIu32 " B, "
            "required: %" PRIu32 " B. Attempting PSRAM stack allocation.",
            freeDram, required);
    }

    // --- Launch OTA task — stack in PSRAM when available, DRAM otherwise ---
    // xTaskCreateWithCaps allocates the task stack from memory matching the
    // supplied MemoryCaps mask. MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT targets
    // PSRAM on S3/S3-Nano. On boards without PSRAM the allocator falls back
    // to DRAM transparently (standard ESP-IDF behaviour).
    BaseType_t result = xTaskCreateWithCaps(
        otaTaskWrapper,                            // Task function
        "ota_update",                              // Task name
        OTA_TASK_STACK_BYTES,                      // Stack size (bytes)
        this,                                      // Parameter
        5,                                         // Priority
        &taskHandle_,                              // Task handle output
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT        // Prefer PSRAM stack
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG,
            "Failed to create OTA FreeRTOS task — "
            "free DRAM: %" PRIu32 " B, stack required: %" PRIu32 " B. "
            "Device may need reboot to recover heap.",
            freeDram, OTA_TASK_STACK_BYTES);
        state_ = OTAState::Error;
        lastOtaError_ = "Failed to create OTA task (insufficient memory)";
        publishProgress(0.0f, "Failed to create OTA task (insufficient memory)", true, false);
    }
#else
    // Native builds cannot run FreeRTOS tasks
    ESP_LOGW(TAG, "performUpdate() — no FreeRTOS on native build, setting Error");
    state_ = OTAState::Error;
    lastOtaError_ = "FreeRTOS not available on native build";
#endif
```

**Important**: `xTaskCreateWithCaps` is declared in `esp_heap_caps.h` (included transitively
via Arduino.h on ESP32) and in `freertos/idf_additions.h`. Verify the include path compiles.
If the symbol is unavailable, check `#include "freertos/idf_additions.h"` — add it only if
needed and only inside the `#ifndef NATIVE_BUILD` guard.

### Step 3 — Expose readiness fields in the `status` action handler (`.cpp`)

In the `strcmp(action, "status") == 0` block (approximately line 1191), add two new fields to
the `resp` JSON document **before** `serializeJson`:

```cpp
// OTA task readiness fields — used by portal to render low-memory warning
resp["dram_free_bytes"]  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
resp["ota_stack_bytes"]  = OTA_TASK_STACK_BYTES;
```

These two lines go after the existing `resp["checkHour"] = ...` line and before
`String output; serializeJson(resp, output);`.

### Step 4 — Add the `#include` for `inttypes.h` if `PRIu32` is not already available

Search for `#include <inttypes.h>` in the `.cpp`. If absent, add it inside the
`#ifndef NATIVE_BUILD` include block at the top of the file.

---

## Verification Requirement

**Read `.agents/skills/pio-manager/SKILL.md` before running any PlatformIO command.**
All PlatformIO operations MUST go through the MCP server (Tier 1).

### Phase A — Compile

1. `mcp_platformio_build_project` with `environment: "native"`, `background: true`
   → poll with `check_task_status` until complete → must be zero errors, zero new warnings.
2. `mcp_platformio_build_project` with `environment: "esp32s3nano"`, `background: true`
   → poll until complete → must be zero errors, zero new warnings.

### Phase B — Flash to device

3. Run `mcp_platformio_list_devices` to find the correct port for the attached esp32s3nano.
4. `mcp_platformio_upload_firmware` with:
   - `environment: "esp32s3nano"`
   - `port: <discovered port>`
   - `background: true`
   - `start_monitor: true`   ← automatically restarts monitor after flash
5. Poll `check_task_status` until upload completes.

### Phase C — Live device verification

6. `mcp_platformio_query_logs` with `searchPattern: "OTA|ota|heap|PSRAM|task"` to capture
   startup and any OTA-related log lines.
7. Confirm the device boots cleanly (no panic, no TWDT, no abort).
8. If the device is reachable via HTTP, trigger a manual OTA status check:
   `POST /api/ota {"action":"status"}` and confirm `dram_free_bytes` and `ota_stack_bytes`
   fields are present in the response.
   (You can use `curl` or the portal — whichever is quickest.)

### Phase D — Release lock

9. Call `mcp_platformio_release_lock` to free the hardware queue after all operations complete.

---

## Done Condition

- Both environments (`native`, `esp32s3nano`) compile clean.
- Firmware flashed to the attached esp32s3nano with zero boot errors.
- `dram_free_bytes` and `ota_stack_bytes` present in `/api/ota status` response.
- Serial monitor confirms device is healthy post-flash.
- **STOP and send a message back to me (the Principal Architect)** with:
  - Build results (both envs)
  - Flash result
  - Device boot log excerpt
  - `/api/ota status` JSON excerpt showing the two new fields
  Do not invoke `@distillery` or wrap up until I explicitly approve the work.

Once approved, invoke the `@distillery` skill passing
`.agents/plugins/principal-architect-workspace/pr-21-ota-task-hardening/wo1-firmware-heap-guard/`
as the target directory. Then append a status update to this WORKORDER.md and terminate.

---

## STATUS — COMPLETE

**Completed:** 2026-06-11T20:53:00Z  
**Commit:** `175dfc8` on branch `pr-21-ota-task-hardening-wo1-heap-guard`  
**Session:** `01f8e568-4763-4787-a03d-a4a00a32cd26`  
**Approved by:** Principal Architect (8c58afa2)

### Checklist

- [x] Step 1 — `OTA_TASK_STACK_BYTES = 32768` and `OTA_TASK_DRAM_HEADROOM_BYTES = 8192` added to `.hpp`
- [x] Step 2 — Pre-flight DRAM check + `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` in `performUpdate()`
- [x] Step 3 — `dram_free_bytes` and `ota_stack_bytes` fields added to `status` action handler
- [x] Step 4 — `#include <inttypes.h>` and `#include <freertos/idf_additions.h>` added inside `#ifndef NATIVE_BUILD`

### Verification Summary

| Phase | Result |
|-------|--------|
| native run_tests | ✅ 525/525 PASS |
| esp32s3nano build | ✅ SUCCESS — 0 errors, 0 new warnings |
| Flash | ✅ Wrote 1,222,560 bytes, hash verified |
| Boot | ✅ `Firmware self-test PASSED` — 8.6 MB heap free |
| /api/ota status | ⚠️ Host not on device AP — verified by code inspection and PA approval |

### Distillery

Context archived to: `.agents/plugins/principal-architect-workspace/pr-21-ota-task-hardening/wo1-firmware-heap-guard/`
- `context_bridge.md` — Index and end-state
- `sessions.md` — Session registry
- `context_artifacts/adr_psram_task_allocation.md` — Design decisions
- `context_artifacts/graveyard_native_build.md` — Failure traces and mitigations
