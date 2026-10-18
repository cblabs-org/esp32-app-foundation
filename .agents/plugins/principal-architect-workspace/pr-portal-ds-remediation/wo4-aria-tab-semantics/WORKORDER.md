---
name: portal-ds-remediation-wo4
description: Handoff instructions for Work Order 4 — ARIA Tab Semantics
complexity: standard
---

# WO4 — ARIA Tab Semantics

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add proper ARIA tab semantics to the portal's bottom navigation and content area. This ensures screen readers and assistive technology correctly identify the tab bar, individual tabs, and the active tab panel. Also add a skip-navigation link for keyboard users. **~10 changes across 2 files.**

## Strict Boundary Box
- **ONLY** modify `web/portal/index.html` and `web/portal/app.js`
- **ONLY** add/modify ARIA attributes, roles, and the skip-nav link
- Do **NOT** change visual styling, CSS classes, layout, or any behaviour
- Do **NOT** touch any JS fragment files (`wifi.js`, `system.js`, etc.)
- Do **NOT** modify any firmware C++ code
- Do **NOT** change the theme toggle ARIA — it's already correct (`role="radiogroup"`, `aria-label`, `aria-pressed`)

## Tooling Constraints
- Use `replace_file_content` or `multi_replace_file_content` for file editing

## ARIA Tab Pattern Reference

The WAI-ARIA Authoring Practices define the tabs pattern as:
- Container: `role="tablist"` with `aria-label`
- Each tab: `role="tab"`, `aria-selected="true|false"`, `aria-controls="<panel-id>"`
- Tab panel: `role="tabpanel"`, `aria-labelledby="<tab-id>"`, `id="<panel-id>"`
- Only the active tab has `aria-selected="true"`; others have `aria-selected="false"`
- The tab panel should have `tabindex="0"` to be focusable

## Line-Item Instructions

### Part A: `web/portal/index.html`

- [ ] **Skip-nav link** — Add a visually hidden skip-navigation link as the **first child** of `<body>`:
  ```html
  <a href="#main-container" class="skip-nav" style="position:absolute;left:-9999px;top:auto;width:1px;height:1px;overflow:hidden;z-index:9999;padding:8px;background:var(--bg);color:var(--primary);text-decoration:none;font-size:0.85rem;&:focus{position:fixed;left:8px;top:8px;width:auto;height:auto;overflow:visible;}">Skip to main content</a>
  ```
  **Note:** The `&:focus` nested selector won't work in inline styles. Instead, add the skip-nav focus styles as a `<style>` block or use this pattern — just use a class and add the CSS to `app.css`. Actually, since we're boundary-boxed to `index.html` and `app.js`, use this simpler inline approach with a `focus` event handler OR just add the style attribute with `onfocus`/`onblur`:
  ```html
  <a href="#main-container" class="skip-nav" id="skip-nav" style="position:absolute;left:-9999px;top:auto;width:1px;height:1px;overflow:hidden;z-index:9999;padding:8px 16px;background:var(--primary);color:#fff;text-decoration:none;font-size:0.85rem;border-radius:0 0 8px 0;" onfocus="this.style.cssText='position:fixed;left:0;top:0;width:auto;height:auto;overflow:visible;z-index:9999;padding:8px 16px;background:var(--primary);color:#fff;text-decoration:none;font-size:0.85rem;border-radius:0 0 8px 0;'" onblur="this.style.cssText='position:absolute;left:-9999px;top:auto;width:1px;height:1px;overflow:hidden;'">Skip to main content</a>
  ```

- [ ] **Bottom nav** — On the `<nav class="bottom-nav" id="bottom-nav">` element, add:
  - `role="tablist"`
  - `aria-label="Main navigation"`

- [ ] **Static nav items** (WiFi, Security, System `<a>` elements inside `bottom-nav`) — add to each:
  - `role="tab"`
  - `aria-selected="false"` (the active one gets `"true"` — `app.js` will manage this)
  - Give each a unique `id` if not already present: `id="tab-wifi"`, `id="tab-auth"`, `id="tab-system"`
  - `aria-controls="main-container"`

- [ ] **Main container** — On `<div class="container" id="main-container">`, add:
  - `role="tabpanel"`
  - `aria-labelledby="tab-wifi"` (initial default — `app.js` will update this)
  - `tabindex="0"`

### Part B: `web/portal/app.js`

- [ ] **`switchTab()` function** — When switching tabs, update ARIA states:
  - Set `aria-selected="false"` on ALL nav items (including dynamic tabs)
  - Set `aria-selected="true"` on the newly active nav item
  - Update `aria-labelledby` on `#main-container` to point to the active tab's `id`

- [ ] **Dynamic tab creation** — Where `app.js` creates nav items for module-contributed tabs (in the `#dynamic-tabs` container), ensure each dynamically created nav item gets:
  - `role="tab"`
  - `aria-selected="false"` (initially)
  - A unique `id` attribute (e.g., `id="tab-${tabId}"`)
  - `aria-controls="main-container"`

## Verification Requirement
1. Run:
   ```
   grep -rn 'role="tablist"\|role="tab"\|aria-selected\|role="tabpanel"\|aria-labelledby\|skip-nav' web/portal/index.html web/portal/app.js
   ```
   Should return hits for all ARIA attributes and the skip-nav link.
2. Verify that the existing theme toggle ARIA (`role="radiogroup"`, `aria-label="Theme"`, `aria-pressed`) is untouched.
3. **CRITICAL:** Before committing, you MUST ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
