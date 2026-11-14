---
schema_version: 1
wo_id: WO-320.3
pr_id: PR-320
title: Add Time portal DHCP UI, Playwright E2E tests, and update developer docs/skills
complexity: standard
status: PENDING
allowed_files:
- modules/TimeManager/web/time.js
- test/web/server.js
- test/web/tests/time.spec.ts
- docs/developer/**
- .agents/skills/framework-expert/**
forbidden_files:
- docs/design_specification.md
required_skills:
- framework-expert
- modern-web-guidance
verification_command: python3 scripts/doc_lint.py && cd test/web && npx playwright
  test
max_attempts: 5
max_structural_failures: 2
spec_hash: 0b44fb68b214919f7703b86bc9c111bb632a56c890a6aa9e3075d1f0677b5f0f
completion: null
---

# WO-320.3 — Add Time portal DHCP UI, Playwright E2E tests, and update developer docs/skills

## 1. Objective & Scope

Update the device web portal's Time tab (`modules/TimeManager/web/time.js`) to expose DHCP NTP (Option 42) and DHCP Timezone (RFC 4833 Options 100/101) controls and live status telemetry, add mock routes and Playwright E2E coverage (`test/web/`), and update developer docs (`docs/developer/**`) and `.agents/skills/framework-expert/**` cleanly passing `doc_lint.py`.

## 2. Detailed Specification

### 2.1 Web Portal UI (`modules/TimeManager/web/time.js`)
First read `.agents/skills/modern-web-guidance/` or ensure strict XSS escaping (`window.escapeHtml`) and semantic accessible form markup:
1. **Sync Status Card (`#time-sync-status`):**
   - Display active effective timezone (`config.activeTimezone || config.timezone`) and active POSIX (`config.activePosix || config.posix`) cleanly escaped via `window.escapeHtml`.
   - If `config.dhcpTzActive`, show an inline badge/indicator cleanly labeled with source (`Option 100 (PCode)` when `dhcpTzSource === 'pcode'`, `Option 101 (TCode)` when `dhcpTzSource === 'tcode'`).
   - If `config.dhcpNtpActive` and non-empty `config.dhcpNtpServers`, show the discovered DHCP NTP server IPs cleanly escaped (`window.escapeHtml`).
2. **Timezone Card (`#time-tz-section`):**
   - Add a clear toggle checkbox `"Use DHCP Timezone (RFC 4833 Options 100/101)"` (`id="time-dhcp-tz"`), initialized from `config.dhcpTz !== false`.
   - Include concise helper text explaining that when enabled and supplied by DHCP, DHCP overrides the manual timezone below without flash writes, while the selector/POSIX override remains the persistent fallback.
   - If DHCP timezone is currently active (`config.dhcpTzActive`), render a visible status banner (`id="time-dhcp-tz-status"`) showing the active DHCP timezone/POSIX and source (`pcode` / `tcode`).
3. **NTP Servers Section (`#time-ntp-section`):**
   - Add a toggle checkbox `"Use DHCP NTP Servers (Option 42)"` (`id="time-dhcp-ntp"`), initialized from `config.dhcpNtp !== false`.
   - If `config.dhcpNtpActive` and `Array.isArray(config.dhcpNtpServers) && config.dhcpNtpServers.length > 0`, show discovered server IPs (`id="time-dhcp-ntp-servers"`) cleanly escaped, with helper text noting static servers below act as fallback/trailing slots.
4. **Save Payload (`#btn-time-save` click handler):**
   - Include `dhcpNtp` (`Boolean(dhcpNtpEl.checked)`) and `dhcpTz` (`Boolean(dhcpTzEl.checked)`) in the `POST /api/time/config` JSON payload alongside existing `timezone`, `posix`, `ntpServer1`, `ntpServer2`, `ntpServer3`.
   - Note: `portalBuilder.py` compiles `modules/TimeManager/web/time.js` into gzip header assets at build time (`scripts/portalBuilder.py` outputs generated headers into `include/portal_assets/` — check whether generated headers are gitignored or tracked before finishing; do NOT edit anything outside `allowed_files`!). Let's verify `portal_assets` in `.gitignore` first.

### 2.2 Mock Server & Playwright E2E Suite (`test/web/server.js`, `test/web/tests/time.spec.ts`)
1. `test/web/server.js`:
   - **DO NOT** alter `MOCK_TABS` (`navigation.spec.ts` asserts exact default mock tabs!).
   - Add `MOCK_TIME_CONFIG` and `MOCK_TIME_ZONES` plus routes in `handleApiRoute`:
     - `GET /api/time/config` -> `MOCK_TIME_CONFIG` (with `timezone`, `posix`, `activeTimezone`, `activePosix`, `ntpServer1..3`, `dhcpNtp`, `dhcpTz`, `dhcpNtpActive`, `dhcpNtpServers`, `dhcpTzActive`, `dhcpTzSource`, `configured`, `syncStatus`, `localTime`).
     - `POST /api/time/config` -> merges submitted JSON cleanly and echoes updated state.
     - `GET /api/time/zones` -> minimal valid map (`"Europe/London"`, `"America/New_York"`, `"UTC"`, etc.).
     - `POST /api/time/zones/update` -> returns `200 { status: "ok" }`.
2. `test/web/tests/time.spec.ts`:
   - Define test-local `MOCK_TABS` (adding `{ name: 'Time', id: 'time', parent: 'System', order: 5 }`) and `MOCK_SYSTEM`, intercepting `**/api/tabs`, `**/api/system`, `**/api/time/config`, `**/api/time/zones`.
   - Verify:
     - Active DHCP Timezone badge/status renders `Option 101 (TCode)` / `Option 100 (PCode)` and active timezone/POSIX safely.
     - Active DHCP NTP servers render safely.
     - XSS payload inside mocked `dhcpNtpServers` or `activeTimezone` / `activePosix` is escaped (does NOT inject `<script>` or `<img>` elements).
     - Toggling `#time-dhcp-tz` / `#time-dhcp-ntp` and clicking `#btn-time-save` sends updated `dhcpNtp` and `dhcpTz` booleans in `POST /api/time/config`.

### 2.3 Developer Documentation & Skill Updates (`docs/developer/**`, `.agents/skills/framework-expert/**`)
- Update relevant developer docs (e.g., `docs/developer/config-system.md`, `docs/developer/mcp-server.md`, `docs/developer/architecture-overview.md`, `docs/developer/using-optional-modules.md`, and/or a dedicated section) documenting:
  - Schema v2 migration for `TimeManager` (`dhcpNtp`, `dhcpTz`, v1->v2 behavior).
  - LwIP DHCP Option 42 (NTP) and RFC 4833 Options 100 (`PCode`) & 101 (`TCode`) hook design, three-app CMake/Kconfig parity (`CMakeLists.txt`, `sdkconfig.defaults`, `setEventBus`), zero flash wear, and Option 2 exclusion.
  - `MCPBuiltinTools` `"system_info"` new `time_dhcp_*` fields.
- Update `.agents/skills/framework-expert/` (`SKILL.md` / `references/` as appropriate) so agents scaffolding new consumer apps include `timeManager.setEventBus(&app.getEventBus())`, `lwip_dhcp_tz_hooks.h` CMake injection, and DHCP Kconfig options.
- Ensure `python3 scripts/doc_lint.py` passes cleanly!

## 3. Verification Requirements

1. Run `python3 scripts/doc_lint.py && cd test/web && npx playwright test` — confirm ALL Playwright tests (including existing suites + new `time.spec.ts`) and `doc_lint.py` pass with zero failures.
2. Run PlatformIO MCP `build_project` (`esp32dev`) after updating `modules/TimeManager/web/time.js` to confirm `portalBuilder.py` gzips the updated `time.js` and compiles cleanly (`acquire_lock` -> `build_project` -> `release_lock`).
3. Update `completion` & `## Completion Report` in this `WORKORDER.md` BEFORE running `pa wo verify WO-320.3`.

## Completion Report
