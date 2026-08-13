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
| **[ ] [wo2-route-decentralisation](wo2-route-decentralisation/WORKORDER.md)** | `portal-decentralise/wo2-route-decentralisation` | Pending | Move route serving + includes from WebServerManager into individual modules; add getTabScript() overrides |
| **[ ] [wo3-dynamic-script-loading](wo3-dynamic-script-loading/WORKORDER.md)** | `portal-decentralise/wo3-dynamic-script-loading` | Pending | Trim app.js hardcoded script list to core-only; verify dynamic loading via /api/tabs |

## Session Log

### Session 1 — 2026-08-13 (Decomposition)
- Researched full portal architecture: 17 JS files, portalBuilder.py pipeline, WebServerManager route registration, IWebContributor interface, HomeAssistant self-serving pattern.
- User clarified display asset ownership: split by IDisplayModule contract (display.js, screenshot.js/html → shared web/display/) vs module-specific (carousel.js → U8G2, hardware.js → LVGL).
- Decomposed into 3 WOs: pipeline+relocation → route decentralisation → dynamic script loading.
