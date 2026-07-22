# PR-46: Open Issues Batch — 4 Consumer-Reported Defects

**Parent Program:** esp32-app-foundation  
**Base Branch:** `main`  
**Stack Tip:** `open-issues-batch/wo4-arduinojson-docs` (when complete)  
**GitHub PR:** TBD — will be submitted as a stack

## Objective

Fix all 4 open P2-correctness issues reported by the power-monitor consumer program:
- **#37** — Sub-pill (child) tabs render blank in the web portal
- **#31** — `apiFetch` discards JSON error body on non-2xx responses
- **#36** — `BOARD_HAS_PSRAM` without `CONFIG_SPIRAM` gives no warning; PSRAM silently disabled
- **#29** — ADR-15 mandates `StaticJsonDocument`, removed in ArduinoJson 7

## Status Log

| Date | Event |
|------|-------|
| 2026-07-22 | PA investigated all 4 open issues, validated root causes against source code |
| 2026-07-22 | User approved decomposition and resolved all open questions |
| 2026-07-22 | Execution started — WO1 spawned |

## Work Order Ledger

- **[x] [wo1-child-tabs](wo1-child-tabs/WORKORDER.md):** Issue #37 — Create content divs for child (sub-pill) tabs in `buildBottomNav()` so they render instead of showing blank — `1e9f855`
- **[x] [wo2-apifetch-error-body](wo2-apifetch-error-body/WORKORDER.md):** Issue #31 — Preserve structured JSON error body on non-2xx responses in `apiFetch` + fix doc inaccuracies — `079f5c6`
- **[x] [wo3-psram-guard](wo3-psram-guard/WORKORDER.md):** Issue #36 — Add compile-time + runtime PSRAM guards, remove inert S3 flag, update docs — `73b5b7f`
- **[x] [wo4-arduinojson-docs](wo4-arduinojson-docs/WORKORDER.md):** Issue #29 — Replace all `StaticJsonDocument`/`DynamicJsonDocument` references across 14+ doc files with ArduinoJson 7 guidance — `6ece967`

## Dependency Graph

```
WO1 (child tabs) → WO2 (apiFetch error body) → WO3 (PSRAM guard) → WO4 (ArduinoJson docs)
```

WO1 and WO2 both touch `app.js` — must serialize. WO3 is C++/build. WO4 is docs-only.
