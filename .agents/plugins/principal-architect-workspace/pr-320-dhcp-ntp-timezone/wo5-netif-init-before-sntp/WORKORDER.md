---
schema_version: 1
wo_id: WO-320.5
pr_id: PR-320
title: Initialize esp_netif before esp_sntp calls in TimeManager::_reinitNtp() to
  prevent boot abort
complexity: standard
status: PENDING
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
completion: null
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

_Populated by pa-engineer upon completion._
