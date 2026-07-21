# PR-43: Upstream Issue Fixes (Issues #18–#23)

## Overview

Fix all 6 open issues filed by the power-monitor consumer program (PR-6/PR-7).
Three code fixes (capture decoder, tab script contribution, MCP config safety)
and three docs fixes (display docs, ADR-15, portal docs), plus global body-size
middleware.

**GitHub Issues:** #18, #19, #20, #21, #22, #23
**Base Branch:** `main`
**Filed by:** power-monitor (PR-6 `pr-06-display`, PR-7 `pr-07-web-ui`)
**Human Approval:** ✅ APPROVED 2026-07-21T00:16:56+01:00

---

## Work Order Ledger

| WO | Topic | Complexity | Status | Closes |
|---|---|---|---|---|
| **[ ] [wo1-capture-decoder](wo1-capture-decoder/WORKORDER.md)** | Fix captureU8g2.py decoder dispatch + device format field | standard | ⬜ | #18 |
| **[ ] [wo2-display-docs](wo2-display-docs/WORKORDER.md)** | Rewrite display system docs to match real IDisplayBoard API | standard | ⬜ | #19 |
| **[ ] [wo3-tab-script](wo3-tab-script/WORKORDER.md)** | Tab script contribution (IWebContributor + WebServerManager + app.js) | complex | ⬜ | #20 |
| **[ ] [wo4-mcp-config-safety](wo4-mcp-config-safety/WORKORDER.md)** | MCP config safety (deep-merge + namespace scoping) | standard | ⬜ | #21 |
| **[ ] [wo5-body-middleware](wo5-body-middleware/WORKORDER.md)** | Provider-declared body limits + global middleware | complex | ⬜ | #22 |
| **[ ] [wo6-portal-docs](wo6-portal-docs/WORKORDER.md)** | Rewrite web portal docs (overview.md + html-js-fragments.md) | standard | ⬜ | #23 |
| **[ ] [wo7-template-app](wo7-template-app/WORKORDER.md)** | Update template-app for new IWebContributor methods | standard | ⬜ | — |
| **[ ] [wo8-sample-app](wo8-sample-app/WORKORDER.md)** | Update sample-app (WeatherModule) for new IWebContributor methods | standard | ⬜ | — |
| **[ ] [wo9-build-verification](wo9-build-verification/WORKORDER.md)** | Full build + test verification (3 apps × 3 environments) | standard | ⬜ | — |

## Execution Order

WO1 → WO3 → WO4 → WO5 → WO7 → WO8 → WO2 → WO6 → WO9

Code fixes first (WO1-WO5), then app updates (WO7-WO8), then docs (WO2, WO6),
then full build verification last (WO9).

## Branch Strategy

Single feature branch `fix/issues-18-23` from `main`. All WOs committed sequentially.

## Stacking

Standard linear stack — each WO branch bases on the previous:
- `fix/issues-18-23/wo1-capture-decoder` ← main
- `fix/issues-18-23/wo2-display-docs` ← wo1
- etc.

---

*Created: 2026-07-20 by PR-43 Principal Architect session*
