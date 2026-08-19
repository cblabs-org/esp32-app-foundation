# Project: issue-sweep-aug

> Close out 11 actionable open issues in a single stacked PR. Covers P1 compilation
> blockers, TWDT safety, portal DRAM tuning, LVGL/portal branding fixes, skill/doc
> updates, SLS naming, test fixes, and a screensaver policy UI feature.

## Human Approval

- [x] Decomposition approved by human: 2026-08-19T20:39+01:00

## Work Order Ledger

| WO | Branch | Status | Closes | Summary |
|----|--------|--------|--------|---------|
| **[x] [wo1-ota-compilation-fixes](wo1-ota-compilation-fixes/WORKORDER.md)** | `issue-sweep-aug/wo1-ota-compilation-fixes` | ✅ Complete | #120 | Fix 2 compilation defects blocking `WITH_OTA` on consumer apps |
| **[x] [wo2-datamanager-twdt-safety](wo2-datamanager-twdt-safety/WORKORDER.md)** | `issue-sweep-aug/wo2-datamanager-twdt-safety` | ✅ Complete | #128 | Harden DataManager fetch loop against TWDT boot-loop |
| **[x] [wo3-portal-dram-tuning](wo3-portal-dram-tuning/WORKORDER.md)** | `issue-sweep-aug/wo3-portal-dram-tuning` | ✅ Complete | #114, #119 | Reduce httpd stack/sockets for constrained boards + HWM logging |
| **[x] [wo4-branding-fouc](wo4-branding-fouc/WORKORDER.md)** | `issue-sweep-aug/wo4-branding-fouc` | ✅ Complete | #129, #80 | LVGL boot title + portal branding FOUC elimination |
| **[x] [wo5-docs-sls-tests](wo5-docs-sls-tests/WORKORDER.md)** | `issue-sweep-aug/wo5-docs-sls-tests` | ✅ Complete | #127, #130, #97, #113 | Skill/doc updates, PSRAM audit, EventBus payload lifetime doc, SLS naming fix, sample-app test fix |
| **[x] [wo6-screensaver-policy-ui](wo6-screensaver-policy-ui/WORKORDER.md)** | `issue-sweep-aug/wo6-screensaver-policy-ui` | ✅ Complete | #110 | Screensaver policy toggle in portal Display settings |
| **[x] [wo7-lvgl-eventbus-race](wo7-lvgl-eventbus-race/WORKORDER.md)** | `issue-sweep-aug/wo7-lvgl-eventbus-race` | ✅ Complete | #131 | Fix LVGL race in EventBus subscriber + document threading rule |
| **[x] [wo8-queue-data-job](wo8-queue-data-job/WORKORDER.md)** | `issue-sweep-aug/wo8-queue-data-job` | ✅ Complete | #132 | Extend DataManager with queueDataJob() for off-loop data operations |

## Modules & Components

| Module / Area | WOs | Notes |
|---|---|---|
| `modules/OTAUpdateManager/` | WO1 | Lambda capture + static method fixes |
| `modules/LVGLDisplay/` | WO1, WO4, WO6, WO7 | Static call fix, boot title, screensaver policy, EventBus race |
| `modules/DataManager/` | WO2, WO8 | TWDT unsubscribe around fetch, IDataSource contract, queueDataJob primitive |
| `modules/WebServer/` | WO3, WO4 | Stack size, sockets, `/api/app` stylesheet |
| `modules/Application/` | WO4 | `setAppStylesheet()` API |
| `modules/EventBus/` | WO5, WO7 | Publisher-side payload lifetime doc (WO5), threading rule doc (WO7) |
| `web/portal/` | WO3, WO4, WO6 | HWM logging, FOUC fix in app.js, screensaver toggle |
| `.agents/skills/framework-expert/` | WO5 | LV_CONF_PATH consumer warning, PSRAM audit checklist |
| `squareline/crowpanel-128/` | WO5 | SLS naming consistency |
| `sample-app/` | WO5 | Native test link fix |

## Session Log

### Session 1 — 2026-08-19 (Triage & Decomposition)
- Reviewed all 15 open GitHub issues.
- Triaged into 11 actionable + 4 parked (#55 roadmap RFC, #103/#104/#105 release-ops chain).
- Recommended #102 (beta OTA) remain open as a standalone investigation.
- Decomposed 11 issues into 6 sequential Work Orders, serialised to avoid file conflicts.
- Prior art: `pr-p2-issue-remediation` project closed #91/#90/#84 in 4 WOs with 271 new tests.
- Pruned 8 stale local branches from graduated projects.
- Working tree clean on `main` at `446a1724`, tag `v0.0.12-dev.52`.
