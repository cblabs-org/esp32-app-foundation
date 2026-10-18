---
name: web-testing-wo1
description: Handoff instructions for Work Order 1 — Local dev server and mock API endpoints
---

# WO1 — Local Dev Server + Mock API Endpoints

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the foundational test infrastructure: a zero-dependency Node.js HTTP server that serves the portal files statically and provides mock API responses for all device endpoints. This server is what Playwright will auto-start for every test run.

## Strict Boundary Box

- **DO NOT** modify any file outside `test/web/`
- **DO NOT** modify any portal source file in `web/portal/`
- **DO NOT** add runtime dependencies — the server uses only Node.js built-ins (`http`, `fs`, `path`)
- **DO NOT** install npm packages yet (that's WO2's job)
- **DO NOT** create test spec files (that's WO2)

## Line-Item Instructions

### 1. `test/web/server.js` — Mock Server

- [ ] Create a Node.js HTTP server using `http.createServer()` (zero dependencies)
- [ ] **Static file serving**: Serve files from `../../web/portal/` relative to the script location
  - Resolve `/` and `/index.html` to `web/portal/index.html`
  - Set correct MIME types: `.html` → `text/html`, `.css` → `text/css`, `.js` → `application/javascript`, `.json` → `application/json`
  - Return 404 for missing files
- [ ] **Port**: Read from `PORT` env var, default to `3000`
- [ ] **Mock API endpoints**: Implement the following routes with realistic mock data:

#### Boot-critical endpoints (SPA won't render without these):

| Route | Method | Mock Response |
|-------|--------|---------------|
| `GET /api/app` | GET | `{"name": "Foundation Test App"}` |
| `GET /api/tabs` | GET | Array of tab objects matching the real navigation hierarchy. Must include: `[{name: "WiFi", id: "wifi", parent: null, order: 0}, {name: "System", id: "system", parent: null, order: 100}, {name: "Status", id: "system", parent: "system", order: 0}, {name: "Keys", id: "keys", parent: "system", order: 1}, {name: "MCP", id: "mcp", parent: "system", order: 2}, {name: "Backup", id: "backup", parent: "system", order: 3}, {name: "OTA", id: "ota", parent: "system", order: 4}]` |
| `GET /api/system` | GET | Rich telemetry with values that exercise RAG thresholds. See detailed mock below. |

#### WiFi endpoints:

| Route | Method | Mock Response |
|-------|--------|---------------|
| `GET /api/wifi/status` | GET | `{mode: "STA", ssid: "TestNetwork", ip: "192.168.1.100", mac: "AA:BB:CC:DD:EE:FF", rssi: -55, channel: 6, gateway: "192.168.1.1", ap_mode: false, passExists: true}` |
| `GET /api/wifi/scan` | GET | `{networks: [{ssid: "HomeWiFi", rssi: -35, secure: true}, {ssid: "OfficeNet", rssi: -60, secure: true}, {ssid: "GuestNet", rssi: -75, secure: false}, {ssid: "FarAway", rssi: -90, secure: true}]}` |
| `POST /api/wifi/test` | POST | `{status: "ok", ip: "192.168.1.100"}` |
| `POST /api/wifi/connect` | POST | `{status: "connecting"}` |
| `POST /api/wifi/forget` | POST | `{status: "forgotten"}` |

#### System endpoints:

| Route | Method | Mock Response |
|-------|--------|---------------|
| `POST /api/system/restart` | POST | `{status: "restarting"}` |
| `POST /api/system/factory-reset` | POST | `{status: "resetting"}` |

#### Keys endpoints:

| Route | Method | Mock Response |
|-------|--------|---------------|
| `GET /api/keys` | GET | `[{id: "weather_key"}, {id: "wifi_password"}, {id: "api_token"}]` |
| `POST /api/keys` | POST | `{status: "saved"}` |
| `DELETE /api/keys` | DELETE | `{status: "deleted"}` |

#### MCP endpoints:

| Route | Method | Mock Response |
|-------|--------|---------------|
| `GET /api/mcp/status` | GET | `{initialized: true, toolCount: 5}` |
| `GET /api/mcp/config` | GET | `{enabled: true, readOnly: false}` |
| `POST /api/mcp/config` | POST | Echo back sent JSON |
| `POST /api/mcp` | POST | JSON-RPC response: `{jsonrpc: "2.0", id: 1, result: {tools: [{name: "get_system_info", description: "Get device system information", mutatesState: false}, {name: "restart_device", description: "Restart the device", mutatesState: true}, {name: "list_keys", description: "List stored API keys", mutatesState: false}]}}` |

#### Backup/Config endpoints:

| Route | Method | Mock Response |
|-------|--------|---------------|
| `GET /api/config/backup` | GET | JSON config blob with `Content-Disposition: attachment; filename="config-backup.json"` header |
| `POST /api/config/restore` | POST | `{status: "ok"}` |
| `GET /api/config` | GET | `{namespaces: ["wifi", "display", "ota", "keys"], count: 4}` |

#### OTA endpoints:

| Route | Method | Mock Response |
|-------|--------|---------------|
| `POST /api/ota` | POST | Parse `action` from JSON body: `"checkGithub"` → `{msg: "Update check started"}`, `"status"` → `{state: "idle", lastError: ""}`, `"rollback"` → `{status: "rolling back"}` |
| `POST /update` | POST | `{status: "ok"}` |

#### System telemetry mock (for `GET /api/system`):

```javascript
{
  firmware_version: "1.0.0-test",
  build_serial: "20260601-001",
  sdk_version: "v5.4.1",
  chip_model: "ESP32-S3",
  chip_revision: 1,
  chip_cores: 2,
  flash_size: 8388608,
  partition: "app0",
  uptime: 3661,        // 1h 1m 1s
  chip_temp: 48.5,     // Amber threshold
  heap_total: 249856,
  heap_free: 124928,   // ~50% — green threshold boundary
  heap_min_free: 45000, // ~18% — red threshold
  heap_max_alloc: 110000,
  psram_total: 2097152,
  psram_free: 1800000,
  storage_used: 786432,
  storage_total: 1048576, // 75% used — amber threshold
  ap_mode: false,
  wifi_ssid: "TestNetwork",
  wifi_ip: "192.168.1.100",
  wifi_mac: "AA:BB:CC:DD:EE:FF",
  wifi_rssi: -55
}
```

Note: The values are deliberately chosen to exercise multiple RAG threshold states (green, amber, red) across different metrics.

- [ ] **Catch-all**: Any unmatched `/api/*` route returns `404` with `{error: "Not found"}`
- [ ] **Logging**: Print `[mock-server] Listening on http://localhost:<port>` on startup
- [ ] **Logging**: Print each API request as `[mock-server] <METHOD> <URL>` for debugging

### 2. `test/web/package.json` — Package Definition

- [ ] Create with:
```json
{
  "name": "esp32-app-foundation-web-tests",
  "private": true,
  "scripts": {
    "test": "playwright test",
    "test:headed": "playwright test --headed",
    "server": "node server.js"
  },
  "devDependencies": {
    "@playwright/test": "^1.52.0",
    "@types/node": "^22.0.0",
    "typescript": "^5.8.0"
  }
}
```

### 3. `test/web/tsconfig.json` — TypeScript Config

- [ ] Create with standard Playwright-compatible settings:
```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "commonjs",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "resolveJsonModule": true,
    "outDir": "./dist"
  },
  "include": ["tests/**/*.ts", "*.ts"]
}
```

### 4. `test/web/playwright.config.ts` — Playwright Configuration

- [ ] Create with:
  - `testDir: './tests'`
  - `fullyParallel: true`
  - `forbidOnly: !!process.env.CI`
  - `retries: process.env.CI ? 2 : 0`
  - `baseURL: process.env.BASE_URL || 'http://localhost:3000'`
  - Single project: `{ name: 'chromium', use: { ...devices['Desktop Chrome'] } }`
  - `webServer: { command: 'node server.js', url: 'http://localhost:3000', reuseExistingServer: !process.env.CI }`
  - Reporter: `'html'`
  - Trace: `'on-first-retry'`

### 5. Create `test/web/tests/` directory

- [ ] Create the empty `tests/` directory (it will be populated by WO2)
- [ ] Optionally create a minimal smoke test (`smoke.spec.ts`) that verifies the server starts and the portal loads:
```typescript
import { test, expect } from '@playwright/test';

test('portal loads successfully', async ({ page }) => {
  await page.goto('/');
  await expect(page).toHaveTitle(/Portal/i);
});
```

### 6. Add `test/web/` to `.gitignore` exclusions

- [ ] Ensure `node_modules/` inside `test/web/` is git-ignored. Check the project root `.gitignore` — if it already has a global `node_modules/` entry, no action needed. If not, add `test/web/node_modules/` to `.gitignore`.

## Verification

After completing all items:

1. Start the server manually and verify it responds:
```bash
cd test/web
node server.js &
curl -s http://localhost:3000/ | head -5
curl -s http://localhost:3000/api/system | python3 -m json.tool
curl -s http://localhost:3000/api/tabs | python3 -m json.tool
curl -s http://localhost:3000/api/wifi/status | python3 -m json.tool
kill %1
```

2. Verify static file MIME types are correct:
```bash
curl -sI http://localhost:3000/app.css | grep Content-Type
# Should show text/css
```

3. Verify 404 handling:
```bash
curl -s http://localhost:3000/api/nonexistent
# Should return {"error": "Not found"}
```

4. **STOP and send a message back to the Principal Architect** reporting validation results. Do NOT wrap up or distill until the PA explicitly approves.
