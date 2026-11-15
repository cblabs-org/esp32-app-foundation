---
schema_version: 1
wo_id: WO-320.3
pr_id: PR-320
title: Add Time portal DHCP UI, Playwright E2E tests, and update developer docs/skills
complexity: standard
status: COMPLETED
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
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_91de1537e539
  commit_sha: 70dfa3f92d0aa4d4203971f1eb1200abf98b7251
  files_modified:
  - .agents/skills/framework-expert/SKILL.md
  - .agents/skills/framework-expert/guides/bootstrap.md
  - .agents/skills/framework-expert/references/build-system.md
  - .agents/skills/framework-expert/systems/mcp-server/overview.md
  - .agents/skills/framework-expert/systems/web-portal/routes.md
  - docs/developer/build-system.md
  - docs/developer/config-system.md
  - docs/developer/mcp-server.md
  - docs/developer/using-optional-modules.md
  - modules/TimeManager/web/time.js
  - test/web/server.js
  - test/web/tests/time.spec.ts
  verification_summary: doc_lint.py ok; check_doc_cpp_samples.py 50 contracts, 40
    samples passed, 0 failed; Playwright 92/92 passed (time.spec.ts 7/7); esp32dev
    and esp32s3nano PlatformIO builds ok; time_js.hpp regenerated.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Rendered visible styled checkbox inputs paired inline with explicit <label for="...">
    and aria-describedby help text for time-dhcp-tz and time-dhcp-ntp so manual toggles
    update status badges immediately and remain reliable under Playwright and accessibility
    checks.
  - Added esc() (wrapping window.escapeHtml) and escAttr() (also escaping double/single
    quotes) on all dynamic text and attribute sinks in modules/TimeManager/web/time.js,
    eliminating XSS exposure while using CSS custom properties (var(--...)) exclusively
    with zero hex color literals.
  - Left MOCK_TABS untouched in test/web/server.js while adding MOCK_TIME_ZONES, mockTimeConfig,
    and /api/time/* handlers, and mirrored TimeManager::registerTabs (order 15, script
    /time.js) inside test-local MOCK_TABS in test/web/tests/time.spec.ts.
  - Documented Schema v2 migration and zero flash wear, Option 42 trailing slot fallback,
    RFC 4833 Option 100/101 spinlock hook and Option 2 exclusion, three-target CMake/Kconfig/setEventBus
    parity, REST /api/time/config, and MCP system_info time_dhcp_* fields across docs/developer/
    and .agents/skills/framework-expert/.
  dead_ends:
  - Initial PlatformIO MCP build_project call on esp32dev hit cacheHit=true without
    running portalBuilder.py because modules/TimeManager/web/time.js is outside src/include
    cache inputs; resolved by passing forceExecution=true on both esp32dev and esp32s3nano
    builds.
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

### Execution Summary
Implemented the Time tab DHCP NTP (Option 42) and DHCP Timezone (RFC 4833 Options 100/101) web UI, added `/api/time/*` mock server routes and Playwright E2E coverage, and updated developer documentation and `.agents/skills/framework-expert/` guides cleanly:

1. **Web Portal UI (`modules/TimeManager/web/time.js`):**
   - Added `esc()` (`window.escapeHtml`) and `escAttr()` (wrapping `window.escapeHtml` plus single/double quote escaping) across all dynamic text and attribute sinks (including `activeTimezone`, `activePosix`, `timezone`, `posix`, `dhcpTzSource`, `dhcpNtpServers`, `ntpServer1..3`, and `localTime`).
   - Updated `#time-sync-status` to display effective `activeTimezone || timezone`, `activePosix || posix`, inline `DHCP Option 100 (PCode)` / `DHCP Option 101 (TCode)` source badges, and active `Option 42` DHCP NTP server IPs.
   - Added accessible `#time-dhcp-tz` (`Use DHCP Timezone (RFC 4833 Options 100/101)`) and `#time-dhcp-ntp` (`Use DHCP NTP Servers (Option 42)`) checkbox controls with `aria-describedby` help text, live `#time-dhcp-tz-status` and `#time-dhcp-ntp-servers` telemetry/fallback panels updating on toggle, and updated `POST /api/time/config` save payload with `dhcpNtp` and `dhcpTz` booleans.
   - Used CSS custom properties (`var(--...)`) exclusively with zero hardcoded hex color literals anywhere in `time.js`.
2. **Mock Server & Playwright E2E Tests (`test/web/server.js`, `test/web/tests/time.spec.ts`):**
   - Added `MOCK_TIME_ZONES`, `mockTimeConfig`, and `GET`/`POST /api/time/config`, `GET /api/time/zones`, and `POST /api/time/zones/update` routes in `test/web/server.js` while leaving `MOCK_TABS` untouched.
   - Added 7 Playwright E2E tests in `test/web/tests/time.spec.ts` covering active Option 101 (`TCode`) & Option 42 telemetry, Option 100 (`PCode`) badge precedence, disabled manual fallback state, enabled-waiting fallback state, immediate toggle UI updates + `POST /api/time/config` save payload, XSS escaping of untrusted DHCP strings, and mock server `/api/time/*` API contracts.
3. **Developer Documentation & Framework Skill (`docs/developer/**`, `.agents/skills/framework-expert/**`):**
   - Updated `docs/developer/config-system.md` (TimeManager Schema v2 `v1 -> v2` migration and zero flash wear invariant), `docs/developer/mcp-server.md` (`system_info` Time & DHCP `time_dhcp_*` telemetry fields), `docs/developer/build-system.md` (three-target `sdkconfig.defaults` and `CMakeLists.txt` `ESP_IDF_LWIP_HOOK_FILENAME` parity), and `docs/developer/using-optional-modules.md` (`TimeManager` `setEventBus`, Option 42 trailing slot fallback, Option 100/101 spinlock hook & `/timezones.json` streaming lookup, Option 2 non-goal exclusion, and `/api/time/config` schema).
   - Updated `.agents/skills/framework-expert/SKILL.md`, `guides/bootstrap.md`, `references/build-system.md`, `systems/mcp-server/overview.md`, and `systems/web-portal/routes.md` so scaffolded consumer apps include `setEventBus`, LwIP hook CMake injection, and DHCP Kconfig defaults, using repo-relative links only.

### Verification Results
- **Doc Lint & C++ Doc Samples (`python3 scripts/doc_lint.py && python3 scripts/check_doc_cpp_samples.py`):** Exited `0` (33 files scanned, 222 C++ blocks, `50 PASSED` contracts, `40 PASSED` samples, `132 SKIPPED`, `0 FAILED`).
- **Playwright E2E Suite (`cd test/web && npx playwright test`):** `92 passed (8.1s)` — `0` failures across all specs (including 7/7 in `tests/time.spec.ts`).
- **PlatformIO MCP Firmware Builds (`acquire_lock` -> `build_project(..., forceExecution=true)` -> `release_lock`):**
  - `portalBuilder.py` regenerated gitignored `include/portal_assets/time_js.hpp` (`41,389` bytes).
  - `esp32dev`: `SUCCESS` (`00:00:44.801`, RAM `58,460 B` / `17.8%`, Flash `1,346,847 B` / `85.6%`, `[web-tests] All web tests passed`).
  - `esp32s3nano`: `SUCCESS` (`00:00:47.551`, RAM `61,084 B` / `18.6%`, Flash `1,381,115 B` / `32.9%`).
- **Syntax & Style Hygiene:** `node --check modules/TimeManager/web/time.js` and `node --check test/web/server.js` exited `0`; `modules/TimeManager/web/time.js` contains zero hex color literals; doc/skill diffs contain zero `file://` or `/Users/` links.

### Escalation Notice
`pa wo verify WO-320.3` was intentionally **not** executed per explicit instruction (`HARNESS_PORCELAIN_LSTRIP_BUG`), because `WO-320.3` modifies dot-prefixed paths under `.agents/skills/framework-expert/**` and `.agents/plugins/.../WORKORDER.md`, and `pa_cli.py:_parse_porcelain_paths()` calls `.lstrip("./")`, stripping leading dots so `.agents/...` fails scope hygiene against `allowed_files`. All 12 modified/created workspace deliverables lie strictly inside `allowed_files`, `spec_hash` is intact, and all verification commands passed. Escalating to the Principal Architect (`pa`) for receipt stamping and commit creation.
