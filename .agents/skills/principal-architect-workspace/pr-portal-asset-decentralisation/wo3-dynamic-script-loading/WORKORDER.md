---
name: portal-decentralise-wo3
description: Trim app.js hardcoded script list to core-only; verify dynamic loading via /api/tabs
complexity: mechanical
---

# WO3 — Dynamic Script Discovery

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent.

## Objective
Replace the hardcoded 14-script list in `app.js` `loadScripts()` with a core-only list. Optional module scripts are now loaded dynamically via the `/api/tabs` response (which calls each module's `getTabScript()`). Also ensure `screenshot.js` loads correctly since it has no tab — `display.js` must lazy-load it.

## Strict Boundary Box
- **ONLY** modify `web/portal/app.js`.
- **ONLY** modify `web/display/display.js` (if needed to add lazy loading for screenshot.js).
- **DO NOT** modify any `.cpp`, `.hpp`, or `.h` files.
- **DO NOT** modify `platformio.ini`.
- **DO NOT** branch.

## Tooling Constraints
- Use the `platformio` MCP server for build verification (`build_project`).
- Do NOT use raw `pio` CLI commands.

## Line-Item Instructions

### app.js — Trim script list
- [ ] Replace the `scripts` array in `loadScripts()` (currently lines 573–587) with core-only:
```js
var scripts = [
    'utils.js',
    'wifi.js',
    'system.js',
    'backup.js',
    'keys.js',
    'appconfig.js',
    'auth.js',
    'credits.js'
];
```
- [ ] The `extraScripts` parameter (from `/api/tabs` → `getTabScript()`) already handles appending optional module scripts — verify this code path is intact (lines 590–593).
- [ ] Update the JSDoc comment on `loadScripts()` to reflect the new core-only list.

### display.js — Lazy-load screenshot.js
- [ ] `screenshot.js` has no dedicated tab and is not returned by any `getTabScript()` override. It needs to be loaded when the display tab activates.
- [ ] In `web/display/display.js`, find where the screenshot panel is initialised or first used. Add a one-time dynamic script load:
```js
function loadScreenshotScript() {
    if (document.querySelector('script[src="screenshot.js"]')) return;
    var s = document.createElement('script');
    s.src = 'screenshot.js';
    document.body.appendChild(s);
}
```
- [ ] Call `loadScreenshotScript()` when the display tab initialises (e.g. in the tab's `init()` or `activate()` function).

## Verification Requirement
1. Build `esp32s3nano` via the platformio MCP server — verify the build succeeds.
2. Run `pio test -e native` — verify no test regression.
3. **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect or concern outside your boundary box, surface it in your Completion Report.

---

## Completion Report

### Status
(pending)

### Changes Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)
