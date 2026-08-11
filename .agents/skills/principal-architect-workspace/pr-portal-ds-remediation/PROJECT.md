# Project: Portal Design System Remediation

**Project ID:** `pr-portal-ds-remediation`  
**Base Branch:** `main`  
**Origin:** Design system adherence review (session `e11b0271`)  
**Review Artifact:** [design_system_review.md](../../../../.gemini/antigravity/brain/e11b0271-5773-4c0c-9684-fc90e3852d57/design_system_review.md)

---

## Objective

Eliminate **all design system debt** identified in the web portal audit against
the [framework design system](../../framework-design-system/readme.md). The
portal's CSS (`app.css`) is the ground truth the DS was reverse-engineered from,
so the token *values* already match — the debt is in **hard-coded literals in JS
fragments and CSS** that bypass the semantic token layer, plus emoji/icon
violations, incomplete font stacks, and missing ARIA semantics.

**Success criteria:** Every colour, radius, shadow, font-family, and transition
in the portal references a CSS custom property (or a documented CSS class that
does). Zero emoji-as-icons. All Feather SVGs use standard attributes. ARIA tab
semantics on bottom nav.

---

## Decomposition

Four serialised Work Orders. All touch `web/portal/` exclusively — no firmware C++,
no build scripts, no test infrastructure changes. WO1–WO3 are pure `mechanical`
find-and-replace with clear checklists; WO4 is `standard` (requires understanding
the nav/ARIA contract).

### File Overlap Analysis

| File | WO1 | WO2 | WO3 | WO4 |
|---|---|---|---|---|
| `app.css` | ✅ colours | ✅ font-mono, grey | | |
| `wifi.js` | ✅ colours | | | |
| `system.js` | ✅ colours | | | |
| `ota.js` | ✅ colours | | | |
| `auth.js` | ✅ colours | | ✅ icon | |
| `mcp.js` | ✅ colours | | ✅ icon | |
| `credits.js` | ✅ colours | | ✅ icon | |
| `display.js` | ✅ colours | | | |
| `homeassistant.js` | ✅ colours | | ✅ icon | |
| `backup.js` | ✅ colours | | | |
| `keys.js` | ✅ colours | | ✅ emoji+icon | |
| `time.js` | ✅ colours | | | |
| `appconfig.js` | ✅ colours | | | |
| `index.html` | | | | ✅ ARIA |
| `app.js` | | | | ✅ ARIA |
| `utils.js` | | | | |

**Conflict verdict:** WO1 and WO2 both touch `app.css` but in non-overlapping
sections (colours vs font-family + minor grey). **Serialise them** to avoid merge
friction — the cost is negligible since these are fast mechanical WOs.

WO1 and WO3 both touch `auth.js`, `mcp.js`, `credits.js`, `homeassistant.js`,
`keys.js` — WO1 for colour tokens, WO3 for icon SVGs. Different lines, but
serialisation avoids any base-commit drift. **Serialise all four.**

---

## Work Order Ledger

- **[ ] [wo1-colour-token-migration](wo1-colour-token-migration/WORKORDER.md):** Replace all hard-coded hex/rgba status colours and absolute black/white/grey in JS fragments with semantic CSS custom properties. Covers 10 off-palette hex instances (`#ccc`, `#999`, `#6c63ff`, `#ff8f00`, `#43a047`) + 11 hard-coded `#fff`/`#000` inline styles. **21 replacements across 10 files** (`wifi.js`, `system.js`, `ota.js`, `mcp.js`, `credits.js`, `display.js`, `keys.js`, `time.js`, `utils.js`, `app.css`).
- **[ ] [wo2-css-polish](wo2-css-polish/WORKORDER.md):** Expand monospace font stack to match DS `--font-mono` (3 instances in `app.css`). **3 replacements in 1 file.**
- **[ ] [wo3-icon-remediation](wo3-icon-remediation/WORKORDER.md):** Replace 6 emoji-as-icons in `keys.js` with Feather SVGs. Fix 9 non-standard SVG icon sizes across `wifi.js` (2), `auth.js` (2), `mcp.js` (2), `credits.js` (1), `index.html` (1), `utils.js` (1). **15 replacements across 6 files.**
- **[ ] [wo4-aria-tab-semantics](wo4-aria-tab-semantics/WORKORDER.md):** Add `role="tablist"` to bottom nav, `role="tab"` + `aria-selected` to nav items, `role="tabpanel"` + `aria-labelledby` to `#main-container`. Add skip-nav link. Update `app.js` tab-switch logic to maintain `aria-selected`. **~10 changes across 2 files.**

---

## Modules & Components

| Module | Path | Notes |
|---|---|---|
| Portal CSS | `web/portal/app.css` | Ground-truth stylesheet — WO1 + WO2 |
| Portal HTML Shell | `web/portal/index.html` | WO4 only |
| Portal SPA Logic | `web/portal/app.js` | WO4 only |
| Fragment: WiFi | `web/portal/wifi.js` | WO1 + WO3 |
| Fragment: System | `web/portal/system.js` | WO1 |
| Fragment: OTA | `web/portal/ota.js` | WO1 |
| Fragment: Auth | `web/portal/auth.js` | WO1 + WO3 |
| Fragment: MCP | `web/portal/mcp.js` | WO1 + WO3 |
| Fragment: Credits | `web/portal/credits.js` | WO1 + WO2 + WO3 |
| Fragment: Display | `web/portal/display.js` | WO1 |
| Fragment: Home Asst | `web/portal/homeassistant.js` | WO1 + WO3 |
| Fragment: Backup | `web/portal/backup.js` | WO1 |
| Fragment: Keys | `web/portal/keys.js` | WO1 + WO3 |
| Fragment: Time | `web/portal/time.js` | WO1 |
| Fragment: AppConfig | `web/portal/appconfig.js` | WO1 |

---

## Verification Plan

### Automated Tests
- `npm run test` (Playwright portal tests) — must pass after each WO
- Visual inspection in browser at `http://<device-ip>/` — confirm light + dark theme

### Manual Verification
- Grep audit: `grep -rn '#4CAF50\|#f44336\|#ff5252\|#ff6b6b\|#66bb6a\|#ffa726\|#ef5350\|#FFC107\|#6c63ff' web/portal/` should return zero results after WO1
- Emoji audit: `grep -rn '🔒\|🔗\|🗑' web/portal/` should return zero results after WO3
- ARIA audit: `grep -rn 'role="tablist"\|role="tab"\|aria-selected\|role="tabpanel"' web/portal/` should return hits after WO4

---

## Session Log

### Session: e11b0271 — Design System Review & Decomposition
- **Date:** 2026-08-10
- **Scope:** Audited entire web portal (13 JS files + CSS + HTML) against the framework design system skill. Identified ~41 violations across 9 issue categories. Decomposed into 4 serialised WOs.
- **Key decisions:**
  - Serialise all 4 WOs despite limited file overlap — mechanical WOs are fast, merge friction not worth the complexity of parallelism
  - WO1 is the bulk (~40 replacements), WO2–WO4 are small focused changes
  - Classified WO1–WO3 as `mechanical` (clear find-replace lists), WO4 as `standard` (requires ARIA/accessibility reasoning)
  - Deliberately did NOT attempt to refactor `app.css` to import DS token files — the portal is flash-embedded and the current inline token definitions are correct. The DS was reverse-engineered FROM `app.css`, not the other way around.
