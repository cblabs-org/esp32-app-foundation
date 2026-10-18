# ADR: Credits Tab Design Decisions

## Decision 1: Hardcoded HTML Tab vs IWebContributor

**Choice:** Hardcoded the Credits tab directly in `index.html` rather than implementing it as an `IWebContributor`.

**Why:** Credits is a framework-owned static tab — it has no server-side logic, no API endpoint, and no module ownership. Using `IWebContributor` would have required creating a needless module class just to register a tab name. The existing pattern for framework tabs (WiFi, System, Security) is to hardcode them in HTML with SVG icons, and Credits follows this same convention.

**Implication:** The `buildBottomNav()` skip list in `app.js` had to be extended to include `tab-credits` to prevent double-rendering if `/api/tabs` ever returns a "Credits" entry.

## Decision 2: Skip `/api/tabs` Registration

**Choice:** Did NOT register Credits in the `/api/tabs` response.

**Why:** The work order initially included a Step 6 to add a manual tab injection (like the Status sub-pill), but the self-correction note at the bottom clarified that since Credits is hardcoded in HTML — like WiFi, System, and Security — it doesn't need an `/api/tabs` entry. `app.js` only uses `/api/tabs` to dynamically build nav links for tabs that aren't already in the HTML.

## Decision 3: SVG Icon — Feather-style Copyright Symbol

**Choice:** Used a circle + arc path (`M14.83 14.83a4 4 0 1 1 0-5.66`) to create a © icon in the Feather stroke style.

**Why:** All existing bottom-nav icons use the Feather icon convention: 24×24 viewBox, stroke-only with `stroke-width="2"`, `stroke-linecap="round"`, `stroke-linejoin="round"`, no fill. The copyright circle-with-C pattern maintains visual consistency.

## Decision 4: IIFE Pattern for credits.js

**Choice:** Used the standard IIFE (Immediately Invoked Function Expression) pattern consistent with all other portal fragment scripts.

**Why:** Every other tab fragment (wifi.js, system.js, auth.js, etc.) uses this pattern: wrap everything in `(function() { ... })();`, find the container via `document.getElementById()`, and set `innerHTML`. Deviating would have been inconsistent and potentially caused scoping issues.

## Decision 5: Inline Styles in credits.js

**Choice:** Used inline styles for the library entry rows rather than adding new CSS classes to `app.css`.

**Why:** The work order boundary box explicitly forbids modifying `app.css`. The existing `.card` and `.card-title` classes handle the outer container styling. Row-level layout uses simple flexbox via inline styles, referencing CSS custom properties (`var(--card-border)`, `var(--accent)`) for theme consistency.
