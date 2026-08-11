# Project: aria-tabpanel

**Objective:** Resolve GitHub issue #92 — add complete WAI-ARIA tabpanel semantics to the web portal at both top-level (bottom nav) and sub-pill (pill bar) levels. Migrate from the incomplete single-wrapper `tabpanel` pattern to the standard multi-panel pattern.

**Base branch:** `main`
**PR target:** `main`
**Issue:** [#92](https://github.com/matthewmcneill/esp32-app-foundation/issues/92)

## Work Order Ledger

- **[x] [wo1-portal-semantics](wo1-portal-semantics/WORKORDER.md):** Add `role="tabpanel"` / `aria-labelledby` to all static and dynamic tab content divs, add ARIA attributes to pill buttons, clean up the wrapper-level pattern, and update `aria-controls` on all tab triggers.

## Modules & Components

| Module | Files | Notes |
|---|---|---|
| web-portal | `web/portal/index.html`, `web/portal/app.js` | Static HTML tab panels + dynamic JS injection logic |

## Session Log

### 2026-08-11 — Project Creation
- Issue #92 analysed: the outer `#tab-content-area` wrapper has `role="tabpanel"` and dynamic `aria-labelledby` (single-wrapper pattern), but inner tab content divs and sub-pill buttons have no ARIA semantics.
- Decision: migrate to standard multi-panel pattern (each inner div gets its own `role="tabpanel"`).
- Expanded scope to include sub-pill ARIA gap (pill buttons had zero ARIA attributes).
- Implementation plan approved by user.
- **User approval of decomposition: ✅ Approved 2026-08-11T14:15:00+01:00**
