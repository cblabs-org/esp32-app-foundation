---
name: open-issues-batch-wo3
description: "Handoff instructions for Work Order 3: PSRAM configuration guard + build flag cleanup (Issue #36)"
complexity: standard
---

# WO3 — Issue #36: PSRAM Configuration Guard & Build Flag Cleanup

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Prevent PSRAM from being silently disabled when a consumer's `sdkconfig.defaults` omits the `CONFIG_SPIRAM=y` master switch while `BOARD_HAS_PSRAM` is defined. Add both compile-time and runtime guards, and remove the inert ESP32-classic flag from the S3 build.

## Parent Project

`pr-46-open-issues-batch` — user approved the decomposition on 2026-07-22. See `PROJECT.md` in the parent directory.

## Strict Boundary Box

- You MUST ONLY modify the files listed in the checklist below.
- You MUST NOT modify any web portal files (JS, HTML, CSS).
- You MUST NOT modify `sdkconfig.defaults.esp32s3nano` — the framework's own config is correct.
- You MUST NOT add any new interfaces or change any public API.
- The compile-time guard MUST use `#error` (not `#warning`) — the user confirmed fail-loud is the desired behaviour.
- Surface any out-of-scope findings to the PA in your completion message.

## Tooling Constraints

- Use the `platformio` MCP tools for all build operations (not raw `pio` CLI).
- Build environments to test: `native` and `esp32s3nano`.

## Line-Item Instructions

- [ ] **1. Remove inert S3 flag from `platformio.ini`**: Open `platformio.ini` and locate the `[env:esp32s3nano]` section. Remove the line `-mfix-esp32-psram-cache-issue` from `build_flags`. This flag is an ESP32-classic rev 0/1 hardware workaround and is inert on the S3. Leave `-DBOARD_HAS_PSRAM` in place.

- [ ] **2. Add compile-time PSRAM guard**: Add a compile-time preprocessor check that fires `#error` when `BOARD_HAS_PSRAM` is defined but `CONFIG_SPIRAM` is not. The best location is in `modules/Application/Application.cpp` near the top (after includes), since this is the framework's entry point:

```cpp
// --- PSRAM configuration guard (Issue #36) ---
// BOARD_HAS_PSRAM is an Arduino-layer hint set in platformio.ini build_flags.
// It does NOT enable the ESP-IDF SPIRAM driver — CONFIG_SPIRAM=y in sdkconfig
// is required for that. Catch the misconfiguration at compile time.
#if defined(BOARD_HAS_PSRAM) && !defined(CONFIG_SPIRAM)
  #error "BOARD_HAS_PSRAM is defined but CONFIG_SPIRAM is not enabled in sdkconfig. " \
         "PSRAM will be silently disabled. Add CONFIG_SPIRAM=y to your sdkconfig.defaults."
#endif
```

**Important**: This guard must be wrapped in `#ifndef NATIVE_BUILD` or placed after the existing platform-detection logic, because the `native` test environment does not define `CONFIG_SPIRAM` and does not set `BOARD_HAS_PSRAM`. Check the existing code to determine how NATIVE_BUILD is handled — if stubs exist, use the same pattern. The guard should only fire on ESP32 target builds.

- [ ] **3. Add runtime PSRAM boot check**: In `Application::begin()` in `modules/Application/Application.cpp`, add a runtime check early in the boot sequence (after logging starts but before module registration):

```cpp
#if defined(BOARD_HAS_PSRAM)
    // Runtime PSRAM initialization check (Issue #36)
    // Catches the case where CONFIG_SPIRAM is set but PSRAM physically
    // failed to initialize (e.g., hardware fault, timing issue).
    if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0) {
        ESP_LOGE(TAG, "PSRAM expected (BOARD_HAS_PSRAM) but none detected! "
                      "Check CONFIG_SPIRAM=y and PSRAM hardware.");
    }
#endif
```

You may need to add `#include "esp_heap_caps.h"` if not already included. Check what headers are already present. The `MALLOC_CAP_SPIRAM` constant is from `esp_heap_caps.h`.

- [ ] **4. Verify the native build passes** (native env does not define BOARD_HAS_PSRAM, so the guard must not fire):
  Use the `platformio` MCP tool `build_project` with environment `native`.

- [ ] **5. Verify the esp32s3nano build passes** (this env has both BOARD_HAS_PSRAM and CONFIG_SPIRAM=y, so the guard must not fire):
  Use the `platformio` MCP tool `build_project` with environment `esp32s3nano`.

- [ ] **6. Update build-system documentation**: Open `.agents/skills/framework-expert/references/build-system.md` and add a note in the relevant section about:
  - `BOARD_HAS_PSRAM` is an Arduino-layer hint, NOT the IDF driver enable
  - `CONFIG_SPIRAM=y` in sdkconfig is the required master switch
  - The compile-time guard will `#error` if `BOARD_HAS_PSRAM` is set without `CONFIG_SPIRAM`
  - `CONFIG_SPIRAM_*` sub-options are inert without the master switch

## Verification Requirement

After completing all checklist items:
1. Confirm native build passes (all tests green).
2. Confirm esp32s3nano build passes (no compile errors).
3. **STOP and ask the user** if they are happy with the changes.
4. Only after user approval: commit your changes with message `fix(build): add PSRAM config guard, remove inert S3 flag (Issue #36)`, then invoke the `@distillery` skill to package the state, passing `.agents/plugins/principal-architect-workspace/pr-46-open-issues-batch/wo3-psram-guard/` as the target directory.
5. Append a status update to this WORKORDER.md and send a completion message to the PA.

## Status Update

| Date | Event |
|------|-------|
| 2026-07-22 | Worker agent executed all 6 checklist items |
| 2026-07-22 | native: 423/423 tests passed ✅ |
| 2026-07-22 | esp32s3nano: firmware compiled successfully (17.1% RAM, 32.1% Flash) ✅ |
| 2026-07-22 | User approved changes |
| 2026-07-22 | Committed as `73b5b7f` on branch `open-issues-batch/wo3-psram-guard` |
| 2026-07-22 | Distillery output written to `context_bridge.md` + `context_artifacts/` |
| 2026-07-22 | **COMPLETE** |
