# PR-15: Web Portal Testing Infrastructure

## Overview
Establish a complete 3-phase web testing strategy for the ESP32 device portal. The portal is a vanilla HTML/CSS/JS SPA served from LittleFS, currently with no local test harness, no mock API server, and no automated browser tests.

## Parent Branch
`main`

## Branch Name
`pr-15-web-testing` ✅ Complete

## Work Order Ledger

| WO | Topic | Status | Commit |
|----|-------|--------|--------|
| **[x] [wo1-test-server](wo1-test-server/WORKORDER.md)** | Local dev server + mock API endpoints | ✅ MERGED | `6efa691` |
| **[x] [wo2-playwright-tests](wo2-playwright-tests/WORKORDER.md)** | 48 Playwright test specs (9 feature areas) | ✅ MERGED | `cc8b515` |
| **[x] [wo3-build-gate](wo3-build-gate/WORKORDER.md)** | PlatformIO build-phase gate script | ✅ MERGED | `9d3a113` |
| **[x] [wo4-documentation](wo4-documentation/WORKORDER.md)** | Web testing strategy workflow document | ✅ MERGED | `610851a` |

## Final Verification
- 48/48 Playwright tests passing (6.2s)
- Build gate script validated (standalone + skip mechanism)
- All work orders merged to integration branch

## Portal Bugs Discovered
1. **`order: 0 || 100` sorting bug** (app.js) — `order: 0` treated as falsy, making Keys the default sub-pill instead of Status
2. **Mock server tab parent casing** — corrected during WO2 to use `'System'` (proper case) matching `tabIdToName()` output

## Architecture Constraints Honoured
- Zero portal source files modified (`web/portal/*`)
- Screenshot endpoint skipped (user decision)
- No live hardware test spec (deferred)
- No CI/GitHub Actions wiring (deferred)

## Distillation
[Context Bridge](./context_bridge.md) — Strategic decisions, portal bugs, API surface catalog.

---

*Completed by Principal Architect — 2026-06-02*
