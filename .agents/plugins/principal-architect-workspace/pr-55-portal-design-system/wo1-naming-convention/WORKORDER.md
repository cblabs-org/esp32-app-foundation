---
name: pr-55-wo1
description: Handoff instructions for Work Order 1 — BEM naming convention ADR + legacy CSS class migration
complexity: standard
---

# WO1 — BEM Naming Convention ADR + Legacy CSS Class Migration

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Adopt BEM (`block--modifier`) as the CSS naming convention for the portal design system. Write a one-page ADR documenting the decision. Migrate all ~24 legacy classes that use inconsistent naming patterns (hyphen-suffix variants and compound/state classes) to BEM. Update all JS, HTML, and test references.

## Parent Project

`pr-55-portal-design-system` — RFC #67 implementation. This is WO1 of 4.

## Strict Boundary Box

- **IN SCOPE:** `web/portal/app.css`, `web/portal/*.js`, `web/portal/index.html`, `docs/developer/adr/`, `docs/developer/extending-the-web-portal.md`, `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`, `test/web/` (Playwright selectors)
- **OUT OF SCOPE:** C++ source files (`src/`, `include/`, `lib/`), `platformio.ini`, build scripts, any interface changes. Do NOT add new CSS components — that is WO3/WO4. Do NOT formalize `.card` or `.alert` — that is WO2.

## Tooling Constraints

- Use the `platformio` MCP server for any build verification if needed (but this WO is CSS/JS only — no firmware build required).
- Run Playwright tests via `npx playwright test` in the `test/web/` directory (if it exists and is configured).

## Line-Item Instructions

### ADR

- [x] Create `docs/developer/adr/adr-css-naming-convention.md` with:
  - **Status:** Accepted
  - **Context:** `app.css` has 76 classes across 3+ naming conventions (hyphen-suffix, compound/state, prefixed components). RFC #67 proposes adding 18 new classes. Without a convention, a 4th style would be introduced.
  - **Decision:** Adopt BEM (`block--modifier`) for all CSS classes going forward. Migrate existing non-conforming classes. Element selectors (`article`, `dialog`, `button`) are exempt (they are element resets, not class conventions). Prefixed component names (`.status-badge`, `.info-row`) are already BEM-compatible block names and stay as-is.
  - **Consequences:** ~24 classes renamed. All JS/HTML references updated. Playwright selectors updated. One-time migration cost, permanent consistency gain.

### CSS Migrations in `app.css`

Rename the following classes. For each, update **both** the CSS definition and **every** JS/HTML reference:

**Hyphen-suffix → BEM modifier:**
- [x] `.badge-ok` → `.badge--ok` (L907)
- [x] `.badge-warn` → `.badge--warn` (L911)
- [x] `.rag-green` → `.rag--green` (L639)
- [x] `.rag-amber` → `.rag--amber` (L640)
- [x] `.rag-red` → `.rag--red` (L641)
- [x] `.toast-success` → `.toast--success` (L678)
- [x] `.toast-error` → `.toast--error` (L679)
- [x] `.toast-warning` → `.toast--warning` (L680)
- [x] `.toast-info` → `.toast--info` (L681)
- [x] `.toast-visible` → `.toast--visible` (L673)
- [x] `.btn-delete` → `.btn--delete` (L731)

**Compound/state → BEM modifier:**
- [x] `.dot.green` → `.dot--green` (L596)
- [x] `.dot.red` → `.dot--red` (L597)
- [x] `.dot.yellow` → `.dot--yellow` (L598)
- [x] `.dot.grey` → `.dot--grey` (L599)
- [x] `.dot.pulse` → `.dot--pulse` (L607)
- [x] `.tab-content.active` → `.tab-content--active` (L236)
- [x] `.tab-link.active` → `.tab-link--active` (L295)
- [x] `.pill-btn.active` → `.pill-btn--active` (L338)
- [x] `.btn-ss-colour.active` → `.btn-ss-colour--active` (L345)
- [x] `.theme-btn.active` → `.theme-btn--active` (L378)
- [x] `.login-overlay.hidden` → `.login-overlay--hidden` (L828)
- [x] `.reboot-overlay.visible` → `.reboot-overlay--visible` (L994)

**Important:** For compound selectors like `.dot.green`, the CSS currently uses `.dot.green { ... }` which requires BOTH classes on the element (`<span class="dot green">`). The BEM migration changes this to a single class: `.dot--green { ... }` on `<span class="dot dot--green">`. This means:
1. The CSS selector changes from `.dot.green` to `.dot--green`
2. The HTML/JS that applies the class changes from adding `'green'` to adding `'dot--green'`
3. The base `.dot` class should still be present for base styling

### JS Reference Updates

For each renamed class, `grep` across all `web/portal/*.js` files and update:
- [x] `classList.add()` / `classList.remove()` / `classList.toggle()` calls
- [x] `querySelector()` / `querySelectorAll()` selectors
- [x] String-interpolated class names in template literals (e.g., `class="${...}"`)
- [x] `className` property assignments

Key files likely needing updates:
- `app.js` — tab navigation (`.tab-link.active`, `.tab-content.active`, `.pill-btn.active`, `.toast-*`, `.reboot-overlay.visible`)
- `utils.js` — `showToast()` uses `.toast-success` etc., `showRebootOverlay()` uses `.reboot-overlay.visible`
- `auth.js` — `.login-overlay.hidden`
- `display.js` / `screenshot.js` — `.btn-ss-colour.active`
- `system.js` — `.dot.green`, `.dot.red`, etc.
- `wifi.js` — `.dot.*` variants
- Any file using `.badge-ok`, `.badge-warn`

### HTML Reference Updates

- [x] Update `web/portal/index.html` — check for any inline class references to renamed classes

### Documentation Updates

- [x] Update `docs/developer/extending-the-web-portal.md` — update any references to renamed classes in code samples
- [x] Update `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md` — update documented class names for toast, reboot overlay, progress bar, etc.

### Playwright Test Updates

- [x] Update `test/web/` — find and update any CSS selectors or class name assertions that reference renamed classes

## Verification Requirement

1. **Grep verification:** After all renames, run `grep -rn` for every OLD class name across `web/portal/`, `docs/`, and `test/web/` to confirm zero remaining references.
2. **Playwright tests:** Run the full Playwright test suite (if configured) and report results.
3. **Build check:** Confirm the portal builder/gzip step still succeeds (if applicable).
4. **CRITICAL:** Before distilling or committing, ask the user if they are happy with the changes. Only after approval, invoke the `@distillery` skill.

---

## Status Log

| Date | Status | Details |
|---|---|---|
| 2026-08-05 | ✅ **Completed & Validated** | Adopted BEM naming convention ADR (`adr-css-naming-convention.md`). Migrated ~24 legacy classes in `app.css` and updated all HTML, JS, and Playwright test references (`70/70 passed`). User approved implementation and distilled memory paging artifacts to `.agents/plugins/principal-architect-workspace/pr-55-portal-design-system/wo1-naming-convention/`. |

