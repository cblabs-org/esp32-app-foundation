---
name: pr-20-config-refactor-wo2a
description: Handoff for WO2a — move the three already-persisted runtime defaults (hostname, timezone, mcpReadOnly) out of FrameworkConfig into their owning modules' resetConfig(), with cross-module re-sourcing. Refactor.
complexity: standard
---

# pr-20-config-refactor / WO2a — Slim FrameworkConfig: Already-Persisted Moves

**Branch:** `pr-20-config-refactor/wo2a-persisted-moves` (cut by the PA, off `pr-20-config-refactor/wo1-appconfig-clarity`)

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are **NOT** the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Execute the checklist below exactly — no more, no less.

## Objective

Remove `hostname`, `timezone`, and `mcpReadOnly` from `FrameworkConfig`, relocating each as a **factory default in its owning module's `resetConfig()`**, and re-source the modules that read `hostname` from `FrameworkConfig` so they read it from `WiFiManager` instead. These three fields are **already persisted** module config, so the portal contract is unchanged.

## Strict Boundary Box

- **DO NOT** touch `firmwareUpdatesEnabled` or `otaQuietHour` — those are **WO2b**. Leave them in `FrameworkConfig` for now.
- **DO NOT** remove the three OTA secrets (`otaPublicKey`, `otaGithubRepo`, `otaGithubToken`) — they stay in `FrameworkConfig`.
- **DO NOT** perform the OO-review's `OTAUpdateManager` `app_`-removal refactor — out of scope.
- **DO NOT** change on-device runtime behaviour beyond the field relocation. No new portal code (these fields are already exposed: `timezone` via time.js, `mcpReadOnly` via mcp.js; `hostname` has no portal surface).
- **DO NOT** branch, merge, or rebase. The PA owns branch state.

## Design Decisions (fixed — implement as stated)

1. **Hostname → WiFiManager (app names the default).** Add `WiFiManager::setDefaultHostname(const char*)` storing a `_defaultHostname` (framework fallback `"esp32-device"`). `resetConfig()` sets `currentHostname = _defaultHostname`. `begin()` drops its `const char* hostname` parameter and relies on the persisted value (`applyConfig`) or the reset default. `main.cpp` sets the app default via `app.getWifiManager().setDefaultHostname("esp32-foundation")` **before** `app.begin(config)`.
2. **Timezone → TimeManager.** `resetConfig()` sets the default POSIX TZ (`"GMT0"`). `begin()` drops its `const char* timezone` parameter. (No app override needed — `main.cpp` never set timezone.)
3. **mcpReadOnly → MCPServer.** `resetConfig()` sets `_mcpReadOnly = true` (**secure-by-default — must be `true`**). Remove the `_mcpReadOnly = frameworkConfig->mcpReadOnly` seeding.
4. **MCPServer hostname re-source.** MCPServer's status endpoint sources hostname from **WiFiManager** (single source of truth), not `FrameworkConfig`.

## Line-Item Instructions

### WiFiManager (`modules/WifiManager/`)
- [ ] Add `void setDefaultHostname(const char*)` + a `char _defaultHostname[33]` member (init `"esp32-device"`).
- [ ] Add a public `const char* getHostname() const` returning `currentHostname`.
- [ ] `resetConfig()`: set `currentHostname` ← `_defaultHostname`.
- [ ] Change `begin(const char* hostname)` → `begin()` (no hostname param); remove the hostname-seeding logic (persisted value via `applyConfig`, else reset default).

### TimeManager (`modules/TimeManager/`)
- [ ] `resetConfig()`: set default POSIX TZ `"GMT0"` into the timezone member.
- [ ] Change `begin(const char* timezone)` → `begin()`; remove the timezone-seeding logic.

### MCPServer (`modules/MCPServer/`)
- [ ] `resetConfig()`: set `_mcpReadOnly = true`.
- [ ] Remove the `FrameworkConfig*` parameter from `begin(...)` and the `_frameworkConfig` member; **add a `WifiManager*` parameter** (mirroring the existing `TimeManager*`), stored for the status endpoint.
- [ ] `MCPServer.cpp:~436`: replace `_frameworkConfig->hostname` with `_wifiManager->getHostname()` (guard for null).
- [ ] Remove the `mcpReadOnly` seeding line (`~:53`).

### DiagnosticsBoard
- [ ] `configure(...)`: drop the `hostname` parameter; self-source via the already-injected `WifiManager*` (`getHostname()`).

### Application (`modules/Application/Application.cpp`)
- [ ] `:161` `_wifiManager.begin(_config.hostname)` → `_wifiManager.begin()`.
- [ ] `:166` `_timeManager.begin(_config.timezone)` → `_timeManager.begin()`.
- [ ] `:201` `_mcpServer.begin(_mcpContributors, &_configManager, &_config, &_timeManager)` → drop `&_config`, add `&_wifiManager` (final arg list: contributors, `&_configManager`, `&_timeManager`, `&_wifiManager`).
- [ ] `:210` `_diagnosticsBoard.configure(_config.hostname, &_wifiManager)` → `_diagnosticsBoard.configure(&_wifiManager)`.
- [ ] `:119-120` remove/adjust the log line referencing `_config.hostname` / `_config.timezone`.

### FrameworkConfig (`include/FrameworkConfig.hpp`)
- [ ] Remove the `hostname`, `timezone`, and `mcpReadOnly` fields and their doc-header lines. **Keep** `firmwareUpdatesEnabled`, `otaQuietHour` (WO2b), `otaPublicKey`, `otaGithubRepo`, `otaGithubToken`.

### main.cpp (`src/main.cpp`)
- [ ] Remove `config.hostname = "esp32-foundation"` (`:47`).
- [ ] Add `app.getWifiManager().setDefaultHostname("esp32-foundation");` **before** `app.begin(config)`.

### Tests (`test/test_unit/`)
- [ ] Update for the changed signatures/behaviour: `test_wifiManager` (`begin()`, `setDefaultHostname`, `getHostname`, `resetConfig` hostname default), `test_timeManager` (`begin()`, resetConfig TZ), `test_mcpServer` (`begin()` new param list, `resetConfig` `readOnly==true`), `test_application` (wiring), `test_u8g2Display/test_config`, `test_configManager` as needed.

## Verification Requirement

- [ ] **`pio test -e native` → all suites PASSED (target 519/519), 0 ERRORED.** (Baseline is currently green at this exact count — do not regress it.)
- [ ] **ESP32 build compiles:** `pio run -e esp32dev` (or the project standard board env) builds clean — do not fix native by breaking the device build.
- [ ] Route all build/test through the `pio-manager` skill.

**CRITICAL:** Before distilling or committing, **STOP and ask the user if they are happy**. Only *after* explicit approval invoke `@distillery` (target dir = this WO directory), append a status line to this `WORKORDER.md`, and terminate.

> *(Spawn note for the PA: include the claude-code adapter's artifact-write authorization block in the spawn prompt so the worker can update this WORKORDER's status line / write any status reliably.)*
