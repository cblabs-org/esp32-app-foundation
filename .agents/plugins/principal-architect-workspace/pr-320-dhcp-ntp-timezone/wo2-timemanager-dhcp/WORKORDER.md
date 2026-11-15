---
schema_version: 1
wo_id: WO-320.2
pr_id: PR-320
title: Implement DHCP NTP & Timezone in TimeManager, MCP telemetry, main wiring, and
  native tests
complexity: architect
status: COMPLETED
allowed_files:
- modules/TimeManager/TimeManager.hpp
- modules/TimeManager/TimeManager.cpp
- modules/TimeManager/TimePreconditionContributor.hpp
- modules/MCPServer/MCPBuiltinTools.cpp
- src/main.cpp
- sample-app/src/main.cpp
- template-app/src/main.cpp
- test/stubs/platform/esp_sntp.h
- test/stubs/platform/freertos/FreeRTOS.h
- test/test_unit/test_timeManager/**
- test/test_unit/test_mcpServer/**
forbidden_files:
- docs/design_specification.md
required_skills:
- embedded-systems
- framework-expert
- pio-manager
verification_command: python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: e77b2ac4601c2910039495b3bd815a3112ab8019b64e37bb5b97b7af7d6ba310
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_1cc38419b229
  commit_sha: 41bb5fb7c147decdd82b4be69bc3409e2cd49cba
  files_modified:
  - modules/MCPServer/MCPBuiltinTools.cpp
  - modules/TimeManager/TimeManager.cpp
  - modules/TimeManager/TimeManager.hpp
  - sample-app/src/main.cpp
  - src/main.cpp
  - template-app/src/main.cpp
  - test/stubs/platform/esp_sntp.h
  - test/stubs/platform/freertos/FreeRTOS.h
  - test/test_unit/test_mcpServer/test_mcpServer.cpp
  - test/test_unit/test_timeManager/test_timeManager.cpp
  verification_summary: doc_lint.py ok; native 1019/1019 (36 suites); builds ok (root
    esp32dev/s3nano/crowpanel, sample all_on/all_off); nm strong lwip_dhcp hooks ok.
    Blocked on pa wo verify .lstrip('./') bug.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Implemented strong extern "C" lwip_dhcp_on_extra_option and lwip_dhcp_tz_take_pending
    behind a static portMUX_TYPE spinlock in TimeManager.cpp, filtering strictly for
    accepted lease states (REQUESTING/RENEWING/REBINDING/REBOOTING/BOUND), Options
    100/101 only (never Option 2), printable ASCII, and bounded length (<64), with
    zero heap/VFS/EventBus/TZ/logging inside the hook.
  - 'Guarded esp_sntp_servermode_dhcp() behind #if defined(CONFIG_LWIP_DHCP_GET_NTP_SRV)
    && CONFIG_LWIP_DHCP_GET_NTP_SRV and bounded SNTP slot iteration via EFFECTIVE_SNTP_SLOTS
    (min(MAX_SNTP_SLOTS, CONFIG_LWIP_SNTP_MAX_SERVERS)) so boards with SNTP_MAX_SERVERS=1
    and DHCP SNTP disabled (esp32s3nano, crowpanel-128-rotary) compile and run safely.'
  - Subscribed directly to the "wifi.connected" EventBus topic literal inside TimeManager.cpp
    to avoid introducing an LDF header dependency on WifiManager.hpp, matching HomeAssistant
    and decoupling display test targets.
  - Implemented a fixed-256-byte-buffer streaming JSON key/value parser in TimeManager::_lookupIanaPosixTz()
    to resolve Option 101 TCode IANA names from LittleFS /timezones.json without heap
    allocation.
  - Left TimePreconditionContributor.hpp unmodified after confirming it already reads
    TimeManager::isConfigured() and TimeManager::getSyncStatus().
  dead_ends: []
---

# WO-320.2 — Implement DHCP NTP & Timezone in TimeManager, MCP telemetry, main wiring, and native tests

## 1. Objective & Scope

Implement complete DHCP NTP (RFC 2132 Option 42) and DHCP Timezone (RFC 4833 Options 100 `PCode` & 101 `TCode`) runtime handling in `TimeManager`, upgrade `IConfigurable` config schema to v2 with v1->v2 migration, expose live DHCP state in REST `/api/time/config` and MCP `"system_info"`, wire `setEventBus(&app.getEventBus())` across all three apps (`src/main.cpp`, `sample-app/src/main.cpp`, `template-app/src/main.cpp`), upgrade host-native stubs, and add thorough GoogleTest unit tests.

## 2. Architecture & Design Contracts

### 2.1 LwIP Hook Implementation & Thread-Safe Spinlock Handoff (`TimeManager.cpp` / `include/lwip_dhcp_tz_hooks.h`)
- Inside `modules/TimeManager/TimeManager.cpp`, provide the strong `extern "C"` LwIP hook overriding ESP-IDF's `__weak` symbol in `lwip_default_hooks.c` (plus host-unit-testable helpers for native GoogleTests):
  ```cpp
  extern "C" void lwip_dhcp_on_extra_option(
      struct dhcp *dhcp, uint8_t state, uint8_t option,
      uint8_t len, struct pbuf* p, uint16_t offset);
  extern "C" bool lwip_dhcp_tz_take_pending(
      char* outPcode, size_t pcodeCap, bool* gotPcode,
      char* outTcode, size_t tcodeCap, bool* gotTcode);
  ```
- Guard shared static staging state (`char pcode[LWIP_DHCP_TZ_MAX_LEN]`, `char tcode[LWIP_DHCP_TZ_MAX_LEN]`, `bool hasPcode`, `bool hasTcode`) with `static portMUX_TYPE s_dhcpTzMux = portMUX_INITIALIZER_UNLOCKED;` using `portENTER_CRITICAL(&s_dhcpTzMux)` / `portEXIT_CRITICAL(&s_dhcpTzMux)`.
  - Note: Include `#include <freertos/FreeRTOS.h>` and `#include <freertos/task.h>`, and add matching no-op `portMUX_TYPE`, `portMUX_INITIALIZER_UNLOCKED`, `portENTER_CRITICAL(mux)`, `portEXIT_CRITICAL(mux)` stubs to `test/stubs/platform/freertos/FreeRTOS.h` for `native` builds.
  - On `ESP_PLATFORM` (and `!defined(UNIT_TEST)`), include `"lwip/pbuf.h"` and `"lwip/prot/dhcp.h"`. On `UNIT_TEST` / native, either define minimal `struct pbuf`, `pbuf_copy_partial`, and `DHCP_STATE_*` constants inside `test/stubs/platform/esp_sntp.h` (or a small stub section included on `UNIT_TEST`) so `lwip_dhcp_on_extra_option(...)` compiles and runs directly under GoogleTest!
- **Hook Invariants (LwIP TCP/IP context):**
  1. Filter `state`: only accept `DHCP_STATE_REQUESTING`, `DHCP_STATE_RENEWING`, `DHCP_STATE_REBINDING`, `DHCP_STATE_REBOOTING`, `DHCP_STATE_BOUND`. Ignore `DHCP_STATE_SELECTING` (raw `DHCPOFFER` before lease acceptance) and other transient states.
  2. Accept only `option == LWIP_DHCP_OPTION_TZ_PCODE` (100) or `option == LWIP_DHCP_OPTION_TZ_TCODE` (101).
  3. Reject `len == 0` or `len >= LWIP_DHCP_TZ_MAX_LEN` or `p == nullptr`.
  4. Extract payload into a small stack buffer via `pbuf_copy_partial(p, tmp, len, offset) == len`, null-terminate, and validate printable ASCII (`0x20 <= ch && ch <= 0x7E` for every byte; reject control chars / embedded NULs / leading-or-trailing whitespace-only strings).
  5. **ZERO** heap allocations (`malloc`/`new`/`String`), LittleFS I/O, `ConfigManager` calls, `setenv()`, `tzset()`, or `ESP_LOG*` heavy formatting inside the spinlock / hook!
  6. Note: RFC 2132 Option 2 (`Time Offset`) MUST NOT be implemented.

### 2.2 Config Schema v2 & Zero Flash Wear (`TimeManager.hpp` / `TimeManager.cpp`)
- `getSchemaVersion()` -> `return 2;`
- Persistent config fields (serialized in `/config.json` & `/mod/time.json`):
  - Existing: `timezone`, `posix`, `ntpServer1`, `ntpServer2`, `ntpServer3`
  - New: `bool dhcpNtp` (`_dhcpNtpEnabled`, default `true`), `bool dhcpTz` (`_dhcpTzEnabled`, default `true`)
- `resetConfig()`:
  - Keep existing defaults (`_timezone = ""`, `_posixTz = "GMT0"`, `pool.ntp.org`, `time.nist.gov`, `time.windows.com`, `_configured = false`).
  - Set `_dhcpNtpEnabled = true; _dhcpTzEnabled = true;`
  - Clear transient runtime state: `_dhcpNtpActive = false; _dhcpNtpCount = 0; memset(_dhcpNtpServers, ...); _dhcpTzActive = false; _dhcpTzSource = DhcpTzSource::None; _dhcpTimezone[0] = '\0'; _dhcpPosixTz[0] = '\0';`
- `migrateConfig(JsonObject& section, int fromVersion)`:
  - When `fromVersion < 2`:
    - If `section["dhcpNtp"].isNull()` -> `section["dhcpNtp"] = true;`
    - If `section["dhcpTz"].isNull()`:
      - Determine if v1 user explicitly configured a manual timezone (`timezone` is non-empty string, OR `posix` is non-empty string different from `"GMT0"`).
      - If manually configured in v1, set `section["dhcpTz"] = false;` (preserve explicit user preference across OTA upgrade). Otherwise set `section["dhcpTz"] = true;`.
    - Return `true`.
- `applyConfig(const JsonObject& section)`:
  - Read optional `"dhcpNtp"` (`if (section["dhcpNtp"].is<bool>()) _dhcpNtpEnabled = section["dhcpNtp"].as<bool>();`) and `"dhcpTz"` (`if (section["dhcpTz"].is<bool>()) _dhcpTzEnabled = section["dhcpTz"].as<bool>();`).
  - Keep `_configured = true` and `setMetaTimezone` behavior (or update meta timezone to active timezone when available).
- `serializeConfig(JsonObject& section) const`:
  - Serialize ONLY persisted configuration: `timezone`, `posix`, `ntpServer1`, `ntpServer2`, `ntpServer3`, `dhcpNtp`, `dhcpTz`.
  - **ZERO FLASH WEAR:** NEVER overwrite `_timezone` or `_posixTz` with transient DHCP-acquired values, and NEVER call `_configManager->saveModuleConfig(this)` when receiving DHCP options!

### 2.3 EventBus Wiring & Runtime DHCP Consumption (`TimeManager::setEventBus`, `tick()`, `WIFI_CONNECTED`)
- Add `void setEventBus(EventBus* eb);` to `TimeManager` (forward declare or `#include "EventBus.hpp"`).
  - Subscribes once (guarded against duplicate subscribe) to `WifiEvents::WIFI_CONNECTED` (`"wifi.connected"`).
  - The static `EventCallback` ONLY executes `_wifiConnectedPending.store(true);` (`std::atomic<bool> _wifiConnectedPending{false};`) because `EventBus` runs on a 2304-byte event-loop stack!
- Wire `timeManager.setEventBus(&app.getEventBus());` inside `#ifdef WITH_TIME` in `src/main.cpp`, `sample-app/src/main.cpp`, and `template-app/src/main.cpp` alongside `timeManager.setConfigManager(...)`.
- In `TimeManager::begin()` and `TimeManager::_reinitNtp()`:
  - Call `esp_sntp_servermode_dhcp(_dhcpNtpEnabled);` **before** `configTzTime(effectivePosix, _ntpServer1, _ntpServer2, _ntpServer3);` (wrapped `#if CONFIG_LWIP_DHCP_GET_NTP_SRV` on target, or enabled on `UNIT_TEST`).
- In `TimeManager::tick(uint32_t currentMillis)`:
  1. Check `_wifiConnectedPending.exchange(false)` and also check pending LwIP DHCP Option 100/101 staging via `_processPendingDhcpTimezone()` and `_refreshDhcpNtpStateAndFallback()` (note: run `_processPendingDhcpTimezone()` every tick — spinlock check is O(1) — and run `_refreshDhcpNtpStateAndFallback()` on `_wifiConnectedPending` OR periodically while `_dhcpNtpEnabled` / `_syncStatus == IN_PROGRESS`, e.g. at least whenever DHCP might have settled or SNTP slots changed, without heavy work).
  2. **`_processPendingDhcpTimezone()`:**
     - Call `lwip_dhcp_tz_take_pending(...)`. If either `gotPcode` or `gotTcode`:
       - If `!_dhcpTzEnabled`, discard and return.
       - Resolve runtime POSIX + IANA display name cleanly:
         - If `gotTcode`, store `tcode` safely as candidate IANA name, and if `!gotPcode`, resolve `tcode -> POSIX` via streaming scan of `/timezones.json` (`_lookupIanaPosixTz(const char* iana, char* outPosix, size_t outCap)` using a small fixed stack buffer without `malloc(fileSize)`!).
         - Precedence when BOTH arrive: Option 100 (`PCode`) supplies direct POSIX string reliably (works even without `/timezones.json`); if valid `TCode` also arrived, keep `tcode` as `_dhcpTimezone` display label (when `PCode` is set, `_dhcpTzSource = DhcpTzSource::PCode`, or verify if `TCode` resolves). If ONLY `TCode` arrived and lookup fails in `/timezones.json`, log a warning (`ESP_LOGW`) and do NOT clobber fallback timezone!
         - On successful resolution of runtime POSIX string: set `_dhcpPosixTz`, `_dhcpTimezone` (from `TCode` if present else empty), `_dhcpTzSource`, `_dhcpTzActive = true`, call `setTimezone(_dhcpPosixTz)`, and update `_configManager->setMetaTimezone(...)` if available (`_timezone` or `_dhcpTimezone`).
  3. **`_refreshDhcpNtpStateAndFallback()`:**
     - If `_dhcpNtpEnabled`:
       - Inspect SNTP server slots `0 .. MAX_SNTP_SLOTS - 1` (`3`):
         - A slot `i` holds a DHCP Option 42 server if `esp_sntp_getservername(i) == nullptr` AND `esp_sntp_getserver(i)` is non-null & non-zero/non-ANY (`!ip_addr_isany_val(...)`). Format via `ipaddr_ntoa_r` into `_dhcpNtpServers[]` and record `_dhcpNtpCount`. Set `_dhcpNtpActive = (_dhcpNtpCount > 0)`.
         - Because LwIP `dhcp_set_ntp_servers` clears slots `num .. MAX_SNTP_SLOTS - 1` to NULL, restore non-empty static fallback servers (`_ntpServer1`, `_ntpServer2`, `_ntpServer3`) into any trailing slot `idx` that has `esp_sntp_getservername(idx) == nullptr` and empty/ANY IP (`!isDhcpSlot(idx)`), using `esp_sntp_setservername(idx, fallback)` without overwriting active DHCP slots `0..num-1` or duplicating an already-present static server!
     - Else (`!_dhcpNtpEnabled`): ensure `_dhcpNtpActive = false; _dhcpNtpCount = 0;`.
- Note on `tick()` sync state check: Make sure `tick()` drains pending DHCP TZ and refreshes DHCP NTP fallback cleanly regardless of whether `_syncStatus` is `UNSYNCED`, `IN_PROGRESS`, or `SYNCED` (currently lines 74-83 have an early `return;` inside `if (_syncStatus == IN_PROGRESS)` — place DHCP processing BEFORE that early return!).

### 2.4 Effective Getters, Preconditions, REST `/api/time/config`, & MCP `"system_info"`
- Update `TimeManager` getters cleanly while preserving backward compatibility:
  - `isConfigured() const`: return `_configured || (_dhcpTzActive && _dhcpTzEnabled);`
  - `getTimezone() const`: return `(_dhcpTzActive && _dhcpTzEnabled && strlen(_dhcpTimezone) > 0) ? _dhcpTimezone : _timezone;` (or fallback string cleanly; wait: if DHCP provided only Option 100 `PCode` without `TCode`, `getTimezone()` may remain `_timezone` or `""` while `getPosixTz()` returns `_dhcpPosixTz` — check that `TimePreconditionContributor` still treats `isConfigured() == true`!).
  - `getPosixTz() const`: return `(_dhcpTzActive && _dhcpTzEnabled) ? _dhcpPosixTz : _posixTz;`
  - Add clean accessors: `isDhcpNtpEnabled()`, `isDhcpNtpActive()`, `getDhcpNtpServerCount()`, `getDhcpNtpServer(size_t idx)`, `isDhcpTzEnabled()`, `isDhcpTzActive()`, `getDhcpTzSource()`, `getDhcpTzSourceStr()` (`"none"`, `"pcode"`, `"tcode"`), `getConfiguredTimezone()`, `getConfiguredPosixTz()`.
- Update `GET /api/time/config` (`_handleGetConfig`):
  - Keep existing keys (`timezone` -> persisted fallback, `posix` -> persisted fallback, `ntpServer1..3`, `configured`, `syncStatus`, `localTime`). Wait: keep `timezone`/`posix` consistent or clarify `timezone`/`posix` + `activeTimezone`/`activePosix` so UI can show both fallback inputs and active runtime status clearly! Specifically:
    - `timezone`: `_timezone` (configured fallback IANA)
    - `posix`: `_posixTz` (configured fallback POSIX)
    - `activeTimezone`: `getTimezone()` (or `_dhcpTzActive && _dhcpTzEnabled ? (strlen(_dhcpTimezone) > 0 ? _dhcpTimezone : _dhcpPosixTz) : _timezone`)
    - `activePosix`: `getPosixTz()`
    - `dhcpNtp`: `_dhcpNtpEnabled`
    - `dhcpTz`: `_dhcpTzEnabled`
    - `dhcpNtpActive`: `_dhcpNtpActive`
    - `dhcpNtpServers`: JSON array of `_dhcpNtpServers[0.._dhcpNtpCount-1]`
    - `dhcpTzActive`: `_dhcpTzActive`
    - `dhcpTzSource`: `getDhcpTzSourceStr()` (`"none"`, `"pcode"`, `"tcode"`)
- Update `POST /api/time/config` (`_handlePostConfig`):
  - Handle optional `dhcpNtp` (`bool`) and `dhcpTz` (`bool`).
  - If `dhcpTz` changed to `false`, clear active DHCP TZ runtime state (`_dhcpTzActive = false`, `_dhcpTzSource = None`, clear `_dhcpTimezone`/`_dhcpPosixTz`) and immediately `setTimezone(_posixTz)`.
  - If `dhcpNtp` changed to `false`, clear `_dhcpNtpActive = false`, `_dhcpNtpCount = 0`.
  - Apply any manual `timezone`, `posix`, `ntpServer1..3` passed, persist ONLY persisted config via `_configManager->saveModuleConfig(this)` if non-null, call `esp_sntp_servermode_dhcp(_dhcpNtpEnabled)` + `_reinitNtp()` using `getPosixTz()`.
- Update `modules/MCPServer/MCPBuiltinTools.cpp` (`"system_info"` tool inside `if (c->timeManager != nullptr)`):
  - Keep all existing fields (`time_configured`, `time_synced`, `time_timezone`, `time_posix`, `time_sync_status`, `time_local`).
  - Add:
    - `result["time_dhcp_ntp_enabled"] = c->timeManager->isDhcpNtpEnabled();`
    - `result["time_dhcp_ntp_active"]  = c->timeManager->isDhcpNtpActive();`
    - `JsonArray dhcpNtpArr = result["time_dhcp_ntp_servers"].to<JsonArray>();` (populated with discovered IPs)
    - `result["time_dhcp_tz_enabled"]  = c->timeManager->isDhcpTzEnabled();`
    - `result["time_dhcp_tz_active"]   = c->timeManager->isDhcpTzActive();`
    - `result["time_dhcp_tz_source"]   = c->timeManager->getDhcpTzSourceStr();`

### 2.5 Native Test Stubs & GoogleTest Suite
- Upgrade `test/stubs/platform/freertos/FreeRTOS.h` with `portMUX_TYPE`, `portMUX_INITIALIZER_UNLOCKED`, `portENTER_CRITICAL`, `portEXIT_CRITICAL`.
- Upgrade `test/stubs/platform/esp_sntp.h` to be stateful and test-controllable (reset helper for test fixtures, `ip_addr_t` / `ip4_addr_t` / `ipaddr_ntoa_r` / `esp_sntp_setserver` / `esp_sntp_setservername` / `esp_sntp_getserver` / `esp_sntp_getservername` / `esp_sntp_servermode_dhcp` / `esp_sntp_stop` / `esp_sntp_enabled`, plus minimal `struct pbuf` / `pbuf_copy_partial` / `DHCP_STATE_*` / test helper to simulate LwIP `dhcp_set_ntp_servers`!).
- Preserve all existing tests in `test/test_unit/test_timeManager/test_timeManager.cpp` AND add comprehensive GoogleTests covering:
  - Schema v2 (`getSchemaVersion() == 2`), v1->v2 migration with empty timezone vs manually configured v1 timezone, `resetConfig()`, zero flash wear (`serializeConfig` excludes runtime DHCP overrides).
  - Option 42 (`esp_sntp_servermode_dhcp(true/false)` tracking, simulated DHCP Option 42 setting 1 or 2 IP servers while preserving static fallback servers in trailing slots, disabling DHCP NTP restores static servers).
  - RFC 4833 Option 100 (`PCode`) and Option 101 (`TCode` resolved against a test `/timezones.json` under `test/tmp/timezones.json`, cleaning up after test!), `DHCP_STATE_SELECTING` rejection, non-printable/oversized rejection, Option 100 + Option 101 precedence, `isConfigured()` returning `true` when DHCP TZ active, `dhcpTz=false` clearing DHCP override back to manual fallback.
  - `EventBus` `WifiEvents::WIFI_CONNECTED` triggering deferred handling inside `tick()`.
  - `MCPBuiltinTools` `"system_info"` reporting the new `time_dhcp_*` telemetry fields in `test/test_unit/test_mcpServer/`.

## 3. Verification Requirements

1. Use PlatformIO MCP tools (`acquire_lock` -> execute -> `release_lock`, NEVER shell `pio`!):
   - `run_tests` (`environment: "native"`, root project) — confirm ALL native suites pass.
   - `build_project` in root project on: `esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`.
   - `build_project` in `sample-app` on: `esp32dev_all_on`, `esp32dev_all_off`.
   - Note: ALSO verify with `nm` or ELF check on built `esp32dev` firmware if helpful that `lwip_dhcp_on_extra_option` resolved to our strong symbol (non-weak) from `TimeManager`!
2. Run `python3 scripts/doc_lint.py` and confirm exit `0`.
3. Update `completion` & `## Completion Report` in this `WORKORDER.md` BEFORE running `pa wo verify WO-320.2`.

## Completion Report

### Execution Summary
Implemented DHCP Option 42 (NTP server discovery) and RFC 4833 Options 100 (`PCode`) & 101 (`TCode`) timezone discovery across `TimeManager`, `MCPBuiltinTools`, all three app entry points, native host stubs, and unit tests:

1. **LwIP Hook & Spinlock Handoff (`modules/TimeManager/TimeManager.cpp`):**
   - Implemented strong `extern "C"` `lwip_dhcp_on_extra_option(...)` and `lwip_dhcp_tz_take_pending(...)` guarded by `portMUX_TYPE s_dhcpTzMux`.
   - Accepts only `DHCP_STATE_REQUESTING`, `DHCP_STATE_RENEWING`, `DHCP_STATE_REBINDING`, `DHCP_STATE_REBOOTING`, and `DHCP_STATE_BOUND` (ignoring `DHCP_STATE_SELECTING`), accepts only Options 100 and 101 (no Option 2), rejects `len == 0` / `len >= 64` / `nullptr` / non-printable ASCII / embedded NULs / outer whitespace, and performs zero heap, VFS, EventBus, `setenv`/`tzset`, or heavy logging inside the hook.
2. **Schema v2 & Zero Flash Wear (`modules/TimeManager/TimeManager.hpp`, `modules/TimeManager/TimeManager.cpp`):**
   - Upgraded `getSchemaVersion()` to `2` with `migrateConfig()` migrating v1->v2 (`dhcpNtp=true`, `dhcpTz=false` when manual v1 timezone or non-`"GMT0"` POSIX rule is configured, else `true`).
   - Kept volatile `_dhcpNtp*` and `_dhcpTz*` state strictly out of `serializeConfig()` and never overwrote persisted `_timezone`/`_posixTz` or called `saveModuleConfig()` on DHCP updates.
3. **Runtime Consumption & EventBus Wiring (`TimeManager.cpp`, `src/main.cpp`, `sample-app/src/main.cpp`, `template-app/src/main.cpp`):**
   - Added `TimeManager::setEventBus(EventBus*)` subscribing idempotently to `"wifi.connected"` and setting only `std::atomic<bool> _wifiConnectedPending` in the EventBus callback.
   - Wired `timeManager.setEventBus(&app.getEventBus())` inside `#ifdef WITH_TIME` across all three `main.cpp` files.
   - Updated `begin()`, `_reinitNtp()`, and `tick()` (before `IN_PROGRESS` early return) to stop/clear SNTP slots, invoke `esp_sntp_servermode_dhcp(_dhcpNtpEnabled)` before `configTzTime`, prioritize Option 100 `PCode` while preserving `TCode` display label, resolve Option 101 `TCode` via fixed-stack streaming scan of `/timezones.json`, and preserve non-duplicate static fallback servers in trailing SNTP slots.
4. **REST API & MCP Telemetry (`TimeManager.cpp`, `modules/MCPServer/MCPBuiltinTools.cpp`):**
   - Extended `GET`/`POST /api/time/config` with `activeTimezone`, `activePosix`, `dhcpNtp`, `dhcpTz`, `dhcpNtpActive`, `dhcpNtpServers`, `dhcpTzActive`, and `dhcpTzSource`.
   - Extended `"system_info"` in `MCPBuiltinTools.cpp` with `time_dhcp_ntp_enabled`, `time_dhcp_ntp_active`, `time_dhcp_ntp_servers`, `time_dhcp_tz_enabled`, `time_dhcp_tz_active`, and `time_dhcp_tz_source` without adding extra tools.
5. **Native Stubs & GoogleTests (`test/stubs/platform/*`, `test/test_unit/test_timeManager/`, `test/test_unit/test_mcpServer/`):**
   - Upgraded `FreeRTOS.h` (`portMUX_TYPE`, critical section macros) and stateful `esp_sntp.h` (`stub_sntp_*`, `ip_addr_t`, `pbuf`, `dhcp`, SNTP slot APIs).
   - Preserved all existing tests and added tests covering hook filtering, malformed payloads, schema v2 migration, zero flash wear, Option 42 trailing fallbacks, Option 100/101 precedence, streaming `/timezones.json`, preconditions, `EventBus` handoff, REST config, and MCP `system_info`.

### Verification Results
- **Doc Lint:** `python3 scripts/doc_lint.py` passed (exit 0).
- **Native Unit Tests (`native`):** PlatformIO MCP `run_tests` passed all 1019/1019 test cases across all 36 suites (`failed: 0, errored: 0, skipped: 0`) in 53.21s, including 22/22 in `test_unit/test_timeManager` and 76/76 in `test_unit/test_mcpServer`.
- **Root Firmware Builds (PlatformIO MCP `build_project`):**
  - `esp32dev`: SUCCEEDED (RAM 58,460 B, Flash 1,345,243 B)
  - `esp32s3nano`: SUCCEEDED (RAM 61,084 B, Flash 1,379,551 B)
  - `crowpanel-128-rotary`: SUCCEEDED (RAM 64,828 B, Flash 1,912,095 B)
- **Sample App Firmware Builds (PlatformIO MCP `build_project`):**
  - `esp32dev_all_on`: SUCCEEDED (RAM 65,460 B, Flash 1,559,559 B)
  - `esp32dev_all_off`: SUCCEEDED (RAM 58,516 B, Flash 1,488,147 B)
- **ELF Strong Symbol Verification (`xtensa-esp32-elf-nm`):**
  - `.pio/build/esp32dev/firmware.elf`: `400e71fc T lwip_dhcp_on_extra_option`, `400e72bc T lwip_dhcp_tz_take_pending`
  - `sample-app/.pio/build/esp32dev_all_on/firmware.elf`: `400e9410 T lwip_dhcp_on_extra_option`, `400e94d0 T lwip_dhcp_tz_take_pending`

### Escalation Notice
`pa wo verify WO-320.2` failed scope hygiene with:
`Error: Scope hygiene check failed for WO-320.2; modified or untracked files outside allowed_files: agents/plugins/principal-architect-workspace/pr-320-dhcp-ntp-timezone/wo2-timemanager-dhcp/WORKORDER.md`
because `pa_cli.py:_parse_porcelain_paths()` calls `.lstrip("./")`, stripping the leading `.` from `.agents/.../WORKORDER.md` (same known harness defect encountered by `WO-320.1`). All 10 modified code/test files are strictly inside `allowed_files`, `spec_hash` is intact, and every verification requirement has passed. Escalating to the Principal Architect (`pa`) for cryptographic receipt stamping and commit creation.
