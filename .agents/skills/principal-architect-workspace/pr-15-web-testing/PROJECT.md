# PR-15: Web Portal Testing Infrastructure

## Overview
Establish a complete 3-phase web testing strategy for the ESP32 device portal. The portal is a vanilla HTML/CSS/JS SPA served from LittleFS, currently with no local test harness, no mock API server, and no automated browser tests.

## Parent Branch
`main` (pr-04a-portal-enhancement has been merged)

## Branch Name
`pr-15-web-testing` ✅ Created

## Work Order Ledger

| WO | Topic | Status | Branch |
|----|-------|--------|--------|
| **[ ] [wo1-test-server](wo1-test-server/WORKORDER.md)** | Local dev server + mock API endpoints | QUEUED | `pr-15-web-testing/wo1-test-server` |
| **[ ] [wo2-playwright-tests](wo2-playwright-tests/WORKORDER.md)** | Playwright test specs + configuration | QUEUED | `pr-15-web-testing/wo2-playwright-tests` |
| **[ ] [wo3-build-gate](wo3-build-gate/WORKORDER.md)** | PlatformIO build-phase gate script | QUEUED | `pr-15-web-testing/wo3-build-gate` |
| **[ ] [wo4-documentation](wo4-documentation/WORKORDER.md)** | Web testing strategy workflow document | QUEUED | `pr-15-web-testing/wo4-documentation` |

## Architecture Constraints
- **DO NOT** modify any portal source files (`web/portal/*`)
- Test server serves portal files as-is (no bundling, no transpilation)
- Most API mocking done at Playwright `page.route()` level, not in the server
- Server provides only baseline mock endpoints needed for page load
- All tests must account for dynamic script loading and `CustomEvent('tab-changed')` patterns
- Screenshot endpoint (`/api/display/screenshot`) is NOT mocked (user decision)
- No live hardware test spec in this PR (deferred)
- No CI/GitHub Actions wiring (deferred)

## Dependencies
- Node.js (already available on dev machines)
- Playwright (`@playwright/test`)
- TypeScript (for test specs)
- Python 3 (for build gate script, already used by `portalBuilder.py`)

---

*Created by Principal Architect — 2026-06-02*
