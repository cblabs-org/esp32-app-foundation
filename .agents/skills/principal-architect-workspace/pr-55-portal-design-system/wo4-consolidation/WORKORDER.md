---
name: pr-55-wo4
description: Handoff instructions for Work Order 4 — Card grid consolidation, dot/badge modifiers, list-item reconciliation, signal bars promotion
complexity: standard
---

# WO4 — Consolidation + Signal Bars

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add the design-sensitive CSS components that required consolidation decisions (RFC #67 items #2–7, #11, #15+U2). Promote the signal bars SVG renderer from wifi.js to shared utils.

## Parent Project

`pr-55-portal-design-system` — RFC #67 implementation. This is WO4 of 4 (final).
Depends on: WO3 (core primitives landed).

## Strict Boundary Box

- **IN SCOPE:** `web/portal/app.css`, `web/portal/utils.js`, `web/portal/wifi.js`, `docs/developer/extending-the-web-portal.md`, `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`
- **OUT OF SCOPE:** C++ source, interfaces, build config, `app.js` (ApiQueue was WO2).

## Tooling Constraints

- CSS/JS only — no firmware build required.

## Line-Item Instructions

### Part A: Card Grid System (RFC #2–5, #13)

These 5 RFC items are consolidated into a single card modifier + grid system (per reviewer consensus):

- [x] Add `.card--interactive` modifier to `app.css`:
  - `cursor: pointer; transition: transform 0.15s ease, box-shadow 0.15s ease;`
  - On hover: `transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.1);`
  - Dark theme hover: adjust shadow alpha for dark backgrounds
- [x] Add `.card--empty` modifier:
  - `border: 2px dashed var(--border); background: transparent;`
  - `opacity: 0.6; text-align: center;`
  - `cursor: pointer; transition: opacity 0.15s, border-color 0.15s;`
  - On hover: `opacity: 1; border-color: var(--primary);`
- [x] Add `.card-grid` class:
  - `display: grid; gap: 1rem;`
  - Default: `grid-template-columns: 1fr;` (single column on mobile)
- [x] Add `.card-grid--2col` modifier:
  - `grid-template-columns: repeat(2, 1fr);`
  - Media query for narrow screens (< 400px): fall back to single column
- [x] A11y: document that interactive cards should use `role="button"` or be wrapped in `<a>` / `<button>` for keyboard accessibility. Empty-state cards acting as "add new" should have `aria-label="Add new item"`.

### Part B: Dot Sizing via Custom Property (RFC #6)

Instead of a `.dot-sm` class, make `.dot` size-driven:

- [x] Modify the existing `.dot` class in `app.css`:
  - Change `width: 10px; height: 10px;` to `width: var(--dot-size, 10px); height: var(--dot-size, 10px);`
  - Change `border-radius: 50%;` to `border-radius: calc(var(--dot-size, 10px) / 2);` (or keep `50%` since it works for any size)
- [x] Update the `.dot--green`, `.dot--red`, `.dot--yellow`, `.dot--grey` modifiers if their `box-shadow` size should scale with `--dot-size` (use `calc()` if so)
- [x] Document: consumers can override `--dot-size: 8px` on a parent element to get smaller dots in context

### Part C: Badge Modifier (RFC #7)

Instead of a new `.type-badge` or `.chip` class, add a subtle modifier to the existing `.badge`:

- [x] Add `.badge--subtle` modifier:
  - `font-size: 0.65rem; text-transform: uppercase; letter-spacing: 0.05em;`
  - `padding: 0.15rem 0.5rem;`
  - `background: rgba(var(--primary-rgb, 33,150,243), 0.1); color: var(--primary);`
  - `border: none;`
- [x] Dark theme: adjust alpha for readability

### Part D: List-Item / Row Reconciliation (RFC #11)

Current state: `.info-row` (L771) is a key-value flex row with bottom divider. `.compact-row` (L782) is a flex layout pairing input with action button.

**Decision (per PROJECT.md Q3 — implement option C unless user override recorded):**
- [x] Keep `.info-row` and `.compact-row` as-is (they are semantically distinct: data display vs input+action)
- [x] Do NOT add `.list-item` — the RFC's description ("flex row with space-between and bottom border") is identical to `.info-row`
- [x] Add documentation clarifying the distinction:
  - `.info-row` — read-only key-value display (label left, value right, divider)
  - `.compact-row` — interactive input+button pair (label+input left, button right)
- [x] If consumers need a third variant (e.g., clickable list items for scan results), they should use `.card--interactive` inside a `.card-grid` (from Part A)

### Part E: Signal Bars Promotion (RFC #15 + U2)

Promote the existing SVG signal-bars renderer from `wifi.js` local scope to `utils.js` as a shared utility.

**Current implementation** (wifi.js ~L29-43): `signalBarsIcon(bars)` generates an inline SVG with 4 bars using `var(--primary)` for active and `var(--border)` for inactive.

- [x] Add to `web/portal/utils.js`:
  ```
  window.renderSignalBars = function(rssi)
  ```
  - Call `window.rssiToBars(rssi)` to get 0–4 bar level (already in utils.js)
  - Generate the same SVG as the current `signalBarsIcon()` function in wifi.js
  - Wrap in a `<span class="signal-bars">` container
  - Add `aria-label="Signal: N of 4 bars"` to the container `<span>`
  - Return the HTML string

- [x] Add minimal `.signal-bars` CSS class to `app.css`:
  - `display: inline-flex; align-items: center; vertical-align: middle;`
  - (The SVG handles all visual rendering — CSS is just for alignment)

- [x] Update `web/portal/wifi.js`:
  - Remove the local `signalBarsIcon(bars)` function (~L29-43)
  - Remove the local `rssiToBars(rssi)` function (~L16-22) — it's a duplicate of the one in `utils.js`
  - Update the two usage sites:
    - ~L236: `statusSignal.innerHTML = signalBarsIcon(bars) + ...` → `statusSignal.innerHTML = window.renderSignalBars(data.rssi) + ...`
    - ~L372: `${signalBarsIcon(bars)}` → `${window.renderSignalBars(net.rssi)}`
  - Verify that `rssiLabel(rssi)` (local function ~L50) is NOT duplicated in utils.js — if it's only used in wifi.js, keep it local

### Part F: Documentation

- [x] Update `docs/developer/extending-the-web-portal.md`:
  - Document `.card--interactive` and `.card--empty` modifiers with usage examples
  - Document `.card-grid` and `.card-grid--2col` with example markup
  - Document `--dot-size` custom property usage
  - Document `.badge--subtle` modifier
  - Clarify `.info-row` vs `.compact-row` distinction
  - Document `window.renderSignalBars(rssi)` with usage example

- [x] Update `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`:
  - Add `window.renderSignalBars(rssi)` with parameter table, return shape, a11y notes
  - Update `.dot` section to mention `--dot-size` custom property
  - Update `.badge` section to include `.badge--subtle`

## Verification Requirement

1. **WiFi tab regression:** After signal bars promotion, verify that the wifi.js file parses without errors and the signal bars rendering still uses the same SVG output.
2. **CSS syntax:** No unclosed braces or invalid selectors in `app.css`.
3. **Grep check:** Confirm `signalBarsIcon` and the local `rssiToBars` no longer exist in `wifi.js`.
4. **Playwright tests:** Run the suite if applicable — WiFi tab tests may exercise signal bars.
5. **CRITICAL:** Before distilling or committing, ask the user if they are happy. Only after approval, invoke the `@distillery` skill.

---

## Execution Status

- **2026-08-05:** Work Order 4 fully implemented and verified.
  - Implemented card grid modifiers, dot size variable scaling, subtle badge, and promoted signal bars renderer.
  - Verified 70/70 Playwright test suite green.
  - User approval confirmed; output distilled to `.agents/skills/principal-architect-workspace/pr-55-portal-design-system/wo4-consolidation/`.
