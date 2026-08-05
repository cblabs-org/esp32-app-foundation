---
name: pr-55-wo2
description: Handoff instructions for Work Order 2 — Debt repair (formalize .card, .alert, ApiQueue priority lanes)
complexity: architect
---

# WO2 — Debt Repair: Formalize `.card`, `.alert`, ApiQueue Priority Lanes

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Repair three categories of existing debt in the portal:
1. **`.card`** — formalize as a CSS class (currently styled on bare `<article>`, but docs tell consumers to use `<div class="card">`)
2. **`.alert`** — create a unified inline feedback component (currently fragmented across `.ap-banner`, `.form-error`, `.login-error`)
3. **ApiQueue priority lanes** — extend the existing `ApiQueue` in `app.js` with HIGH/LOW priority queues and a configurable delay ("choker") for background tasks

## Parent Project

`pr-55-portal-design-system` — RFC #67 implementation. This is WO2 of 4.
Depends on: WO1 (naming convention migration complete).

## Strict Boundary Box

- **IN SCOPE:** `web/portal/app.css`, `web/portal/app.js`, `web/portal/*.js` (for `.alert` migration), `docs/developer/extending-the-web-portal.md`, `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`
- **OUT OF SCOPE:** C++ source files, interfaces, build config. Do NOT add new CSS components (WO3). Do NOT add card-grid, signal bars, or other consolidation (WO4).

## Tooling Constraints

- This WO is CSS/JS only — no firmware build required.
- Use `grep` extensively to find all references before and after changes.

## Line-Item Instructions

### Part A: Formalize `.card`

The current `article` element selector (app.css, around line 113 after WO1 renaming) defines: `background: var(--card-bg); border-radius: 16px; padding: 1.25rem; margin-bottom: 1rem; border: 1px solid var(--border);`.

- [ ] Create a `.card` class in `app.css` that mirrors the current `article` styling
- [ ] Keep the `article` selector as-is for backward compatibility (both `article` and `.card` should produce identical visual output)
- [ ] Add a CSS comment: `/* .card — canonical card container. article selector retained for backward compat. See ADR. */`
- [ ] Verify that `extending-the-web-portal.md:127`'s `<div class="card">` sample now actually works
- [ ] Update the `extending-the-web-portal.md` documentation to note that `.card` is the canonical class, and `<article>` is the backward-compatible alias

### Part B: Formalize `.alert`

Create a unified `.alert` component with BEM modifiers:

- [ ] Add `.alert` base class to `app.css`:
  - `padding: 0.75rem 1rem`
  - `border-radius: 8px`
  - `border: 1px solid`
  - `font-size: 0.85rem`
  - `margin-bottom: 0.75rem`
- [ ] Add `.alert--success` — green tint background, green border, green text
- [ ] Add `.alert--error` — red tint background, red border, red text
- [ ] Add `.alert--warning` — amber tint background, amber border, amber text
- [ ] Add `.alert--info` — blue tint background, blue border, blue text
- [ ] A11y: add `role="alert"` requirement in documentation for dynamic alerts, `role="status"` for static ones

**Migration audit:**
- [ ] `.form-error` (app.css ~L889 after WO1) — migrate to `.alert--error`. Search for all JS references in `auth.js` and other files that create/manipulate `.form-error` elements.
- [ ] `.login-error` (app.css ~L859 after WO1) — evaluate: if it has unique login-card-specific positioning, keep as `.login-card__error` (BEM element). If it's just a red box, migrate to `.alert--error`.
- [ ] `.ap-banner` (app.css ~L88) — this has special `body.ap-mode` show/hide logic. Evaluate whether it should become `.alert--warning` with a unique ID for the AP-mode toggle, or stay as a separate structural component. **Decision guidance:** if `.ap-banner` has unique absolute/fixed positioning or body-state-driven visibility, keep it as a standalone component (rename to `.ap-banner` stays). If it's just a styled box, migrate.
- [ ] Remove old class definitions from `app.css` after migration is verified
- [ ] Update all JS files that create or reference migrated classes

### Part C: ApiQueue Priority Lanes

Extend the existing `ApiQueue` class in `web/portal/app.js` (currently lines ~10–91 after WO1):

- [ ] Split `this.queue` into `this.highQueue` (UI requests) and `this.lowQueue` (background tasks)
- [ ] Add `this.lowDelayMs = 0` for configurable delay between background tasks
- [ ] Add `this._delayTimer = null` for the delay timer reference
- [ ] Modify `enqueue(url, options)` to push to `highQueue` and cancel any active delay timer (pre-emption)
- [ ] Add `enqueueTask(taskFn, delayMs)` method:
  - Pushes to `lowQueue`
  - If `delayMs` is provided, sets `this.lowDelayMs = delayMs`
  - Returns a Promise that resolves/rejects with the task result
- [ ] Add `_cancelDelay()` helper to clear the delay timer
- [ ] Modify `processNext()` to:
  - Drain `highQueue` first (always), then `lowQueue`
  - After processing a LOW item: if more LOW items exist AND no HIGH items are waiting AND `lowDelayMs > 0`, use `setTimeout` with the delay before calling `processNext` again
  - After processing a HIGH item or when HIGH items are waiting: call `processNext` immediately (via `setTimeout(..., 0)`)
- [ ] Add `clearBackground()` method — empties `lowQueue` and cancels delay timer
- [ ] Add `get pendingBackground()` getter — returns `lowQueue.length`
- [ ] Expose on the global API:
  ```js
  window.apiFetch.enqueueTask = function(taskFn, delayMs) { return api.enqueueTask(taskFn, delayMs); };
  window.apiFetch.clearBackground = function() { api.clearBackground(); };
  Object.defineProperty(window.apiFetch, 'pendingBackground', { get: function() { return api.pendingBackground; } });
  ```
- [ ] Preserve ALL existing `enqueue()` behavior — the auth interceptor, error handling, JSON parsing, etc. must remain unchanged
- [ ] Add JSDoc comments to all new methods

### Part D: Documentation

- [ ] Update `docs/developer/extending-the-web-portal.md`:
  - Document `.card` as the canonical card class
  - Document `.alert` component with usage examples
  - Document `apiFetch.enqueueTask()` for background operations
- [ ] Update `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`:
  - Add `.alert` component documentation
  - Add `apiFetch.enqueueTask()` documentation with parameter table and examples
  - Add `apiFetch.clearBackground()` and `apiFetch.pendingBackground` documentation

## Verification Requirement

1. **Grep verification:** Confirm no orphaned references to old `.form-error` / migrated classes remain.
2. **Behavioral verification:** The existing `apiFetch()` calls across all tabs must work identically (no regression). Write a brief test scenario description for the priority pre-emption behavior.
3. **CRITICAL:** Before distilling or committing, ask the user if they are happy. Only after approval, invoke the `@distillery` skill.
