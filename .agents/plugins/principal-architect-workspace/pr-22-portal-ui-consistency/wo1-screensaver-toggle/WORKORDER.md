---
name: pr-22-portal-ui-consistency-wo1
description: Handoff instructions for Work Order 1 — Fix screensaver toggle switches
complexity: standard
---

# pr-22-portal-ui-consistency / WO1 — Screensaver Toggle Switches

You are an expert senior software engineer and execution-focused worker agent.
**DO NOT trigger or act as the principal-architect.**
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

---

## Objective

Fix the screensaver (and flip-screen) toggle switches in the web portal Display tab
so they correctly render as blue/on when their firmware state is `true`, and grey
when `false`. Currently both toggles appear permanently grey because the CSS
that drives `.toggle-slider` colour-change on `:checked` state is missing from
`app.css` — the only CSS for them is a dynamic `<style>` injection in `appconfig.js`
whose rules are **overridden** by inline `style` attributes on the slider spans.

---

## Strict Boundary Box

- **ONLY touch:** `web/portal/app.css`, `web/portal/appconfig.js`
- **DO NOT touch:** Any JS files other than `appconfig.js`
- **DO NOT touch:** Any firmware source files (.cpp, .hpp)
- **DO NOT touch:** `ota.js`, `time.js`, `system.js`, `wifi.js` — those are WO2 scope
- **DO NOT apply or drop the stash yourself** — report the finding and await instruction

---

## Pre-Flight: Stash Audit

**Step 1 — Read-only stash audit (NO code changes here)**

Run: `grep -n "setClockFont" modules/U8G2Display/boards/ScreensaverBoard.hpp`

- If `setClockFont` IS found → stash is already incorporated into main.
  Report this to the PA.
- If NOT found → stash is not incorporated.
  Report this to the PA and await instruction before applying.

Do NOT `git stash apply` or `git stash drop` without explicit user approval.

---

## Root Cause (Pre-Diagnosed)

The toggle HTML in `appconfig.js` renders as:
```html
<span class="toggle-slider" style="background:var(--border, #dee2e6); ..."></span>
```

The inline `style="background:..."` has higher specificity than the dynamically-injected
CSS class rule `.toggle-switch input:checked + .toggle-slider { background: var(--primary) }`.
Inline styles always win → slider is always grey regardless of checked state.

Additionally, `app.css` has zero `.toggle-switch`/`.toggle-slider` rules — the
dynamic injection in `appconfig.js` is the only CSS for these components, making it
fragile and ordering-dependent.

---

## Checklist

### A — Add toggle CSS to `app.css`

Insert a new section after section 9 (Drawer Component, ending ~line 421):

```css
/* ==========================================================================
   9b. TOGGLE SWITCH COMPONENT
   ========================================================================== */

.toggle-switch {
  position: relative;
  display: inline-block;
  width: 48px;
  height: 26px;
  flex-shrink: 0;
}

.toggle-switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.toggle-slider {
  position: absolute;
  cursor: pointer;
  top: 0; left: 0; right: 0; bottom: 0;
  background: var(--border, #dee2e6);
  border-radius: 26px;
  transition: background 0.3s;
}

.toggle-slider::before {
  content: "";
  position: absolute;
  height: 20px;
  width: 20px;
  left: 3px;
  bottom: 3px;
  background: white;
  border-radius: 50%;
  transition: transform 0.3s;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
}

.toggle-switch input:checked + .toggle-slider {
  background: var(--primary, #2196F3);
}

.toggle-switch input:checked + .toggle-slider::before {
  transform: translateX(22px);
}
```

### B — Strip inline `background` from `.toggle-slider` spans in `appconfig.js`

Two locations have inline `style` on `<span class="toggle-slider" ...>` that include
`background:var(--border, #dee2e6)`. Remove ONLY that `background` property from the
inline style string. Keep position/layout styles.

1. Flip toggle slider span (~line 54–56)
2. Screensaver enable toggle slider span (~line 83–85)

After fix, those spans should have:
`style="position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; border-radius:26px; transition:0.3s;"`

### C — Remove the dynamic `<style>` injection block in `appconfig.js`

Remove the block at ~lines 155–173:
```js
if (!document.getElementById('display-toggle-styles')) {
  var style = document.createElement('style');
  style.id = 'display-toggle-styles';
  style.textContent = '...' // the toggle CSS rules
  document.head.appendChild(style);
}
```

The toggle CSS now lives in `app.css`. This block is redundant.

### D — Preserve the font-group style

The injected block also contains a rule for `#display-font-group label:has(input:checked)`.
Move this rule into `app.css` in the same new section 9b, or immediately after. Do NOT
drop this rule — the clock font size selector depends on it.

### E — Verify

```bash
cd test/web && npx playwright test tests/display.spec.ts
```
All display tests must pass.

```bash
cd test/web && npx playwright test
```
No regressions in the full suite.

---

## Acceptance Criteria

- [ ] Both flip and screensaver-enable toggles visually reflect their checked state
- [ ] No inline `background` override on `.toggle-slider` spans in `appconfig.js`
- [ ] Dynamic `<style>` injection removed from `appconfig.js`
- [ ] Canonical toggle CSS lives in `app.css` (section 9b)
- [ ] `#display-font-group label:has(input:checked)` rule preserved in `app.css`
- [ ] `npx playwright test tests/display.spec.ts` — all pass
- [ ] `npx playwright test` — no new failures

---

## On Completion

**STOP and send a message back to the Principal Architect** reporting:
1. Stash audit result (is `setClockFont` already in main?)
2. Test results (pass/fail counts for display suite + full suite)
3. Any unexpected findings

Do NOT wrap up, distill, or commit until the PA explicitly approves. Once approved,
invoke the `@distillery` skill passing
`.agents/plugins/principal-architect-workspace/pr-22-portal-ui-consistency/wo1-screensaver-toggle/`
as the target directory. Append a status update to this WORKORDER.md file, then
terminate your session.

---

## Status Update — 2026-06-14T10:09:30Z

**Status: COMPLETE ✅**

**Executed by:** Worker agent `ce65b937-77a7-4ea7-9e40-a74ed3aa9dc4`
**Approved by:** Principal Architect `5177dae5-bdb0-409e-9692-efe1b983e4dd`
**Branch:** `pr-22-portal-ui-consistency/wo1-screensaver-toggle`
**Commit:** `f276ddd`

### Acceptance Criteria

- [x] Both flip and screensaver-enable toggles visually reflect their checked state
- [x] No inline `background` override on `.toggle-slider` spans in `appconfig.js`
- [x] Dynamic `<style>` injection removed from `appconfig.js`
- [x] Canonical toggle CSS lives in `app.css` (section 9b)
- [x] `#display-font-group label:has(input:checked)` rule preserved in `app.css`
- [x] `npx playwright test tests/display.spec.ts` — **13/13 passed**
- [x] `npx playwright test` — **61/61 passed, 0 regressions**

### Stash Audit

`setClockFont` found at line 91 of `modules/U8G2Display/boards/ScreensaverBoard.hpp` — stash is already incorporated into `main`. No stash apply was performed.

### Distillery Output

Context packaged in:
- `context_bridge.md`
- `sessions.md`
- `context_artifacts/adr_toggle_css_strategy.md`
- `context_artifacts/adr_dynamic_style_removal.md`
