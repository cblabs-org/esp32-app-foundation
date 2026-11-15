---
schema_version: 1
charter:
  pr_id: PR-320
  programme_id: prog-dhcp-ntp-timezone
  slug: pr-320-dhcp-ntp-timezone
  title: 'feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts
    100/101)'
  branch: feat/dhcp-ntp-timezone
  base_branch: main
  depends_on: []
  allowed_modules:
  - .gitignore
  - .memory/.gitignore
  - CMakeLists.txt
  - platformio.ini
  - sdkconfig.defaults
  - include/lwip_dhcp_tz_hooks.h
  - src/main.cpp
  - sample-app/CMakeLists.txt
  - sample-app/sdkconfig.defaults
  - sample-app/src/main.cpp
  - template-app/CMakeLists.txt
  - template-app/sdkconfig.defaults
  - template-app/src/main.cpp
  - modules/TimeManager/**
  - modules/MCPServer/**
  - test/stubs/**
  - test/test_unit/test_timeManager/**
  - test/test_unit/test_mcpServer/**
  - test/web/**
  - docs/developer/**
  - .agents/skills/framework-expert/**
  required_skills:
  - embedded-systems
  - framework-expert
  - pio-manager
  verification_floor:
  - python3 scripts/doc_lint.py
  - cd test/web && npx playwright test
  charter_locked: true
  charter_hash: 5733b8ef2ccf4779ecd126f949d67ccdc427e192e9b164aadb4a50db1440e434
status: APPROVED
approved_at: '2026-11-14T18:30:07.889014Z'
github_pr: null
work_orders:
- id: WO-320.1
  slug: wo1-lwip-dhcp-hooks
  title: Configure LwIP DHCP Option 42/100/101 hooks, sdkconfigs, and repo hygiene
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: b0f8ce56d6799c1f6d673a877c91c460575c23fc
- id: WO-320.2
  slug: wo2-timemanager-dhcp
  title: Implement DHCP NTP & Timezone in TimeManager, MCP telemetry, main wiring,
    and native tests
  complexity: architect
  status: COMPLETED
  attempts: 1
  commit_sha: 41bb5fb7c147decdd82b4be69bc3409e2cd49cba
- id: WO-320.3
  slug: wo3-portal-docs-tests
  title: Add Time portal DHCP UI, Playwright E2E tests, and update developer docs/skills
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 70dfa3f92d0aa4d4203971f1eb1200abf98b7251
- id: WO-320.4
  slug: wo4-s3nano-flash-fix
  title: Restore board_upload.arduino.flash_extra_images suppression for esp32s3nano
    in platformio.ini
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 504bb1ee44b1cc9a4f5bdbf809f9cd231ab3c033
---

# PR-320 — feat(time): DHCP NTP (RFC 2132 opt 42) and DHCP Timezone (RFC 4833 opts 100/101)

## 1. PR Charter: Scope, Goals, & Non-Goals

### Goals

1. **DHCP NTP Server Discovery (RFC 2132 Option 42):**
   - Enable `CONFIG_LWIP_DHCP_GET_NTP_SRV=y`, `CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2`, and `CONFIG_LWIP_SNTP_MAX_SERVERS=3` across root, `sample-app/`, and `template-app/` `sdkconfig.defaults`.
   - Integrate `esp_sntp_servermode_dhcp` in `TimeManager` while preserving configured static NTP servers (`_ntpServer1..3`) in trailing SNTP slots and as fallback when DHCP provides fewer than 3 or 0 servers.
2. **DHCP Timezone Discovery (RFC 4833 Options 100 & 101):**
   - Add LwIP DHCP extra-option hook infrastructure (`CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y`, `include/lwip_dhcp_tz_hooks.h`, `ESP_IDF_LWIP_HOOK_FILENAME`, `DHCP_ADD_EXTRA_REQUEST_OPTIONS`) across root, `sample-app/`, and `template-app/`.
   - Support Option 100 (`PCode`, direct POSIX TZ rule string) and Option 101 (`TCode`, IANA timezone name resolved to a POSIX TZ string using the bundled `/timezones.json` lookup).
3. **Thread-Safe Hook Handoff & EventBus Integration:**
   - Stage Option 100/101 payloads inside a spinlock-guarded (`portMUX_TYPE`) static staging buffer in `include/lwip_dhcp_tz_hooks.h` and consume/apply them safely on the main loop inside `TimeManager::tick()`.
   - Add `TimeManager::setEventBus()` and subscribe to `WifiEvents::WIFI_CONNECTED` (`"wifi.connected"`), wiring `timeManager.setEventBus(&app.getEventBus())` consistently in `src/main.cpp`, `sample-app/src/main.cpp`, and `template-app/src/main.cpp`.
4. **Config Schema v2, REST API, MCP Telemetry, Portal UI, Tests & Docs:**
   - Upgrade `TimeManager` `IConfigurable` schema to v2 (`getSchemaVersion() == 2`, `migrateConfig(JsonObject& section, int fromVersion)` migrating schema v1), exposing explicit opt-in / precedence flags (`dhcp_ntp`, `dhcp_tz` and/or fallback behavior).
   - Expose DHCP NTP/timezone settings and live runtime discovery state in `GET`/`POST /api/time/config`, `MCPBuiltinTools` (`"system_info"` telemetry in `modules/MCPServer/MCPBuiltinTools.cpp`), and the portal UI (`modules/TimeManager/web/time.js`) with matching `test/web/` mocks and Playwright tests.
   - Add host-native stubs (`test/stubs/platform/esp_sntp.h`, FreeRTOS spinlock stubs) and native unit tests (`test/test_unit/test_timeManager/`, `test/test_unit/test_mcpServer/`).
   - Update developer documentation (`docs/developer/`) and `.agents/skills/framework-expert/`.
5. **Repository Hygiene:**
   - Update `.memory/.gitignore` (`.cache/`, `.enrolled`, `local/`) so untracked runtime artifacts do not pollute `git status`.

### Non-Goals

- Do **NOT** implement RFC 2132 Option 2 (`Time Offset`, deprecated by RFC 4833, lacks DST transitions).
- Do **NOT** modify `docs/design_specification.md`.
- Do **NOT** touch hardware GPIO, display, or OTA modules/configs unrelated to `TimeManager` and `MCPBuiltinTools`.
- Do **NOT** persist DHCP-acquired runtime NTP servers or DHCP-acquired timezone values to `/config.json` or `/mod/time.json` (zero flash wear invariant).

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

### System Invariants

1. **LwIP TCP/IP Context Safety:** The LwIP DHCP hook (`lwip_dhcp_on_extra_option`) executes inside LwIP TCP/IP context: **ZERO** heap allocations (`malloc`/`new`/`String`), VFS/LittleFS I/O, `ConfigManager` calls, `setenv()`, or `tzset()` may run inside the hook. It may only copy bounded raw bytes under a `portMUX_TYPE` spinlock (host-stubbed on `native`), to be consumed by `TimeManager::tick()` on the main loop.
2. **Zero Flash Wear:** DHCP-acquired NTP servers and DHCP-acquired timezone are volatile runtime state; `/config.json` (and `/mod/time.json`) only persists explicit user configuration (`dhcp_ntp`, `dhcp_tz`, fallback `timezone`, `posix`, `ntpServer1..3`).
3. **SNTP Fallback Slot Preservation:** LwIP `dhcp_set_ntp_servers()` clears SNTP slots `num..SNTP_MAX_SERVERS-1` when applying Option 42, so `TimeManager` must preserve and re-populate fallback static NTP servers in remaining unused slots or when DHCP returns zero servers.
4. **Three-Target Parity:** Root (`CMakeLists.txt`, `sdkconfig.defaults`, `src/main.cpp`), `sample-app/`, and `template-app/` must all receive consistent CMake hook injection (`ESP_IDF_LWIP_HOOK_FILENAME`), `sdkconfig.defaults` flags, and `timeManager.setEventBus(&app.getEventBus())` wiring in `main.cpp`.
5. **Native Stub & PlatformIO MCP Discipline:** The `native` unit test suite must build and pass cleanly via stubbed `esp_sntp.h` / FreeRTOS spinlock headers; all PlatformIO builds and tests MUST execute via PlatformIO MCP tools (`acquire_lock` -> execute -> `release_lock`), never shell `pio`.

### Cross-Module Interfaces

- **`include/lwip_dhcp_tz_hooks.h` <-> `modules/TimeManager/`:** LwIP C hook (`#ifdef ESP_PLATFORM` / ESP-IDF LwIP hook header) staging Option 100/101 payloads behind `portMUX_TYPE`, read and cleared atomically from `TimeManager::tick()`.
- **`modules/TimeManager/TimeManager.hpp` <-> `EventBus` / `ConfigManager` / `TimePreconditionContributor`:** `setEventBus()` subscribes to `WifiEvents::WIFI_CONNECTED` (`"wifi.connected"`); `IConfigurable` v2 schema + `migrateConfig(section, 1)`; `TimePreconditionContributor` reflects effective timezone/sync readiness without breaking existing contracts.
- **`modules/MCPServer/MCPBuiltinTools.cpp` (`"system_info"`):** Reads `TimeManager` telemetry getters to report DHCP NTP / DHCP timezone active state alongside `time_configured`, `time_synced`, `time_timezone`, `time_posix`, `time_sync_status`, and `time_local`.
- **`modules/TimeManager/web/time.js` <-> `/api/time/config` <-> `test/web/`:** Portal UI toggles and status display backed by `/api/time/config`, covered by `test/web/server.js` mock routes and Playwright specs.

### Required Skills & Tools

- **Required Skills:** `embedded-systems`, `framework-expert`, `pio-manager`
- **Verification Floor (CLI):**
  - `python3 scripts/doc_lint.py`
  - `cd test/web && npx playwright test`
- **Verification Floor (PlatformIO MCP — `acquire_lock` -> execute -> `release_lock`, never shell `pio`):**
  - `run_tests` (`environment: "native"`, root project)
  - `build_project` (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary` in root project)
  - `build_project` (`esp32dev_all_on`, `esp32dev_all_off` in `sample-app`)

## 3. Implementation Plan & Work Order Breakdown

`PR-320` is decomposed into four sequential Work Orders with non-overlapping concerns and full coverage of all three build targets (`esp32-app-foundation`, `sample-app`, `template-app`):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-320.1` | `wo1-lwip-dhcp-hooks` | Configure LwIP DHCP Option 42/100/101 hooks, sdkconfigs, and repo hygiene | `standard` | Charter locked |
| `WO-320.2` | `wo2-timemanager-dhcp` | Implement DHCP NTP & Timezone in TimeManager, MCP telemetry, main wiring, and native tests | `architect` | `WO-320.1` |
| `WO-320.3` | `wo3-portal-docs-tests` | Add Time portal DHCP UI, Playwright E2E tests, and update developer docs/skills | `standard` | `WO-320.2` |
| `WO-320.4` | `wo4-s3nano-flash-fix` | Restore board_upload.arduino.flash_extra_images suppression for esp32s3nano in platformio.ini | `standard` | `WO-320.3` |

### Sequencing Notes
1. `WO-320.1` lays down `.memory/.gitignore`, Kconfig SNTP/LwIP DHCP flags across all 3 projects, `include/lwip_dhcp_tz_hooks.h`, and CMake `ESP_IDF_LWIP_HOOK_FILENAME` injection so LwIP builds cleanly before `TimeManager` implements the strong hook symbol.
2. `WO-320.2` implements `TimeManager` schema v2 (`migrateConfig`), `setEventBus(WIFI_CONNECTED)`, Option 42 SNTP + trailing slot fallback preservation, RFC 4833 Option 100/101 spinlock staging & streaming `/timezones.json` lookup, REST API fields, `MCPBuiltinTools` `"system_info"` telemetry, 3-target `main.cpp` wiring, and stateful host stubs/GoogleTests.
3. `WO-320.3` adds the portal UI controls (`modules/TimeManager/web/time.js`), mock routes (`test/web/server.js`), Playwright E2E suite (`test/web/tests/time.spec.ts`), and developer/skill documentation (`docs/developer/**`, `.agents/skills/framework-expert/**`).
4. `WO-320.4` restores `board_upload.arduino.flash_extra_images =` suppression under `[env:esp32s3nano]` in `platformio.ini` (inherited by `[env:esp32s3nano_ble]`) so hybrid `arduino, espidf` builds and flashes for `esp32s3nano` do not fail trying to merge `nora_recovery.ino.bin` with a broken Tasmota-prefixed path.

## 4. Outcome & Session Log
