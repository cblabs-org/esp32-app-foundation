---
name: pr-22-portal-ui-consistency-wo2
description: Handoff instructions for Work Order 2 — Standardise Advanced Sections as Collapsible Yellow Drawers
complexity: standard
---

# pr-22-portal-ui-consistency / WO2 — Advanced Sections as Collapsible Yellow Drawers

You are an expert senior software engineer and execution-focused worker agent.
**DO NOT trigger or act as the principal-architect.**
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

---

## Objective

Standardise all "Advanced" sections across the web portal to use a single reusable
CSS drawer pattern: **yellow (`--warning`) border + header, collapsed by default**.
This is a visual signal that these are danger-zone / expert controls.

The OTA Advanced card (`ota.js`) is the current reference for yellow styling but is
**not** collapsed by default. The Time tab Advanced section uses a bespoke JS toggle
with no yellow styling. There are no other Advanced sections found in `system.js`,
`wifi.js`, or `appconfig.js`.

---

## Strict Boundary Box

- **ONLY touch:** `web/portal/app.css`, `web/portal/ota.js`, `web/portal/time.js`
- **DO NOT touch:** `appconfig.js`, `system.js`, `wifi.js`, `backup.js`, `keys.js`,
  `mcp.js` — audit found no Advanced sections in these files
- **DO NOT touch:** Any firmware source files (.cpp, .hpp)
- **DO NOT add new Advanced sections** — migrate existing ones only
- Assume `app.css` now has section 9b (toggle switch CSS) from WO1 merge

---

## Audit Results (Pre-Researched by Principal Architect)

Advanced sections found:

| File | Current style | Current behaviour |
|------|---------------|-------------------|
| `ota.js` (Card 4, ~line 93-95) | Yellow border (`--warning`) on `<article>`, yellow header | **Expanded by default** (no fold) |
| `time.js` (~line 39-91) | Blue button (`--primary`), bespoke JS toggle | **Collapsed** but no yellow styling |

No Advanced sections were found in `system.js`, `wifi.js`, `appconfig.js`, `backup.js`,
`keys.js`, or `mcp.js`.

---

## The Target Pattern

Every Advanced section must become an `<article>` with:
- `class="advanced-drawer"` — applies warning border + tinted background
- An inner `<details>` element for native collapse/expand (no JS required)
- `<summary class="advanced-summary">Advanced</summary>` — styled yellow header
- The section content wrapped inside `<details>` after the summary
- **Collapsed by default** — `<details>` without the `open` attribute

---

## Checklist

### A — Define `.advanced-drawer` CSS component in `app.css`

Add a new section after section 9b (toggle switch) titled `9c. ADVANCED DRAWER COMPONENT`:

```css
/* ==========================================================================
   9c. ADVANCED DRAWER COMPONENT
   Reusable yellow collapsible card for advanced/expert controls.
   Collapsed by default; user must explicitly open.
   ========================================================================== */

article.advanced-drawer {
  border-color: var(--warning, #f9a825);
  background: rgba(249, 168, 37, 0.04);
}

article.advanced-drawer > details > summary.advanced-summary {
  font-weight: 700;
  font-size: 1.1rem;
  color: var(--warning, #f9a825);
  cursor: pointer;
  list-style: none;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.25rem 0 0.75rem;
  user-select: none;
}

article.advanced-drawer > details > summary.advanced-summary::after {
  content: '▶';
  font-size: 0.7rem;
  transition: transform 0.2s;
}

article.advanced-drawer > details[open] > summary.advanced-summary::after {
  transform: rotate(90deg);
}

/* Remove default marker on all browsers */
article.advanced-drawer > details > summary.advanced-summary::-webkit-details-marker {
  display: none;
}
```

### B — Migrate OTA Advanced card (`ota.js`)

Locate Card 4 (~line 93-95). Currently:
```html
<article style="border-color: var(--warning, #f9a825); background: rgba(249, 168, 37, 0.04);">
  <header style="color: var(--warning, #f9a825);">Advanced</header>
  <!-- content -->
</article>
```

Replace with:
```html
<article class="advanced-drawer">
  <details>
    <summary class="advanced-summary">Advanced</summary>
    <!-- all existing content unchanged -->
  </details>
</article>
```

- Remove the inline `style` from `<article>` (now handled by CSS class)
- Remove the `<header>` element — replaced by `<summary>`
- Wrap all content that was inside the `<article>` (after the old header) inside the
  `<details>` after the `<summary>`
- The `<details>` has NO `open` attribute → collapsed by default

### C — Migrate Time tab Advanced section (`time.js`)

Locate the Advanced article (~line 37-92 of `time.js`). Currently:
```html
<article>
  <header style="display:flex; justify-content:space-between; align-items:center;">
    <span>Advanced</span>
    <button id="btn-time-advanced-toggle" ...>▶ Show</button>
  </header>
  <div id="time-advanced-section" style="display:none;">
    <!-- NTP servers, POSIX override, Timezone DB -->
  </div>
</article>
```

Replace with:
```html
<article class="advanced-drawer">
  <details>
    <summary class="advanced-summary">Advanced</summary>
    <!-- Move ALL existing content from time-advanced-section div here -->
    <!-- NTP servers, POSIX override, Timezone DB -->
  </details>
</article>
```

Then remove the JS toggle wiring (~lines 83-91):
```js
// Remove this entire block:
const advancedToggle = document.getElementById('btn-time-advanced-toggle');
const advancedSection = document.getElementById('time-advanced-section');
advancedToggle.addEventListener('click', () => {
  ...
});
```

The `<details>`/`<summary>` native browser toggle replaces this JS entirely.

### D — Verify

Run the full Playwright test suite:
```bash
cd test/web && npx playwright test
```

All tests must pass. Pay particular attention to:
- `tests/ota.spec.ts` — the OTA advanced card is now inside `<details>`, check if any
  test clicks inside it. If tests assume the content is always visible and now fail
  because `<details>` is collapsed, add a `page.locator('details').click()` or
  `details.evaluate(el => el.open = true)` step in those tests.
- `tests/time.spec.ts` — similar check.

If any Playwright tests break because content is now hidden inside closed `<details>`,
fix those tests by opening the `<details>` before interacting. This is a valid and
expected test update — do NOT revert the drawer pattern to fix tests.

---

## Acceptance Criteria

- [ ] `.advanced-drawer` CSS defined in `app.css` (section 9c)
- [ ] OTA Advanced card uses `<article class="advanced-drawer"><details>...` — collapsed by default
- [ ] Time Advanced section uses `<article class="advanced-drawer"><details>...` — collapsed by default
- [ ] Bespoke JS toggle in `time.js` removed
- [ ] No inline `border-color: var(--warning)` or `color: var(--warning)` on Advanced elements (all via CSS class)
- [ ] `npx playwright test` — all pass (including any test fixes for newly-collapsed content)

---

## On Completion

**STOP and send a message back to the Principal Architect** reporting:
1. Test results (pass/fail counts for full suite)
2. Which Playwright tests (if any) needed updates to accommodate `<details>` collapse
3. Any unexpected findings

Do NOT wrap up, distill, or commit until the PA explicitly approves. Once approved,
invoke the `@distillery` skill passing
`.agents/plugins/principal-architect-workspace/pr-22-portal-ui-consistency/wo2-advanced-drawers/`
as the target directory. Append a status update to this WORKORDER.md file, then
terminate your session.

---

## ✅ Status Update — COMPLETED 2026-06-14

**Executed by:** Execution Worker (session `70a30087-2f15-43ba-971c-ee2d7a3e5819`)
**PA Approval:** Received 2026-06-14T10:16:05Z

### Acceptance Criteria — Final State

- [x] `.advanced-drawer` CSS defined in `app.css` (section 9c)
- [x] OTA Advanced card uses `<article class="advanced-drawer"><details>...` — collapsed by default
- [x] Time Advanced section uses `<article class="advanced-drawer"><details>...` — collapsed by default
- [x] Bespoke JS toggle in `time.js` removed
- [x] No inline `border-color: var(--warning)` or `color: var(--warning)` on Advanced elements (all via CSS class)
- [x] `npx playwright test` — **61/61 passed** (including 2 test fixes for newly-collapsed content)

### Test Fixes Required

`test/web/tests/ota.spec.ts` — 2 tests updated:
1. `manual upload — requires .bin + .sig, FormData POST to /api/ota/upload`
2. `rollback button requires ROLLBACK confirmation prompt`

Fix: `await page.locator('#tab-ota article.advanced-drawer details').evaluate((el) => { el.open = true; })` added after `bootAndNavigateToOta(page)` in each test. Scoped to `#tab-ota` because all tabs render into DOM simultaneously and both OTA and Time now have `.advanced-drawer` elements.

### Commits

| SHA | Message |
|-----|---------|
| `4cbf31b` | `feat(portal): standardise Advanced sections as collapsible yellow drawers` |
| `17dfaea` | `test(ota): open Advanced drawer before interacting with hidden elements` |

### Distillery Output

Packaged to: `.agents/plugins/principal-architect-workspace/pr-22-portal-ui-consistency/wo2-advanced-drawers/`
- `context_bridge.md`
- `sessions.md`
- `context_artifacts/adr_advanced_drawer_pattern.md`
- `context_artifacts/graveyard_test_locator_failures.md`
