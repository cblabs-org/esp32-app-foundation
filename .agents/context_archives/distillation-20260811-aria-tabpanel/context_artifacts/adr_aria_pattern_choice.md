# ADR: Multi-Panel vs Single-Wrapper Tabpanel Pattern

## Context

PR #87 (Portal Design System Remediation) added `role="tablist"`, `role="tab"`, and `aria-selected` to the bottom navigation bar but left the tab content panels without corresponding ARIA semantics. The outer `#tab-content-area` wrapper had a partial single-wrapper pattern (`role="tabpanel"` with dynamic `aria-labelledby` updated on tab switch).

## Decision

**Chose the standard multi-panel pattern** over keeping/completing the single-wrapper pattern.

## Rationale

1. **Standard compliance**: The [WAI-ARIA Tabs Pattern](https://www.w3.org/WAI/ARIA/apg/patterns/tabs/) canonical examples all use multi-panel (one `tabpanel` per tab trigger).
2. **Nested tabs**: The portal has sub-pill tabs within parent tabs. A single-wrapper pattern makes nested tabs nearly impossible to express correctly — you'd need the wrapper to sometimes be a tabpanel for the bottom nav and sometimes for the pill bar.
3. **Natural 1:1 mapping**: The codebase already had consistent naming (`nav-wifi` → `tab-wifi`, `pill-settings` → `tab-settings`) making the `aria-controls`/`aria-labelledby` wiring mechanical.
4. **Simpler JS**: Removing the dynamic `aria-labelledby` update in `activateTopTab()` simplified the code — each panel statically labels itself.

## Consequences

- Every new tab content div (static or dynamic) must include `role="tabpanel" tabindex="0" aria-labelledby="..."` 
- Every new pill button must have a predictable `id="pill-{name}"` for the back-reference
- The `framework-expert/systems/web-portal/overview.md` skill doc was updated to reflect this contract
