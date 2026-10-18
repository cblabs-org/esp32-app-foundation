# Project: pr-portal-asset-decentralisation

> Decentralise optional module web assets so each module owns its JS source,
> serves its own routes, and declares its script via `getTabScript()`. Core
> framework assets stay in `web/portal/`. Display contract assets move to
> `web/display/`. Module-specific display assets move into their owning module.
> Closes #96.

## Human Approval

- [x] Decomposition approved by human: 2026-08-13T10:47+01:00

## Work Order Ledger

| WO | Branch | Status | Summary |
|----|--------|--------|---------|
| **[x] [wo1-pipeline-relocation](wo1-pipeline-relocation/WORKORDER.md)** | `portal-decentralise/wo1-pipeline-relocation` | ✅ Complete (`4a276f4b`) | Update portalBuilder.py to scan module dirs + web/display/; move 9 JS/HTML files to their owners |
| **[x] [wo2-route-decentralisation](wo2-route-decentralisation/WORKORDER.md)** | `portal-decentralise/wo2-route-decentralisation` | ✅ Complete (`e0d598e3`) | Move route serving + includes from WebServerManager into individual modules; add getTabScript() overrides |
| **[x] [wo3-dynamic-script-loading](wo3-dynamic-script-loading/WORKORDER.md)** | `portal-decentralise/wo3-dynamic-script-loading` | ✅ Complete (`bc929ff4`) | Trim app.js hardcoded script list to core-only; verify dynamic loading via /api/tabs |

## Session Log

### Session 1 — 2026-08-13 (Decomposition)
- Researched full portal architecture: 17 JS files, portalBuilder.py pipeline, WebServerManager route registration, IWebContributor interface, HomeAssistant self-serving pattern.
- User clarified display asset ownership: split by IDisplayModule contract (display.js, screenshot.js/html → shared web/display/) vs module-specific (carousel.js → U8G2, hardware.js → LVGL).
- Decomposed into 3 WOs: pipeline+relocation → route decentralisation → dynamic script loading.

### Session 2 — 2026-08-13 (Execution + Merge)
**Summary**: All 3 WOs executed by worker agents, device-verified on CrowPanel, merged as PR #109.
**Cross-cutting decisions**:
- `portalBuilder.py` scans `web/portal/`, `web/display/`, and `modules/*/web/` with basename collision detection (fail-fast `sys.exit(1)`).
- Display contract assets (shared by both U8G2 and LVGL) live in `web/display/`, not inside either module — follows IDisplayModule interface boundary.
- `screenshot.js` has no tab contributor — lazy-loaded by `display.js` on tab activation rather than being in the core script list.
- Both display modules serve the shared contract assets (`display.js`, `screenshot.js`, `screenshot.html`) plus their own module-specific JS.
- `test/web/server.js` updated to search across all source directories for relocated assets.
**Surfaced findings**:
- Playwright web test runner exits non-zero despite all 46 tests passing (Node.js deprecation warnings). Pre-existing issue — causes false FAILED in PIO but firmware builds succeed.
- `test_mcpServer` and `test_pngWriter` ERRORED (2/660) — pre-existing, unrelated to this PR.
**Dead ends**: None.

### 2026-08-14 — Project Wrap-Up
**Architectural outcome**: Optional modules are now fully self-contained for web assets. Removing a module from a downstream `platformio.ini` cleanly removes its JS source, route serving, and dynamic script discovery. Adding a new module's portal tab requires only: (1) place JS in `modules/<Name>/web/`, (2) serve it in `registerRoutes()`, (3) override `getTabScript()`. No touching WebServerManager or app.js.
**Lessons for future projects**: The HomeAssistant self-serving pattern was the correct precedent — extending it to all optional modules was mechanical. The key design decision (display contract vs module-specific) needed user input and was worth pausing for.
**Unresolved items**: Playwright test runner false-failure exit code should be investigated separately (not related to portal decentralisation).

