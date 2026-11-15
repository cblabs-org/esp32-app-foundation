---
schema_version: 1
programme_id: prog-dhcp-ntp-timezone
title: DHCP NTP (Option 42) & DHCP Timezone (RFC 4833 Options 100/101)
base_branch: main
status: ACTIVE
prs:
- id: PR-320
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
  status: GRADUATED
  github_pr: null
---

# DHCP NTP (Option 42) & DHCP Timezone (RFC 4833 Options 100/101)

## 1. Architectural Vision & System Invariants

Single-PR programme (`PR-320`) delivering standards-compliant DHCP network time and timezone auto-configuration for ESP32 App Foundation without introducing flash wear or blocking the LwIP TCP/IP task:

1. **DHCP NTP Server Discovery (RFC 2132 Option 42):** Enable LwIP DHCP Option 42 (`CONFIG_LWIP_DHCP_GET_NTP_SRV=y`, `CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2`, `CONFIG_LWIP_SNTP_MAX_SERVERS=3`, `esp_sntp_servermode_dhcp`), while preserving configured static NTP servers (`_ntpServer1..3`) in trailing SNTP slots and as fallback when DHCP provides fewer or zero NTP servers.
2. **DHCP Timezone Discovery (RFC 4833 Options 100 & 101):** Use LwIP DHCP extra-option hooks (`CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y`, `include/lwip_dhcp_tz_hooks.h`, `ESP_IDF_LWIP_HOOK_FILENAME`, `DHCP_ADD_EXTRA_REQUEST_OPTIONS`) to receive Option 100 (`PCode`, POSIX TZ string) and Option 101 (`TCode`, IANA timezone name resolved via the bundled `/timezones.json` lookup).
3. **Explicit Exclusion:** Deprecated RFC 2132 Option 2 (`Time Offset`) is **out of scope** — it lacks DST transition rules and is superseded by RFC 4833.

### Programme-Wide System Invariants

- **LwIP TCP/IP Context Safety:** `lwip_dhcp_on_extra_option` executes inside the LwIP TCP/IP task context. Zero heap allocations, VFS/LittleFS I/O, `ConfigManager` calls, `setenv()`, or `tzset()` are permitted inside the hook; it may only copy bounded raw bytes into a static staging buffer protected by a `portMUX_TYPE` spinlock (host-stubbed for `native`), consumed on the main loop by `TimeManager::tick()`.
- **Zero Flash Wear:** DHCP-acquired NTP servers and DHCP-acquired timezone are volatile runtime state held only in RAM. `/config.json` (and `/mod/time.json`) persists only explicit user configuration (`dhcp_ntp`, `dhcp_tz`, fallback `timezone`, `posix`, `ntpServer1..3`).
- **SNTP Slot Preservation:** Because LwIP `dhcp_set_ntp_servers()` clears SNTP slots `num..SNTP_MAX_SERVERS-1` when applying Option 42, `TimeManager` must preserve and re-populate configured fallback static NTP servers into trailing unused slots or when DHCP returns zero servers.
- **Three-Target Parity:** Root (`CMakeLists.txt`, `sdkconfig.defaults`, `src/main.cpp`), `sample-app/`, and `template-app/` must receive consistent LwIP hook injection, `sdkconfig.defaults` flags, and `timeManager.setEventBus(&app.getEventBus())` wiring.
- **MCP Tooling & Native Test Parity:** Host-native unit tests execute cleanly via stubbed `esp_sntp.h` and FreeRTOS spinlock headers; all PlatformIO builds and native tests run via PlatformIO MCP tools (`acquire_lock` -> execute -> `release_lock`), never shell `pio`.

## 2. Cross-PR Contracts & Interfaces

Single-PR programme targeting `feat/dhcp-ntp-timezone` (based on `main` at `6a1c3a11`):

| PR | Project Charter | Branch | Base | Scope & Interface Contracts |
|---|---|---|---|---|
| `PR-320` | [`PROJECT.md`](../pr-320-dhcp-ntp-timezone/PROJECT.md) | `feat/dhcp-ntp-timezone` | `main` | LwIP Option 42 & RFC 4833 Option 100/101 hooks (`include/lwip_dhcp_tz_hooks.h`), `TimeManager` (`setEventBus`, `WifiEvents::WIFI_CONNECTED`, `IConfigurable` v2 schema + `migrateConfig(section, 1)`, `/api/time/config`, `web/time.js`), `MCPBuiltinTools` (`system_info`), root/`sample-app`/`template-app` parity, stubs/tests/docs, `.memory/.gitignore`. |

## 3. Programme Session Log

- **2026-11-14 (Tier 1 Chief Architect):** Initialized `prog-dhcp-ntp-timezone` (`status: ACTIVE`), authored and locked `PR-320` Part I Charter ([`PROJECT.md`](../pr-320-dhcp-ntp-timezone/PROJECT.md)) on `feat/dhcp-ntp-timezone`, committed `PROGRAM.md` and `PROJECT.md`, and prepared `PR-320` via `pa pr start PR-320`. Tier 2 Principal Architect completed Work Order decomposition (`WO-320.1`..`WO-320.3`) and plan is approved (`status: APPROVED`). Handoff to Tier 2 PA for execution dispatch on `WO-320.1`.
