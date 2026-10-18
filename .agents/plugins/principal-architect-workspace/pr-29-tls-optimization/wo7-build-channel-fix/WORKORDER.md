---
name: pr-29-wo7
description: Handoff instructions for Work Order 7 — Add BUILD_CHANNEL string macro and verify version comparison
complexity: mechanical
---

# Work Order 7 — Add `BUILD_CHANNEL` String Macro & Verify Version Comparison

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add a `BUILD_CHANNEL` string macro to `inject_version.py` (complementing the
existing numeric `VERSION_CHANNEL`) and verify that the OTA version comparison
correctly handles beta-channel builds. The numeric `VERSION_CHANNEL` macro
already exists and is used correctly in `OTAUpdateManager.cpp` via
`static_cast<Channel>(VERSION_CHANNEL)` — this WO adds the string variant
for logging and diagnostic purposes.

## Versioning

Read `.agents/rules/03-versioning.md`. This is a minor tooling enhancement —
no version tag increment needed unless the worker adds logging changes to firmware.

## Strict Boundary Box

- You **MAY** modify `scripts/inject_version.py`
- You **MAY** modify `modules/OTAUpdateManager/OTAUpdateManager.cpp` (for logging only)
- Do **NOT** modify `SemVer.hpp` or `SemVer.cpp`
- Do **NOT** modify `sdkconfig.defaults`, `platformio.ini`, or partition tables
- Do **NOT** create any branches or make git commits

## Background

### Current State (already working)

`inject_version.py` already injects these build flags:
```
-DVERSION_MAJOR=0
-DVERSION_MINOR=0
-DVERSION_PATCH=7
-DVERSION_CHANNEL=1        ← numeric (0=dev, 1=beta, 2=rc, 3=stable)
-DVERSION_PRE_NUM=1
-DBUILD_VERSION="v0.0.7-beta.1"
```

`OTAUpdateManager.cpp` uses `static_cast<Channel>(VERSION_CHANNEL)` at lines
302-303 and 496-498 for SemVer comparison — this is correct.

### What's Missing

There is no `BUILD_CHANNEL` **string** macro (e.g., `"beta"`). This would be
useful for:
- Log messages: `ESP_LOGI(TAG, "Running on %s channel", BUILD_CHANNEL);`
- MCP diagnostics / system_info responses
- Web portal display of current channel

The variable is already available in `inject_version.py` as `chan_str`
(line 209) — it's `None` for stable, `"dev"`/`"beta"`/`"rc"` for pre-releases.

## Line-Item Instructions

### Step 1: Add BUILD_CHANNEL string to inject_version.py

- [ ] **1a.** Read `scripts/inject_version.py` to confirm the current state.

- [ ] **1b.** In the `filtered.extend()` block (~line 243-251), add:
  ```python
  f'-DBUILD_CHANNEL=\\\\"{chan_str or "stable"}\\\\"',
  ```

- [ ] **1c.** In the filter prefix list (~line 228-239), add:
  ```python
  "-DBUILD_CHANNEL=",
  ```
  This ensures any existing `BUILD_CHANNEL` flag is stripped before the new one
  is injected.

- [ ] **1d.** Check the `native` test environment handling. The native env
  typically uses a static fallback in `platformio.ini` since `inject_version.py`
  only runs for hardware envs. If the native env defines `BUILD_VERSION` as a
  static fallback, add a corresponding `BUILD_CHANNEL` fallback:
  ```ini
  -DBUILD_CHANNEL=\"stable\"
  ```

### Step 2: Add diagnostic logging (optional but recommended)

- [ ] **2a.** In `OTAUpdateManager.cpp`, find the boot log message that prints
  the current firmware version. If it doesn't already log the channel, add
  `BUILD_CHANNEL` to the log:
  ```cpp
  ESP_LOGI(TAG, "Firmware: %s (channel: %s)", BUILD_VERSION, BUILD_CHANNEL);
  ```

- [ ] **2b.** Add a `#ifndef BUILD_CHANNEL` guard near the top of
  `OTAUpdateManager.cpp` for safety:
  ```cpp
  #ifndef BUILD_CHANNEL
  #define BUILD_CHANNEL "stable"
  #endif
  ```

### Step 3: Build and verify

- [ ] **3a.** Build for esp32s3nano:
  ```bash
  pio run -e esp32s3nano
  ```
  Confirm no compile errors. Check the build output for `BUILD_CHANNEL`.

- [ ] **3b.** Build for esp32dev:
  ```bash
  pio run -e esp32dev
  ```
  Confirm no compile errors.

- [ ] **3c.** Run native tests:
  ```bash
  pio test -e native
  ```
  All tests must pass.

### Step 4: Verify version comparison correctness

- [ ] **4a.** Confirm `VERSION_CHANNEL` is correctly injected by examining the
  build flags in the PlatformIO build output. For a stable tag, it should be
  `VERSION_CHANNEL=3`. For a beta tag, it should be `VERSION_CHANNEL=1`.

- [ ] **4b.** Verify mentally that the SemVer comparison at lines 302-304 and
  496-498 of `OTAUpdateManager.cpp` produces correct results:
  - Beta build (`VERSION_CHANNEL=1`) vs beta remote → compares correctly ✅
  - Beta build (`VERSION_CHANNEL=1`) vs stable remote → stable wins ✅
  - Stable build (`VERSION_CHANNEL=3`) vs beta remote → stable wins ✅

## Verification Requirement

After completing all checklist items:
1. Confirm both boards build cleanly with `BUILD_CHANNEL` in the flags
2. Confirm native tests pass
3. Confirm `inject_version.py` emits `BUILD_CHANNEL` string alongside `VERSION_CHANNEL` numeric
4. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
