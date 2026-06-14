---
title: WO2 — Advanced Sections as Collapsible Yellow Drawers
distilled_at: 2026-06-14T10:17:00Z
original_plan_id: pr-22-portal-ui-consistency
artifacts:
  - context_artifacts/adr_advanced_drawer_pattern.md
  - context_artifacts/graveyard_test_locator_failures.md
---

# WO2 — Advanced Drawer Distillation

## Executive Summary

WO2 is **complete and PA-approved**. All Advanced sections in the web portal now use the reusable `article.advanced-drawer > details > summary.advanced-summary` pattern — yellow border, collapsed by default, zero JS. Branch `pr-22-portal-ui-consistency/wo2-advanced-drawers` has 2 commits on top of WO1 and passes 61/61 Playwright tests.

## Commits Delivered

| SHA | Message |
|-----|---------|
| `4cbf31b` | `feat(portal): standardise Advanced sections as collapsible yellow drawers` |
| `17dfaea` | `test(ota): open Advanced drawer before interacting with hidden elements` |

## Files Changed

| File | Change |
|------|--------|
| `web/portal/app.css` | Added section `9c. ADVANCED DRAWER COMPONENT` after 9b |
| `web/portal/ota.js` | Card 4: inline style + `<header>` → `<article class="advanced-drawer"><details>` |
| `web/portal/time.js` | Advanced article migrated; JS toggle handler removed entirely |
| `test/web/tests/ota.spec.ts` | 2 tests: `el.open = true` added before interacting with drawer contents |

## Next Steps (for PA / WO3+)

- Branch is ready to merge into `pr-22-portal-ui-consistency` or rebase as needed
- No `time.spec.ts` exists — if one is created in future, any tests touching NTP/POSIX-override/TZ-DB elements must open the drawer first using `#tab-time article.advanced-drawer details`
- The CSS class `.advanced-drawer` is now the canonical pattern for any future danger-zone cards

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_advanced_drawer_pattern.md` — Why native `<details>` was chosen, child combinator rationale, future usage contract
- `context_artifacts/graveyard_test_locator_failures.md` — Strict-mode locator violation, file write permission workaround, double-checkout workaround
