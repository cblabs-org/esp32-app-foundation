---
name: web-testing-wo2
description: Handoff instructions for Work Order 2 — Playwright test specs
---

# WO2 — Playwright Test Specs

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Write comprehensive Playwright test specs covering every tab and interaction pattern in the ESP32 portal SPA. Tests mock API responses at the `page.route()` level to control exactly what each test sees.

## Prerequisites

WO1 must be complete. The following files must exist:
- `test/web/server.js` — Mock server
- `test/web/package.json` — With Playwright dependency
- `test/web/playwright.config.ts` — With `webServer` auto-start
- `test/web/tsconfig.json`

You must run `npm install` and `npx playwright install chromium` in `test/web/` before writing tests.

## Strict Boundary Box

- **DO NOT** modify any portal source file in `web/portal/`
- **DO NOT** modify `test/web/server.js` (use `page.route()` for all test-specific mocking)
- **DO NOT** modify `test/web/package.json`, `playwright.config.ts`, or `tsconfig.json`
- **DO NOT** create the build gate script (that's WO3)
- All test files go in `test/web/tests/`

## Important Portal Architecture Notes

Before writing tests, you MUST understand these portal behaviours:

1. **Dynamic script loading**: Only `app.js` is in `index.html`. It dynamically injects `utils.js`, `wifi.js`, `system.js`, `backup.js`, `ota.js`, `keys.js`, `mcp.js`, `screenshot.js` via `loadScripts()` with `onload` chaining. Tests must wait for all scripts to load before asserting.

2. **API queue**: All API calls go through `window.apiFetch()` which serialises requests through an `ApiQueue`. Multiple rapid API calls don't execute in parallel.

3. **Tab events**: Tab switching dispatches `CustomEvent('tab-changed')` with the tab ID. Fragment scripts listen for this to initialise their UI.

4. **Theme system**: Three modes (auto/light/dark). Stored in `localStorage` key `portal-theme`. The `<html>` element gets `data-theme` attribute.

5. **Navigation hierarchy**: Bottom nav shows top-level tabs (WiFi, System). System tab has a sub-pill bar with children (Status, Keys, MCP, Backup, OTA). The tab structure comes from `/api/tabs`.

6. **Armed confirmations**: Dangerous actions (forget WiFi, restart, factory reset, delete key, rollback) use a pattern where the button is disabled for 3 seconds after first click, showing "Are you sure?", then enabled for the actual action.

7. **Screenshot tab is EXCLUDED from testing** — it uses `fetch()` directly (not `apiFetch()`) and the `/api/display/screenshot` binary endpoint is not mocked.

## Line-Item Instructions

### Common Test Pattern

Every spec should follow this pattern:

```typescript
import { test, expect, Page } from '@playwright/test';

// Mock data objects defined at describe scope
const mockSystemData = { /* ... */ };

test.describe('Feature Name', () => {
  test.beforeEach(async ({ page }) => {
    // Mock boot-critical endpoints so the SPA loads
    await page.route('**/api/app', route => route.fulfill({
      json: { name: 'Foundation Test App' }
    }));
    await page.route('**/api/tabs', route => route.fulfill({
      json: [/* tab hierarchy */]
    }));
    await page.route('**/api/system', route => route.fulfill({
      json: mockSystemData
    }));
    await page.route('**/api/wifi/status', route => route.fulfill({
      json: { /* wifi status */ }
    }));

    await page.goto('/');
    // Wait for dynamic scripts to load
    await page.waitForFunction(() => window.apiFetch !== undefined);
  });

  test('specific behaviour', async ({ page }) => {
    // Layer on endpoint-specific mocks
    // Assert behaviour
  });
});
```

**Critical**: The `beforeEach` must mock ALL boot-critical endpoints (`/api/app`, `/api/tabs`, `/api/system`, `/api/wifi/status`) because the SPA calls them on load. Tests will fail with fetch errors otherwise.

### 1. `test/web/tests/navigation.spec.ts` — Tab Navigation

- [ ] Create with 5-6 tests covering:
  - **Bottom nav renders** — WiFi and System tabs visible with icons
  - **Tab switching** — Clicking System shows system content, hides WiFi content
  - **Sub-pill bar appears** — When System tab is active, pill bar shows child tabs (Status, Keys, MCP, Backup, OTA)
  - **Sub-pill activation** — Clicking a sub-pill shows its content panel
  - **Tab-changed event fires** — Verify `CustomEvent('tab-changed')` dispatches with correct detail
  - **Active state memory** — Switch to System→Keys, switch to WiFi, switch back to System → Keys should still be active

### 2. `test/web/tests/wifi.spec.ts` — WiFi Settings Tab

- [ ] Create with 7-8 tests covering:
  - **Status header renders** — Mode badge, SSID, IP address, signal bars from `/api/wifi/status`
  - **SSID auto-populates** — Input field pre-filled from current connection
  - **Network scan** — Scan button triggers `/api/wifi/scan`, network list renders with signal bars and lock icons
  - **Network selection** — Clicking a scanned network fills the SSID field
  - **Password existing indicator** — When `passExists: true`, password shows `●●●●●●●● (saved)` placeholder
  - **Eye toggle state** — Eye toggle disabled for existing password, enabled when user types
  - **Test connection** — POST to `/api/wifi/test`, shows success IP or failure message
  - **Forget network** — Armed confirmation (click → "Are you sure?" disabled 3s → enabled)

### 3. `test/web/tests/system.spec.ts` — System Status Sub-pill

- [ ] Create with 5-6 tests covering:
  - **Device info card** — Chip model, SDK version, firmware version render from `/api/system`
  - **Memory bars RAG colours** — Green (≥50% free), amber (≥25%), red (<25%). Use mock data with `heap_min_free` at 18% to trigger red
  - **Storage bar** — Shows used/total with correct percentage
  - **Temperature display** — RAG threshold colouring
  - **Restart** — Armed confirmation flow
  - **Factory reset** — Double confirmation: type "RESET" in input → confirm button enables

### 4. `test/web/tests/theme.spec.ts` — Theme Toggle

- [ ] Create with 4-5 tests covering:
  - **Auto mode** — Follows `prefers-color-scheme` (use `page.emulateMedia()`)
  - **Light mode** — Sets `data-theme="light"` on `<html>`, CSS variables change
  - **Dark mode** — Sets `data-theme="dark"` on `<html>`
  - **Persistence** — Theme saved to `localStorage` key `portal-theme`, survives reload
  - **Toggle buttons** — Correct button shows active state for current theme

### 5. `test/web/tests/keys.spec.ts` — Key Vault Sub-pill

- [ ] Create with 4-5 tests covering:
  - **Key list renders** — IDs displayed with masked dots
  - **WiFi password protection** — `wifi_password` key shows "Managed by WiFi tab" badge, no delete button
  - **Add key flow** — Enter ID + value → POST to `/api/keys` → key appears in refreshed list
  - **Delete key** — Armed confirmation (3s timeout), DELETE request sent
  - **Empty state** — When `/api/keys` returns empty array, appropriate empty message shown

### 6. `test/web/tests/backup.spec.ts` — Backup & Restore Sub-pill

- [ ] Create with 3-4 tests covering:
  - **Export config** — Triggers download; verify the download event fires (intercept via `page.on('download')`)
  - **Config namespaces** — Pill badges render from `/api/config` response
  - **Import config** — File chooser accepts JSON, POST sent to `/api/config/restore`
  - **Invalid import** — Non-JSON file shows error toast

### 7. `test/web/tests/mcp.spec.ts` — MCP Controls Sub-pill

- [ ] Create with 4-5 tests covering:
  - **Status indicator** — Active/Inactive based on `initialized` field
  - **Tool count badge** — Shows count from `/api/mcp/status`
  - **Toggle switches** — Render from `/api/mcp/config`, interaction sends POST
  - **Tool list** — JSON-RPC `tools/list` response renders tools with MUTATING/READ-ONLY badges
  - **Error handling** — If MCP status returns error, shows appropriate state

### 8. `test/web/tests/ota.spec.ts` — OTA Update Sub-pill

- [ ] Create with 4-5 tests covering:
  - **Firmware info card** — Version and partition render from `/api/system`
  - **Check for update flow** — POST `checkGithub` → polling `status` → progress bar
  - **Manual upload** — File input accepts `.bin`, FormData POST to `/update`
  - **Rollback** — Armed confirmation (3s timeout)
  - **Error states** — Update check fails, shows error message

### 9. `test/web/tests/ap_mode.spec.ts` — AP Mode Behaviour

- [ ] Create with 2-3 tests covering:
  - **AP banner visible** — When `/api/system` returns `ap_mode: true`, body gets `ap-mode` class, banner visible
  - **Tabs hidden in AP mode** — Non-essential tabs hidden when in AP mode
  - **Normal mode** — AP banner hidden, all tabs visible

## Verification

After completing all specs:

1. Install dependencies:
```bash
cd test/web
npm install
npx playwright install chromium
```

2. Run all tests:
```bash
npm test
```

3. All tests must pass. If any fail, debug and fix.

4. Run with HTML report:
```bash
npx playwright test --reporter=html
npx playwright show-report
```

5. **STOP and send a message back to the Principal Architect** reporting:
   - Total number of tests
   - Pass/fail status
   - Any portal bugs discovered (do NOT fix them — just report)
   
Do NOT wrap up or distill until the PA explicitly approves.
