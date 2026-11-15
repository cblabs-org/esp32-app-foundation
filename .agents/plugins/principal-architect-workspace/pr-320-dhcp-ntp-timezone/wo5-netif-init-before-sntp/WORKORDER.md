---
schema_version: 1
wo_id: WO-320.5
pr_id: PR-320
title: Initialize esp_netif before esp_sntp calls in TimeManager::_reinitNtp() to
  prevent boot abort
complexity: standard
status: COMPLETED
allowed_files:
- modules/TimeManager/TimeManager.cpp
- test/stubs/platform/esp_netif.h
- test/stubs/platform/esp_sntp.h
- test/test_unit/test_timeManager/test_timeManager.cpp
forbidden_files:
- docs/design_specification.md
required_skills:
- embedded-systems
- framework-expert
- pio-manager
verification_command: python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 63bf3e0a3af673f2936912ed6f54d7b5b252a8a2360abb9d98c08fed087cf0b0
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_b9a095767923
  commit_sha: 698fef9af632782aa0aa1db6516aff62ff9fd596
  files_modified:
  - modules/TimeManager/TimeManager.cpp
  - test/stubs/platform/esp_netif.h
  - test/stubs/platform/esp_sntp.h
  - test/test_unit/test_timeManager/test_timeManager.cpp
  verification_summary: doc_lint.py exited 0; native tests 1020/1020 passed (including
    BeginInitializesNetifBeforeAnySntpCalls); esp32s3nano, esp32dev, and crowpanel-128-rotary
    MCP builds succeeded.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Added host stub test/stubs/platform/esp_netif.h and instrumented test/stubs/platform/esp_sntp.h
    (SntpStubState + configTzTime) to detect any esp_sntp_* call preceding esp_netif_init().
  - Included <esp_netif.h> in TimeManager.cpp, invoked (void)esp_netif_init() at the
    very top of TimeManager::_reinitNtp(), and moved esp_sntp_servermode_dhcp() ahead
    of the synchronous esp_sntp_setserver() slot-clearing loop so tcpip_callback messages
    drain before configTzTime().
  - Added TimeManagerTest.BeginInitializesNetifBeforeAnySntpCalls verifying pre-begin,
    post-begin(), and post-POST /api/time/config netif initialization ordering and
    count increments.
  dead_ends: []
---

# WO-320.5 — Initialize esp_netif before esp_sntp calls in TimeManager::_reinitNtp() to prevent boot abort

## 1. Objective & Scope

Resolve the boot-time abort observed on `esp32s3nano` (`v1.0.0-dev.2`) where `TimeManager::begin()` calls `TimeManager::_reinitNtp()` and invokes `esp_sntp_setserver()` / `esp_sntp_servermode_dhcp()` before `esp_netif_init()` has initialized LwIP's `tcpip_thread` and `tcpip_mbox`. Ensure `esp_netif_init()` is invoked before any `esp_sntp_*` calls, order `esp_sntp_servermode_dhcp()` ahead of the synchronous `esp_sntp_setserver()` slot-clearing loop, and cover this initialization ordering invariant in host-native stubs and unit tests.

## 2. Detailed Specification

### 2.1 Host Stub: `test/stubs/platform/esp_netif.h`
Create `test/stubs/platform/esp_netif.h` with:
- `#pragma once`
- `#include "esp_sntp.h"`
- `#ifndef ESP_OK` -> `#define ESP_OK 0` -> `#endif`
- `typedef int esp_err_t;`
- `inline esp_err_t esp_netif_init(void)`:
  ```cpp
  inline esp_err_t esp_netif_init(void) {
      stub_sntp_state().netifInitialized = true;
      ++stub_sntp_state().netifInitCount;
      return ESP_OK;
  }
  ```

### 2.2 Host Stub Ordering Instrumentation: `test/stubs/platform/esp_sntp.h`
Update `test/stubs/platform/esp_sntp.h`:
1. Add three fields to `SntpStubState`:
   - `bool netifInitialized = false;`
   - `bool sntpCalledBeforeNetifInit = false;`
   - `size_t netifInitCount = 0;`
2. Reset all three fields in `stub_sntp_reset()`:
   - `st.netifInitialized = false;`
   - `st.sntpCalledBeforeNetifInit = false;`
   - `st.netifInitCount = 0;`
3. In `esp_sntp_stop`, `esp_sntp_setserver`, `esp_sntp_setservername`, and `esp_sntp_servermode_dhcp`:
   - If `!stub_sntp_state().netifInitialized`, set `stub_sntp_state().sntpCalledBeforeNetifInit = true;` before updating slot/enabled/dhcp state.
4. At the top of `configTzTime(const char* tz, const char* server1, const char* server2, const char* server3)`:
   - Set `stub_sntp_state().netifInitialized = true;`
   - Increment `++stub_sntp_state().netifInitCount;`
   (Matching ESP-IDF / Arduino-ESP32 `configTzTime` behavior while still catching any `esp_sntp_*` call made *prior* to `esp_netif_init()` or `configTzTime()`.)

### 2.3 Firmware Fix: `modules/TimeManager/TimeManager.cpp`
1. Include `<esp_netif.h>` in the platform include section of `modules/TimeManager/TimeManager.cpp` (available on both ESP-IDF hardware builds and `env:native` via `test/stubs/platform/esp_netif.h`).
2. In `TimeManager::_reinitNtp()`:
   - Call `(void)esp_netif_init();` at the very top of `_reinitNtp()` before any `esp_sntp_*` calls (`esp_sntp_enabled()`, `esp_sntp_stop()`, `esp_sntp_servermode_dhcp()`, `esp_sntp_setserver()`, `configTzTime()`).
   - Place the `#if defined(CONFIG_LWIP_DHCP_GET_NTP_SRV) && CONFIG_LWIP_DHCP_GET_NTP_SRV` block calling `esp_sntp_servermode_dhcp(_dhcpNtpEnabled ? 1 : 0);` **before** the `for (size_t i = 0; i < EFFECTIVE_SNTP_SLOTS; ++i) { esp_sntp_setserver(static_cast<uint8_t>(i), nullptr); }` loop so the synchronous `tcpip_api_call` inside `esp_sntp_setserver()` drains preceding async `tcpip_callback` messages before `configTzTime(...)` runs.

### 2.4 Unit Test: `test/test_unit/test_timeManager/test_timeManager.cpp`
Add `TEST_F(TimeManagerTest, BeginInitializesNetifBeforeAnySntpCalls)` verifying:
1. Immediately after fixture setup (prior to `begin()`): `netifInitialized == false`, `sntpCalledBeforeNetifInit == false`, `netifInitCount == 0`.
2. After `timeManager.begin()`: `netifInitialized == true`, `sntpCalledBeforeNetifInit == false`, and `netifInitCount >= 1u`.
3. After registering routes and invoking `POST /api/time/config` (triggering `_reinitNtp()` again, e.g. updating timezone/NTP or `dhcpNtp`): `netifInitialized == true`, `sntpCalledBeforeNetifInit == false`, and `netifInitCount` increased strictly beyond its post-`begin()` count.

## 3. Verification Requirements

1. Run `python3 scripts/doc_lint.py` and confirm exit code `0`.
2. Run PlatformIO MCP `run_tests` (`environment: "native"`, root project; ALWAYS `acquire_lock` -> `run_tests` -> `release_lock`, never shell `pio`!) and verify all unit tests pass including `BeginInitializesNetifBeforeAnySntpCalls`.
3. Run PlatformIO MCP `build_project` on `esp32s3nano` and `esp32dev` (ALWAYS `acquire_lock` -> `build_project` -> `release_lock`) and verify clean compilation with `<esp_netif.h>` and `esp_netif_init()`.
4. Confirm `git status --porcelain` shows only your `allowed_files` (plus this `WORKORDER.md` once Part II is updated).
5. Record your work summary in Part II (`completion.verification_summary`, `completion.decisions_made`, and the final report section below — do NOT repeat that section's exact heading anywhere in Part I prose) BEFORE running `pa wo verify WO-320.5` as your final mutating action.

## Completion Report

### Execution Summary
- Created `test/stubs/platform/esp_netif.h` providing host-native `esp_netif_init()` that records `netifInitialized = true` and increments `netifInitCount` in `stub_sntp_state()`.
- Updated `test/stubs/platform/esp_sntp.h` (`SntpStubState`, `stub_sntp_reset()`, `esp_sntp_stop()`, `esp_sntp_setserver()`, `esp_sntp_setservername()`, `esp_sntp_servermode_dhcp()`, and `configTzTime()`) to track whether any SNTP API was called prior to `esp_netif_init()`.
- Updated `modules/TimeManager/TimeManager.cpp` to `#include <esp_netif.h>` across both ESP-IDF and `env:native`, call `(void)esp_netif_init();` at the very top of `TimeManager::_reinitNtp()` before any `esp_sntp_*` calls, and invoke `esp_sntp_servermode_dhcp(_dhcpNtpEnabled ? 1 : 0)` ahead of the synchronous `esp_sntp_setserver()` slot-clearing loop so `tcpip_callback` messages drain on `tcpip_mbox` before `configTzTime()` executes.
- Added `TEST_F(TimeManagerTest, BeginInitializesNetifBeforeAnySntpCalls)` in `test/test_unit/test_timeManager/test_timeManager.cpp` validating netif initialization ordering before `begin()`, after `begin()`, and after `POST /api/time/config`.

### Verification Results
- **Doc Lint (`python3 scripts/doc_lint.py`):** Exited `0` with zero errors.
- **Native Unit Tests (`platformio-mcp_platformio` `run_tests`, `environment: "native"`):** `SUCCESS` — all 36 suites passed (`1020 / 1020` test cases succeeded in `82.48s`), including `TimeManagerTest.BeginInitializesNetifBeforeAnySntpCalls`.
- **Target Firmware Builds (`platformio-mcp_platformio` `acquire_lock` -> `build_project` -> `release_lock`, `sessionId: "wo-320-5-bd4cda46"`):**
  - `esp32s3nano` (`forceExecution: true`): `SUCCESS` (`cacheHit: false`, RAM `61,084` bytes, Flash `1,381,143` bytes).
  - `esp32dev`: `SUCCESS` (`cacheHit: false`, RAM `58,460` bytes, Flash `1,346,835` bytes).
  - `crowpanel-128-rotary`: `SUCCESS` (`cacheHit: false`, RAM `64,828` bytes, Flash `1,913,691` bytes).
  - Hardware lock explicitly released.
- **Boundary & Spec Integrity:** `git status --short` contains only the 4 `allowed_files` plus this `WORKORDER.md` Part II update, and Part I `spec_hash` (`63bf3e0a3af673f2936912ed6f54d7b5b252a8a2360abb9d98c08fed087cf0b0`) is verified intact.

### Escalation Notice
`pa wo verify WO-320.5` was intentionally skipped per instructions (`HARNESS_PORCELAIN_LSTRIP_BUG`), because `pa_cli.py:_parse_porcelain_paths()` strips the leading dot from `.agents/.../WORKORDER.md`. Ready for parent PA receipt stamping and commit creation.
