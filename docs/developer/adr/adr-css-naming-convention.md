# ADR: CSS Naming Convention — BEM Adoption

## Status

Accepted

## Context

The web portal stylesheet (`web/portal/app.css`) contained 76 CSS classes using inconsistent naming patterns:
1. **Hyphen-suffix variants:** `.badge-ok`, `.toast-success`, `.rag-green`, `.btn-delete`
2. **Compound/state classes:** `.dot.green`, `.tab-content.active`, `.pill-btn.active`, `.login-overlay.hidden`
3. **Prefixed components:** `.status-badge`, `.info-row`, `.compact-row`
4. **Element selectors:** `article`, `dialog`, `button` (base element resets)
5. **Standalone block names:** `.badge`, `.dot`, `.toast`, `.container`

RFC #67 proposes adding 18 new classes to establish a design system component library. Without a single, explicit CSS naming convention, new components would introduce further inconsistency and collision risks.

## Decision

We adopt **BEM (Block Element Modifier)** with the `block--modifier` convention for all CSS classes in the web portal design system going forward:

1. **Modifiers:** Use double-hyphens (`block--modifier` or `block__element--modifier`) to indicate variant states or types (e.g., `.badge--ok`, `.toast--success`, `.rag--green`, `.tab-content--active`, `.login-overlay--hidden`).
2. **Elements & Blocks:** Standard block names (`.status-badge`, `.info-row`, `.compact-row`) and element reset selectors (`article`, `dialog`, `button`) are exempt from modifier renaming as they represent standard BEM blocks or base HTML element resets.
3. **Legacy Migration:** All ~24 existing non-conforming classes (hyphen-suffix variants and compound state classes) are migrated to BEM modifier syntax in a single breaking migration (WO1), updating all associated CSS definitions, HTML templates, JS dynamic class mutations, documentation, and Playwright tests.

## Consequences

- **Renamed Classes:** 23 non-conforming legacy CSS classes across `app.css` have been refactored to BEM modifier syntax (`--`).
- **Single-class modifiers for state:** Compound selectors such as `.dot.green` become single BEM modifier classes `.dot--green` applied alongside base block classes (`<span class="dot dot--green">`).
- **Updates Required:** All JavaScript files (`app.js`, `utils.js`, `auth.js`, `system.js`, `wifi.js`, `display.js`, `screenshot.js`), HTML markup (`index.html`), developer documentation, and Playwright integration tests updated.
- **Benefits:** Eliminates class collision risks, establishes clear conventions for future RFC #67 design system components, and ensures consistent state manipulation via BEM modifiers across all web portal modules.
