# ADR: BEM Naming Convention & Migration Rules

## Context
`web/portal/app.css` had 76 CSS classes split across 3+ naming conventions. To avoid introducing a 4th style when adding RFC #67 design system components, WO1 adopted BEM (`block--modifier`) for variant states and migrated all legacy hyphen-suffix and compound state classes.

## Decisions Made
1. **Modifier Syntax:** All state variants and color modifiers use double-hyphens `--` (e.g., `.badge--ok`, `.toast--success`, `.dot--green`, `.tab-content--active`).
2. **Single-class state modifiers for `.dot`:** Changed compound rules like `.dot.green` to `.dot--green` while retaining `.dot` as base block styling (`<span class="dot dot--green">`).
3. **Selector Updates:** Updated all dynamic JS `classList` and `className` assignments, HTML elements, and Playwright test assertions (`test/web/tests/`).

## Verification
- 0 legacy class instances remaining in `web/portal/` and `test/web/`.
- 70/70 Playwright integration tests passing.
- PlatformIO firmware build succeeded.
