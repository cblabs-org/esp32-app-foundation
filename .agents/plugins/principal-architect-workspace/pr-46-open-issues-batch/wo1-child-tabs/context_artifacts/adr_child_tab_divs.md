# ADR: Child Tab Content Div Creation in buildBottomNav()

## Context

The web portal's tab system has two tiers:
1. **Top-level tabs** — rendered as bottom nav icons (WiFi, System, app-contributed tabs)
2. **Child tabs (sub-pills)** — rendered as pill buttons within a parent tab's sub-pill bar

`buildBottomNav()` creates `<div class="tab-content">` containers for top-level tabs (so `showContentDiv()` can find them by ID). However, it did **not** create containers for child tabs. When a user clicked a sub-pill, `activateSubPill()` → `showContentDiv(childId)` → `document.getElementById(childId)` returned `null`, and the content pane was blank.

## Decision

Add a nested loop in `buildBottomNav()` that iterates over the `childTabs` map and creates content divs for every child tab, using the same ID derivation pattern:

```
'tab-' + child.name.toLowerCase().replace(/\s+/g, '-')
```

This pattern is used consistently in:
- `buildPillBar()` (line 352) — sets `data-target` on pill buttons
- `activateTopTab()` (line 312) — computes `firstChildId` fallback
- `activateSubPill()` (line 375) — receives the ID and passes to `showContentDiv()`

### Why in `buildBottomNav()` and not elsewhere?

- `buildBottomNav()` already owns the responsibility of creating content divs for dynamically-contributed tabs
- It runs once after `loadTabs()` populates both `topLevelTabs` and `childTabs`
- The `if (!document.getElementById(childId))` guard makes it idempotent

### Why not modify `activateSubPill()` or `showContentDiv()`?

- The WORKORDER's boundary box explicitly prohibits modifying those functions
- Architecturally, div creation is a setup concern (belongs in `buildBottomNav()`), not an activation concern

## Placement

The new loop is inserted:
- **After** the top-level tab loop (line 270)
- **Before** the click-handler binding block (line 288)
- **Within** `buildBottomNav()`, before its closing brace

## Consequences

- All child tabs now have content divs ready when fragments inject HTML
- No regression risk — the guard (`!document.getElementById`) prevents duplicates
- 69/69 Playwright tests pass unchanged
