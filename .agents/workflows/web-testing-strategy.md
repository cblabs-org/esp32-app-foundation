---
description: The mandatory 3-phase testing workflow for all web portal development in this project.
---

# Web Testing Strategy

This rule defines the mandatory 3-phase testing workflow for all web portal development. All agent sessions MUST follow this sequence to ensure rapid iteration while maintaining quality and avoiding unnecessary hardware flash cycles.

## Phase 1: Local Development & Iteration

**Goal:** Rapidly iterate on UI, UX, and logic using a native test harness.

**Location:** `test/web/`

**Setup (first time only):**

```bash
cd test/web
npm install
npx playwright install chromium
```

**Commands:**

| Action | Command |
|---|---|
| Start dev server | `node server.js` (or `npm run server`) |
| Run automated tests | `npm test` (or `npx playwright test`) |
| Run headed for debugging | `npm run test:headed` |

**Action:** Fix ALL UI issues and logic bugs in this phase. Do NOT move to Phase 2 until local tests pass.

## Phase 2: Build Gate Verification

**Goal:** Verify tests pass as part of the PlatformIO build pipeline.

**Pipeline:**

1. `scripts/run_web_tests.py` — Automatically runs Playwright tests (hard gate — build fails on test failure)
2. `scripts/portalBuilder.py` — Minifies and gzips portal assets into PROGMEM headers

**Commands:**

```bash
pio run -e <target>
```

The test gate runs automatically as a pre-build step.

**Skip mechanism:** Set `SKIP_WEB_TESTS=1` to bypass tests (for firmware-only work).

**Action:** Ensure the build completes without errors. If tests fail, return to Phase 1.

## Phase 3: Hardware Validation (Flash)

**Goal:** Final smoke test on real hardware.

**Pre-requisite:** Phase 2 must pass first.

**Action:** Flash firmware and perform manual smoke test in a browser pointed at the device IP.

## Rationale

- **Phase 1** is the fastest (seconds)
- **Phase 2** ensures the build pipeline stays intact (minutes)
- **Phase 3** is the slowest and uses shared hardware (minutes + queue wait)

## Adding New Tests

- Test files go in `test/web/tests/` with `.spec.ts` extension
- Use `page.route()` for API mocking (not server.js modifications)
- Mock boot-critical endpoints in `beforeEach`: `/api/app`, `/api/tabs`, `/api/system`, `/api/wifi/status`
- Run `npm test` to verify before committing

## Directory Structure

```
test/web/
├── server.js              # Zero-dependency mock server
├── package.json           # Playwright + TypeScript deps
├── playwright.config.ts   # Test runner configuration
├── tsconfig.json          # TypeScript config
└── tests/
    ├── navigation.spec.ts # Tab switching, sub-pills
    ├── wifi.spec.ts       # WiFi settings lifecycle
    ├── system.spec.ts     # System status, restart, reset
    ├── theme.spec.ts      # Auto/light/dark toggle
    ├── keys.spec.ts       # Key vault CRUD
    ├── backup.spec.ts     # Backup & restore
    ├── mcp.spec.ts        # MCP server controls
    ├── ota.spec.ts        # OTA update flows
    └── ap_mode.spec.ts    # AP mode UI changes
```
