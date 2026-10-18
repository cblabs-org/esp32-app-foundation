---
name: aria-tabpanel-wo1
description: Handoff instructions for Work Order 1 — ARIA tabpanel semantics
complexity: mechanical
---

# WO1 — ARIA Tabpanel Semantics

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add complete WAI-ARIA tabpanel semantics to the ESP32 App Foundation web portal at both the top-level (bottom nav) and sub-pill (pill bar) levels, migrating from the incomplete single-wrapper pattern to the standard multi-panel pattern.

## Strict Boundary Box
- **Only touch** `web/portal/index.html` and `web/portal/app.js`
- **Do NOT** modify any CSS files, other JS fragment files, or backend code
- **Do NOT** change any visual behaviour — this is a semantic-only change
- **Do NOT** create any new files
- **Do NOT** create branches — the branch is already checked out

## Tooling Constraints
- Use the `platformio` MCP server's `build_project` tool to verify the project still builds (the web files are embedded into firmware via LittleFS, so a build verifies they are well-formed).
- Use `view_file` and `grep_search` for code research.

## Line-Item Instructions

### `web/portal/index.html`

- [ ] **Line 53** — Remove `role="tabpanel"`, `aria-labelledby="nav-wifi"`, and `tabindex="0"` from the outer `#tab-content-area` wrapper div. It should become: `<div class="tab-content-container" id="tab-content-area">`

- [ ] **Line 46** — Add `role="tablist"` and `aria-label="Sub-navigation"` to the pill bar div: `<div id="pill-bar" class="pill-bar" role="tablist" aria-label="Sub-navigation" style="display:none;"></div>`

- [ ] **Line 55** — Add `role="tabpanel"` and `aria-labelledby="nav-wifi"` to `#tab-wifi`: `<div id="tab-wifi" class="tab-content" role="tabpanel" aria-labelledby="nav-wifi">`

- [ ] **Line 60** — Add `role="tabpanel"` and `aria-labelledby="nav-display"` to `#tab-display`: `<div id="tab-display" class="tab-content" role="tabpanel" aria-labelledby="nav-display">`

- [ ] **Line 63** — Add `role="tabpanel"` and `aria-labelledby="pill-settings"` to `#tab-settings`: `<div id="tab-settings" class="tab-content" role="tabpanel" aria-labelledby="pill-settings">`

- [ ] **Line 66** — Add `role="tabpanel"` and `aria-labelledby="pill-screenshot"` to `#tab-screenshot`: `<div id="tab-screenshot" class="tab-content" role="tabpanel" aria-labelledby="pill-screenshot">`

- [ ] **Line 70** — Add `role="tabpanel"` and `aria-labelledby="pill-carousel"` to `#tab-carousel`: `<div id="tab-carousel" class="tab-content" role="tabpanel" aria-labelledby="pill-carousel">`

- [ ] **Line 73** — Add `role="tabpanel"` and `aria-labelledby="pill-hardware"` to `#tab-hardware`: `<div id="tab-hardware" class="tab-content" role="tabpanel" aria-labelledby="pill-hardware">`

- [ ] **Line 77** — Add `role="tabpanel"` and `aria-labelledby="nav-app"` to `#tab-app`: `<div id="tab-app" class="tab-content" role="tabpanel" aria-labelledby="nav-app">`

- [ ] **Line 82** — Add `role="tabpanel"` and `aria-labelledby="nav-security"` to `#tab-security`: `<div id="tab-security" class="tab-content" role="tabpanel" aria-labelledby="nav-security">`

- [ ] **Line 85** — Add `role="tabpanel"` and `aria-labelledby="pill-password"` to `#tab-password`: `<div id="tab-password" class="tab-content" role="tabpanel" aria-labelledby="pill-password">`

- [ ] **Line 88** — Add `role="tabpanel"` and `aria-labelledby="pill-tls"` to `#tab-tls`: `<div id="tab-tls" class="tab-content" role="tabpanel" aria-labelledby="pill-tls">`

- [ ] **Line 92** — Add `role="tabpanel"` and `aria-labelledby="nav-system"` to `#tab-system`: `<div id="tab-system" class="tab-content" role="tabpanel" aria-labelledby="nav-system">`

- [ ] **Line 95** — Add `role="tabpanel"` and `aria-labelledby="pill-status"` to `#tab-status`: `<div id="tab-status" class="tab-content" role="tabpanel" aria-labelledby="pill-status">`

- [ ] **Line 98** — Add `role="tabpanel"` and `aria-labelledby="pill-time"` to `#tab-time`: `<div id="tab-time" class="tab-content" role="tabpanel" aria-labelledby="pill-time">`

- [ ] **Line 100** — Add `role="tabpanel"` and `aria-labelledby="pill-keys"` to `#tab-keys`: `<div id="tab-keys" class="tab-content" role="tabpanel" aria-labelledby="pill-keys">`

- [ ] **Line 103** — Add `role="tabpanel"` and `aria-labelledby="pill-mcp"` to `#tab-mcp`: `<div id="tab-mcp" class="tab-content" role="tabpanel" aria-labelledby="pill-mcp">`

- [ ] **Line 106** — Add `role="tabpanel"` and `aria-labelledby="pill-backup"` to `#tab-backup`: `<div id="tab-backup" class="tab-content" role="tabpanel" aria-labelledby="pill-backup">`

- [ ] **Line 109** — Add `role="tabpanel"` and `aria-labelledby="pill-ota"` to `#tab-ota`: `<div id="tab-ota" class="tab-content" role="tabpanel" aria-labelledby="pill-ota">`

- [ ] **Line 123** — Change `aria-controls="tab-content-area"` to `aria-controls="tab-wifi"` on the WiFi nav link

- [ ] **Line 136** — Change `aria-controls="tab-content-area"` to `aria-controls="tab-security"` on the Security nav link

- [ ] **Line 147** — Change `aria-controls="tab-content-area"` to `aria-controls="tab-system"` on the System nav link

### `web/portal/app.js`

- [ ] **Lines 329–335** — Dynamic top-level tab content div creation: add `role="tabpanel"` and `aria-labelledby` after setting className:
  ```javascript
  div.setAttribute('role', 'tabpanel');
  div.setAttribute('aria-labelledby', 'nav-' + tabId.replace('tab-', ''));
  ```

- [ ] **Line 353** — Dynamic nav link creation: change `aria-controls` value from `'tab-content-area'` to `tabId`:
  ```javascript
  link.setAttribute('aria-controls', tabId);
  ```

- [ ] **Lines 368–379** — Dynamic sub-pill content div creation: add `role="tabpanel"` and `aria-labelledby` after setting className:
  ```javascript
  cdiv.setAttribute('role', 'tabpanel');
  cdiv.setAttribute('aria-labelledby', 'pill-' + childId.replace('tab-', ''));
  ```

- [ ] **Lines 472–476** — Pill button creation in `buildPillBar()`: add `id`, `role="tab"`, `aria-selected`, and `aria-controls`:
  ```javascript
  btn.id = 'pill-' + childId.replace('tab-', '');
  btn.setAttribute('role', 'tab');
  btn.setAttribute('aria-selected', 'false');
  btn.setAttribute('aria-controls', childId);
  ```

- [ ] **Lines 496–499** — `activateSubPill()`: update the pill button `aria-selected` state toggling to include ARIA. Replace:
  ```javascript
  btn.classList.toggle('pill-btn--active', btn.getAttribute('data-target') === childId);
  ```
  with:
  ```javascript
  var isActive = btn.getAttribute('data-target') === childId;
  btn.classList.toggle('pill-btn--active', isActive);
  btn.setAttribute('aria-selected', isActive ? 'true' : 'false');
  ```

- [ ] **Lines 410–415** — Remove the dynamic `aria-labelledby` update block in `activateTopTab()`. Delete these lines:
  ```javascript
  // --- Update tabpanel label ---
  var contentArea = document.getElementById('tab-content-area');
  var activeLink = bottomNav.querySelector('.tab-link--active');
  if (contentArea && activeLink && activeLink.id) {
    contentArea.setAttribute('aria-labelledby', activeLink.id);
  }
  ```

## Verification Requirement
1. Run a firmware build using `build_project` MCP tool to verify the project compiles cleanly.
2. Visually inspect the modified files to confirm all ARIA attributes are correctly applied.
3. Verify no visual/behavioural changes were introduced — only ARIA semantic attributes.
4. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion, verified by PA -->

### Status
complete

### Changes Made
- `web/portal/index.html` — Removed `role="tabpanel"`, `aria-labelledby`, `tabindex="0"` from outer `#tab-content-area` wrapper. Added `role="tablist"` and `aria-label="Sub-navigation"` to `#pill-bar`. Added `role="tabpanel"` and `aria-labelledby` to all 17 static tab content divs. Updated `aria-controls` on 3 hardcoded nav links to point at specific panel IDs.
- `web/portal/app.js` — Added ARIA attributes to dynamically created top-level tab content divs and sub-pill content divs. Changed `aria-controls` on dynamic nav links to target specific panels. Added `id`, `role="tab"`, `aria-selected`, `aria-controls` to dynamic pill buttons. Added `aria-selected` toggling in `activateSubPill()`. Removed obsolete dynamic `aria-labelledby` update in `activateTopTab()`.

### Decisions Made
- None — all changes followed the WORKORDER specification exactly.

### Dead Ends & Warnings
- Worker used `pio run` CLI instead of the `platformio` MCP `build_project` tool, violating rule 04 and the WORKORDER Tooling Constraints. PA re-verified via MCP `build_project` after worker completion. Future WORKORDERs should reinforce MCP constraints in the Line-Item Instructions section.

### Out-of-Scope Findings
- None.

### Blockers
(none)
