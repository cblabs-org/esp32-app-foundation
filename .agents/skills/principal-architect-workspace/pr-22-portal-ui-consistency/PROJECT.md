# PR-22 — Web Portal UI Consistency

**Status:** 📋 PLANNING — 2 WOs defined, awaiting user approval
**Base branch:** `main`
**Feature branch:** `pr-22-portal-ui-consistency`
**Session:** 5177dae5-bdb0-409e-9692-efe1b983e4dd

---

## Summary

Two UI consistency issues in the web portal, grouped into one PR:

1. **Screensaver toggle switches always grey** — CSS specificity bug; inline `style` overrides
   dynamic class rule. Fix: move `.toggle-switch`/`.toggle-slider` CSS to `app.css`, strip
   inline background from slider spans, remove dynamic `<style>` injection.

2. **Advanced sections inconsistently styled** — OTA has yellow style (no fold); Time has
   fold (no yellow, blue button). Fix: new `.advanced-drawer` CSS component (`<details>`-based),
   applied to both.

---

## Work Order Ledger

- **[ ] [wo1-screensaver-toggle](wo1-screensaver-toggle/WORKORDER.md):** Fix toggle CSS
  (move to app.css, strip inline overrides) — `display.spec.ts` + full suite
- **[ ] [wo2-advanced-drawers](wo2-advanced-drawers/WORKORDER.md):** Advanced drawer
  CSS component, migrate OTA + Time — full suite

---

## Branch Strategy

```
main
 └── pr-22-portal-ui-consistency          (feature branch)
       ├── pr-22-portal-ui-consistency/wo1-screensaver-toggle
       └── pr-22-portal-ui-consistency/wo2-advanced-drawers
             (bases off wo1, not main)
```

---

## Stash Note

`stash@{0}` ("remaining-unstaged", `34f07b6`) contains firmware changes
(`ScreensaverBoard.cpp/.hpp`, `U8G2DisplayModule.cpp`). WO1 worker must
audit whether these are already in `main` and report back before any stash action.

Current finding (PA pre-check): `setClockFont` IS present in the working tree
`ScreensaverBoard.hpp` (line 91) — indicating the stash content may already be
incorporated. Worker must verify and report.

---

## Files In Scope

| File | WO1 | WO2 |
|------|-----|-----|
| `web/portal/app.css` | ✅ Add toggle CSS | ✅ Add advanced drawer CSS |
| `web/portal/appconfig.js` | ✅ Fix slider HTML, remove injection | — |
| `web/portal/ota.js` | — | ✅ Migrate Advanced card |
| `web/portal/time.js` | — | ✅ Migrate Advanced section |

---

## Verification

- WO1: `npx playwright test tests/display.spec.ts` + full suite
- WO2: `npx playwright test` (full suite, may include test updates)
