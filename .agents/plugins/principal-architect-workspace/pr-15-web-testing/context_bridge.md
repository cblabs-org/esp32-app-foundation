---
title: "PR-15: Web Portal Testing Infrastructure"
distilled_at: 2026-06-02T19:40:00Z
original_plan_id: pr-15-web-testing
artifacts:
  - context_artifacts/adr_testing_architecture.md
  - context_artifacts/graveyard_portal_bugs.md
  - context_artifacts/state_api_surface.md
---

# PR-15: Web Portal Testing Infrastructure — Context Bridge

## Executive Summary

Established a complete 3-phase web testing strategy for the ESP32 app-foundation device portal: a zero-dependency Node.js mock server (20+ endpoints), 48 Playwright browser tests across 9 feature areas (6.2s runtime), a PlatformIO build gate (`sys.exit(1)` on failure), and workflow documentation. Merged to `main` as `f03972e`.

## Next Steps

1. **Fix the two portal bugs** discovered during testing:
   - `app.js` L158: `(a.order || 100)` → `(a.order != null ? a.order : 100)` — fixes Status sub-pill default ordering
   - Mock server `server.js` L37-41: already corrected in tests to use `parent: 'System'` (proper case), but verify firmware sends the same

2. **Deferred items** for future PRs:
   - `live_portal.spec.ts` — Phase 3 live hardware test spec (uses `BASE_URL` env var to target a real ESP32)
   - GitHub Actions CI wiring — run `npx playwright test` in CI pipeline
   - Screenshot endpoint mocking — `/api/display/screenshot` binary framebuffer

3. **To add tests for new portal features:** Create `.spec.ts` files in `test/web/tests/`, mock boot-critical endpoints in `beforeEach`, use `page.route()` for endpoint-specific mocks. Run `npm test` in `test/web/`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_testing_architecture.md` - Why zero-dependency server, route-level mocking, TypeScript for specs, build gate ordering
- `context_artifacts/graveyard_portal_bugs.md` - Two portal bugs found: order:0 falsy evaluation, tab parent casing mismatch
- `context_artifacts/state_api_surface.md` - Complete catalog of all 20 portal API endpoints with mock response shapes
