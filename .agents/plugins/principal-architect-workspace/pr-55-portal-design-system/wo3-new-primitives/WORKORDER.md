---
name: pr-55-wo3
description: Handoff instructions for Work Order 3 — Core new CSS primitives and JS utilities
complexity: standard
---

# WO3 — Core New Primitives

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add the uncontested new CSS component patterns and JS utilities identified in RFC #67. Every component ships with its accessibility contract defined.

## Parent Project

`pr-55-portal-design-system` — RFC #67 implementation. This is WO3 of 4.
Depends on: WO2 (`.card`, `.alert`, ApiQueue landed).

## Strict Boundary Box

- **IN SCOPE:** `web/portal/app.css`, `web/portal/utils.js`, `docs/developer/extending-the-web-portal.md`, `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`
- **OUT OF SCOPE:** C++ source, interfaces, build config, `app.js`. Do NOT add card-grid, signal bars, dot sizing, or badge modifiers — that is WO4. Do NOT modify the ApiQueue — that was WO2.

## Tooling Constraints

- CSS/JS only — no firmware build required.

## Line-Item Instructions

### CSS Components (add to `app.css`)

All new classes MUST follow the BEM convention established in WO1's ADR.

#### #1 — Loading Button State
- [ ] Add `button[aria-busy="true"]` selector:
  - `pointer-events: none; opacity: 0.7;`
  - Add a CSS-only spinner via `::after` pseudo-element (small rotating border circle)
  - The spinner should use `var(--card-bg)` or white for visibility on primary-colored buttons
- [ ] A11y: `aria-busy="true"` already communicates loading state to screen readers. Document that consumers must set `aria-busy="true"` on the button element.

#### #8 — Instruction Callout
- [ ] Add `.callout` class:
  - `border-left: 4px solid var(--primary)`
  - `background: rgba(var(--primary-rgb, 33,150,243), 0.06)` (or use an alpha-tinted custom property)
  - `padding: 0.75rem 1rem; border-radius: 0 8px 8px 0; margin-bottom: 1rem;`
  - `font-size: 0.85rem; line-height: 1.5;`
- [ ] Dark theme: adjust alpha for readability
- [ ] A11y: document that callouts should use `<aside>` or have `role="note"` when they contain supplementary guidance

#### #9 — Terminal Log Viewer
- [ ] Add `.terminal-log` class:
  - `background: #1a1a1a; color: #4ade80;` (green-on-black terminal aesthetic)
  - `font-family: 'Courier New', monospace; font-size: 0.75rem;`
  - `padding: 0.75rem; border-radius: 8px;`
  - `max-height: 200px; overflow-y: auto;`
  - `white-space: pre-wrap; word-break: break-all;`
- [ ] Add `.terminal-log__line` for individual log lines (optional BEM element)
- [ ] A11y: document that the container should have `role="log"` and `aria-live="polite"` for live-updating content

#### #12 — Dialog Close Button
- [ ] Add `.dialog-close-btn` class:
  - Absolute positioning: `position: absolute; top: 0.75rem; right: 0.75rem;`
  - Minimal styling: `background: none; border: none; cursor: pointer; padding: 0.25rem;`
  - Size: `width: 24px; height: 24px;`
  - Color: `color: var(--text-muted);`
  - Hover: `color: var(--text);`
- [ ] A11y: document that the button must have `aria-label="Close"`

#### #14 — Password Mask Placeholder
- [ ] Add `.password-mask::placeholder` rule:
  - `letter-spacing: 4px;`
- [ ] This is purely cosmetic for the "●●●●●●●● (saved)" placeholder display

#### #16 — Field Validation Feedback
- [ ] Add `.field-feedback` base class:
  - `font-size: 0.75rem; margin-top: 0.25rem; display: block;`
- [ ] Add `.field-feedback--error`: `color: var(--error);`
- [ ] Add `.field-feedback--success`: `color: var(--success);`
- [ ] A11y: document that validation messages should be linked via `aria-describedby` on the input element

#### #17 — Disabled Section State
- [ ] Add `article[aria-disabled="true"]` (and `.card[aria-disabled="true"]`) selector:
  - `opacity: 0.5; pointer-events: none;`
  - `filter: grayscale(30%);` (subtle visual cue beyond just opacity)
- [ ] A11y: `aria-disabled="true"` already communicates state. Document that this is the canonical pattern for `IConfigGate`-gated sections.

#### #18 — Compact Row Documentation
- [ ] `.compact-row` already exists in `app.css`. No CSS changes needed.
- [ ] Document it in the extending guide (see Documentation section below).

### JS Utility: `renderTerminalLog` (U3)

- [ ] Add to `web/portal/utils.js`:
  ```
  window.renderTerminalLog = function(containerId)
  ```
  - Creates or locates the container element by ID
  - Sets `role="log"` and `aria-live="polite"` on the container
  - Adds the `.terminal-log` class
  - Returns an object: `{ append(line), clear(), scrollToBottom() }`
  - `append(line)` — creates a `<div class="terminal-log__line">` with escaped content (use `window.escapeHtml`), appends it, and auto-scrolls to bottom
  - `clear()` — empties the container
  - `scrollToBottom()` — scrolls container to the bottom

### Documentation

- [ ] Update `docs/developer/extending-the-web-portal.md`:
  - Add a "CSS Component Reference" section (or expand the existing "Styling" section)
  - Document each new component with: class name, usage example (HTML snippet), accessibility requirements
  - Document `.compact-row` (item #18 — exists but was undocumented)
- [ ] Update `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`:
  - Add `window.renderTerminalLog(containerId)` with parameter table, return shape, and usage example

## Verification Requirement

1. **CSS syntax check:** Ensure no syntax errors in `app.css` (no unclosed braces, valid selectors).
2. **JS syntax check:** Ensure `utils.js` parses without errors.
3. **Grep check:** Verify new class names don't conflict with existing classes.
4. **CRITICAL:** Before distilling or committing, ask the user if they are happy. Only after approval, invoke the `@distillery` skill.

---

## Status Update

- **Date:** 2026-08-05
- **Status:** ✅ Complete & Validated
- **Summary:**
  - Added CSS primitives to `web/portal/app.css` (`button[aria-busy="true"]`, `.callout`, `.terminal-log`, `.terminal-log__line`, `.dialog-close-btn`, `.password-mask::placeholder`, `.field-feedback`, `article[aria-disabled="true"]`, `.card[aria-disabled="true"]`).
  - Added `window.renderTerminalLog(containerId)` (U3) utility to `web/portal/utils.js`.
  - Updated developer documentation in `docs/developer/extending-the-web-portal.md` and `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`.
  - Verified CSS brace syntax and JS syntax via `node -c`.
  - Approved by parent agent and distilled into `context_bridge.md`.
