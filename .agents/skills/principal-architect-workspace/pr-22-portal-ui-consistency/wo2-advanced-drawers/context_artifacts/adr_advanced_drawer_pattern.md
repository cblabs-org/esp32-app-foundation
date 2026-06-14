# ADR: Advanced Drawer Pattern — Native `<details>`/`<summary>` vs JS Toggle

## Status
Implemented — 2026-06-14

## Context

Two "Advanced" sections existed in the web portal with inconsistent implementation:
- `ota.js` Card 4: yellow inline styles, always expanded
- `time.js`: bespoke JS toggle with `display:none` visibility trick, blue button, no yellow

The goal was a single reusable pattern: yellow warning-zone aesthetic, collapsed by default, zero JS.

## Decision

Use the native HTML `<details>`/`<summary>` element with a CSS-only chevron animation.

**Why native `<details>` over a JS toggle:**
- Zero JS maintenance: no event listeners, no IDs to keep in sync, no state to manage
- Accessibility: browsers handle keyboard interaction and ARIA for `<details>` natively
- Collapse-by-default: simply omit the `open` attribute — no `display:none` hacks
- Animation: `::after` pseudo-element with `transform: rotate()` + `transition` is sufficient for the chevron

**Why `article.advanced-drawer > details > summary` (deep child combinator):**
- The `>` combinator ensures only the *direct* child `<details>` and its *direct* child `<summary>` are styled
- Prevents accidental cascade if nested `<details>` are ever added inside the drawer content
- `list-style: none` + `::-webkit-details-marker { display: none }` are both required — the standard property alone doesn't remove the marker in all WebKit versions

**Why `article.advanced-drawer` as the outer wrapper (not just `details.advanced-drawer`):**
- Consistent with the portal's existing card structure: all cards are `<article>` elements
- The `border-color` and `background` tint need to apply to the card outline, which is styled at the `<article>` level in `app.css` base styles
- Keeps separation of concerns: article = card shell, details = collapse mechanism

## Consequences

- Any future "Advanced" section must use `<article class="advanced-drawer"><details><summary class="advanced-summary">Advanced</summary>...</details></article>`
- Playwright tests that interact with elements inside Advanced drawers must call `el.open = true` before clicking — use `#tab-[name] article.advanced-drawer details` as the locator scope to avoid strict-mode violations when multiple tabs are in the DOM
- No `time.spec.ts` exists; if one is added later, it will need the same pattern for tests touching posix-override, NTP, or timezone DB elements
