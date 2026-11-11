---
name: pr-23-wo1
description: Full implementation — AppTabContributor, Display tab with sub-pills, appconfig.js split into display.js + appconfig.js, index.html update, system.js link removal, all tests updated
complexity: standard
---

# PR-23 / WO1 — Display Tab + App Tab Separation

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Separate display-hardware settings from application-level configuration:

1. **`U8G2DisplayModule`** becomes owner of a `"Display"` top-level bottom-nav tab with two sub-pills: `"Settings"` (hardware controls) and `"Screenshot"` (framebuffer capture).
2. **A new `AppTabContributor` struct** (owned by `Application`) becomes owner of the `"App"` top-level tab, which retains the clock font size control.
3. **`appconfig.js`** is split: display-hardware content moves to a new **`display.js`**; `appconfig.js` keeps only the clock font size, targeting `#tab-app`.
4. **`screenshot.js`** gets its header comment updated; functionally unchanged.
5. **`system.js`** has its orphaned screenshot link removed.

---

## Strict Boundary Box

- ✅ Modify: `U8G2DisplayModule.hpp`, `U8G2DisplayModule.cpp`, `Application.hpp`, `Application.cpp`, `web/portal/index.html`, `web/portal/app.js`, `web/portal/appconfig.js`, `web/portal/screenshot.js`, `web/portal/system.js`, `test/web/tests/display.spec.ts`, `test/web/tests/navigation.spec.ts`, `test/web/server.js`, any unit test asserting `getTabName()` on U8G2DisplayModule.
- ✅ Create: `web/portal/display.js` (new file — display hardware controls).
- ❌ DO NOT create new REST routes or change the REST API surface.
- ❌ DO NOT touch `WebServerManager`, OTA, WiFi, ConfigManager, or any other module.
- ❌ DO NOT modify `screenshot.html` or the compressed-asset serving in `WebServerManager`.
- ❌ DO NOT branch, commit, merge, or push. The PA handles git.

---

## Background: Current Wiring

- `U8G2DisplayModule::getTabName()` returns `"App"`, `getTabOrder()` returns `30`, `getParentTab()` returns `nullptr` (top-level)
- `Application::begin()` line 187: `_webContributors.push_back(&_display);`
- `appconfig.js` populates `#tab-app` with: brightness slider, flip toggle, screensaver controls, dim/blank timeouts, wake/sleep buttons, and clock font size selector
- `screenshot.js` populates `#tab-screenshot` (listens for `tab-changed` → `tab-screenshot`)
- `system.js` ~line 115: contains `<a href="/screenshot.html">📷 Display Screenshot Tool</a>` link
- Tab order in the portal: WiFi (order 10), Display/App (order 30), System (order 100)

## Target Navigation Hierarchy

```
Bottom nav: [ WiFi (10) ] [ Display (30) ] [ App (40) ] [ System (100) ]
                               │
                       [ Settings (10) ] [ Screenshot (20) ]
```

---

## Pre-Work: Read ALL files first

Before writing a single line, read:
1. `modules/U8G2Display/U8G2DisplayModule.hpp` (lines 340–365 — IWebContributor section)
2. `modules/U8G2Display/U8G2DisplayModule.cpp` (lines 695+ — `registerRoutes()`)
3. `include/interfaces/IWebContributor.hpp` (full — 86 lines)
4. `modules/Application/Application.hpp` (lines 335–460 — private section)
5. `modules/Application/Application.cpp` (lines 179–200 — contributor registration)
6. `web/portal/index.html` (full — 114 lines)
7. `web/portal/app.js` (lines 390–442 — `loadScripts()`)
8. `web/portal/appconfig.js` (full — 350 lines)
9. `web/portal/screenshot.js` (full — 203 lines)
10. `web/portal/system.js` (~lines 110–120 — screenshot link)
11. `test/web/tests/display.spec.ts` (full — MOCK_TABS, `navigateToDisplayTab()`)
12. `test/web/tests/navigation.spec.ts` (lines 1–40 — MOCK_TABS)
13. `test/web/server.js` (lines 30–50 — `/api/tabs` mock)

---

## Implementation Checklist

### ── BLOCK A: C++ Changes ──────────────────────────────────────────────

#### A1 — `U8G2DisplayModule.hpp`: rename tab + add sub-pill stubs

**a) Update `getTabName()`** (line 358) to return `"Display"`:
```cpp
const char* getTabName() const override { return "Display"; }
```
Update its doc comment to say `@return "Display"`.

**b) Add two private nested structs** in the `private:` section, after the IWebContributor block (before or after line 366):

```cpp
// -------------------------------------------------------------------------
// Sub-tab navigation stubs — provide "Settings" and "Screenshot" child tab
// entries under the "Display" parent in /api/tabs. They carry no routes;
// all REST routes remain registered by the main module via registerRoutes().
// Owned as members; exposed via settingsTabStub() / screenshotTabStub()
// for registration by Application::begin().
// -------------------------------------------------------------------------

struct SettingsTabStub : public IWebContributor {
    void        registerRoutes(AsyncWebServer& /*server*/) override {}
    const char* getTabName()   const override { return "Settings"; }
    const char* getParentTab() const override { return "Display";  }
    int         getTabOrder()  const override { return 10;         }
};

struct ScreenshotTabStub : public IWebContributor {
    void        registerRoutes(AsyncWebServer& /*server*/) override {}
    const char* getTabName()   const override { return "Screenshot"; }
    const char* getParentTab() const override { return "Display";    }
    int         getTabOrder()  const override { return 20;           }
};
```

**c) Add member fields** in the private section:
```cpp
SettingsTabStub    _settingsTabStub;    ///< Nav stub: Settings sub-pill under Display
ScreenshotTabStub  _screenshotTabStub;  ///< Nav stub: Screenshot sub-pill under Display
```

**d) Add public accessor methods** in the IWebContributor section:
```cpp
/**
 * @brief Return the Settings sub-pill stub for registration by Application.
 * @return Pointer to the Settings IWebContributor stub.
 */
IWebContributor* settingsTabStub()   { return &_settingsTabStub;   }

/**
 * @brief Return the Screenshot sub-pill stub for registration by Application.
 * @return Pointer to the Screenshot IWebContributor stub.
 */
IWebContributor* screenshotTabStub() { return &_screenshotTabStub; }
```

**Follow house style** — match the doc block format used for the existing `getTabName()` etc.

---

#### A2 — `Application.hpp`: add `AppTabContributor`

In the `private:` section of `Application` (around the other member declarations), add:

```cpp
// -------------------------------------------------------------------------
// App tab contributor — registers the top-level "App" bottom-nav tab.
// Application owns this tab; it is not the responsibility of any hardware
// module. App-level configuration (e.g., clock font size) lives here.
// -------------------------------------------------------------------------

struct AppTabContributor : public IWebContributor {
    void        registerRoutes(AsyncWebServer& /*server*/) override {}
    const char* getTabName()  const override { return "App"; }
    int         getTabOrder() const override { return 40;    }
};
AppTabContributor _appTabContributor;  ///< Contributes "App" top-level nav tab
```

Note: No `IWebContributor.hpp` include is needed in `Application.hpp` — it is already
included at line 55.

---

#### A3 — `Application.cpp`: register all contributors

In `Application::begin()`, find the current contributor registration block (~lines 179–200).
Currently line 187 is: `_webContributors.push_back(&_display);`

Replace or augment to:
```cpp
_webContributors.push_back(&_display);
_webContributors.push_back(_display.settingsTabStub());
_webContributors.push_back(_display.screenshotTabStub());
_webContributors.push_back(&_appTabContributor);
```

No other changes to `Application.cpp`.

---

### ── BLOCK B: Web Portal Changes ──────────────────────────────────────

#### B1 — `web/portal/index.html`: rename + add divs

1. Find `<div id="tab-app" class="tab-content">` and rename to `<div id="tab-display" class="tab-content">`. Update its comment.
2. After `tab-display`, add:
   ```html
   <!-- Display sub-pill: Settings (populated by display.js) -->
   <div id="tab-settings" class="tab-content">
   </div>
   ```
3. The existing `<div id="tab-screenshot" class="tab-content">` already has the correct ID — leave it in place, just update its comment to say `Display sub-pill`.
4. Add a new `<div id="tab-app" class="tab-content">` for the App tab (after the Display group):
   ```html
   <!-- App tab (top-level, application-level config) -->
   <div id="tab-app" class="tab-content">
     <!-- Populated by appconfig.js -->
   </div>
   ```

The final order in the HTML should be:
```html
<div id="tab-wifi" ...>
<div id="tab-display" ...>
<div id="tab-settings" ...>
<div id="tab-screenshot" ...>
<div id="tab-app" ...>
<div id="tab-system" ...>
<div id="tab-status" ...>
... (rest of System sub-pills)
```

---

#### B2 — Create `web/portal/display.js` (new file)

Extract all display-hardware controls from `appconfig.js` into a new `display.js`.

**Content to move from `appconfig.js`:**
- Brightness slider section
- Flip 180° toggle
- Screensaver card (enable toggle, status badge, dim/blank selects, wake/sleep buttons)

**References to update in the extracted code:**
- `document.getElementById('tab-app')` → `document.getElementById('tab-settings')`
- `tab-changed` → `'tab-settings'` event check
- File header comment should read:
  ```javascript
  // web/portal/display.js
  // Display → Settings sub-pill — hardware display controls.
  // Brightness, screen flip, screensaver enable/disable, dim/blank timeouts.
  // Hooks into the "Settings" sub-tab contributed by U8G2DisplayModule.
  // All settings persist via the REST API → ConfigManager pipeline.
  ```

**All REST calls remain the same** (`/api/display/brightness`, `/api/display/flip`,
`/api/display/sleep`) — only the container element ID and tab-changed event change.

---

#### B3 — Trim `web/portal/appconfig.js` to App-only content

After extracting the display hardware controls into `display.js`, `appconfig.js` should
retain only the **clock font size** section.

**References to update:**
- The container element stays `document.getElementById('tab-app')` (already correct — the `#tab-app` div is now the Application's App tab)
- `tab-changed` → `'tab-app'` check stays the same

Update the file header comment:
```javascript
// web/portal/appconfig.js
// App tab — application-level configuration (clock font size, and future app settings).
// Hooks into the "App" tab contributed by Application's AppTabContributor.
// All settings persist via the REST API → ConfigManager pipeline.
```

---

#### B4 — Update `web/portal/screenshot.js`

Update only the header comment (lines 1–3):
```javascript
// web/portal/screenshot.js
// Display → Screenshot sub-pill — Live display framebuffer capture.
// Captures raw U8G2 buffer, renders to canvas, supports zoom and download.
```

No functional changes — the `tab-changed` → `tab-screenshot` listener stays identical
because the C++ stub returns `"Screenshot"` → app.js generates `tab-screenshot` as the
tab content div ID.

---

#### B5 — Update `web/portal/system.js`: remove screenshot link

Find and remove the `📷 Display Screenshot Tool` anchor tag (approximately lines 114–117):
```javascript
      <a href="/screenshot.html" style="display:flex; align-items:center; gap:8px; padding:0.25rem 0; color:var(--primary); text-decoration:none; font-size:0.85rem;">
        📷 Display Screenshot Tool
      </a>
```
This is embedded in an HTML template string inside the system status card. Remove the `<a>` element completely.

---

#### B6 — Update `web/portal/app.js`: add `display.js` to script loader

In `loadScripts()` (~line 396), the current scripts array is:
```javascript
var scripts = [
  'utils.js',
  'wifi.js',
  'system.js',
  'time.js',
  'backup.js',
  'ota.js',
  'keys.js',
  'mcp.js',
  'screenshot.js',
  'appconfig.js'
];
```

Add `'display.js'` before `'appconfig.js'`:
```javascript
  'screenshot.js',
  'display.js',
  'appconfig.js'
```

---

### ── BLOCK C: Test Updates ──────────────────────────────────────────────

#### C1 — `test/web/server.js`: update mock tab list

Find the `/api/tabs` mock array. Currently has:
```javascript
{ name: 'App', id: 'app', parent: null, order: 50 }
```

Replace with:
```javascript
{ name: 'Display',    id: 'display',    parent: null,      order: 30 },
{ name: 'Settings',   id: 'settings',   parent: 'display', order: 10 },
{ name: 'Screenshot', id: 'screenshot', parent: 'display', order: 20 },
{ name: 'App',        id: 'app',        parent: null,       order: 40 },
```

> **Note:** `server.js` uses lowercase parent IDs (`'display'`, `'system'`). Match that convention.

---

#### C2 — `test/web/tests/display.spec.ts`: update MOCK_TABS + nav helper

**a) MOCK_TABS** — replace the App entry:
```typescript
{ name: 'Display',    id: 'display',    parent: null,      order: 30 },
{ name: 'Settings',   id: 'settings',   parent: 'Display', order: 10 },
{ name: 'Screenshot', id: 'screenshot', parent: 'Display', order: 20 },
{ name: 'App',        id: 'app',        parent: null,       order: 40 },
```
> **Note:** `display.spec.ts` uses title-case parent strings (`'Display'`). Match that.

**b) `navigateToDisplayTab()`** — update the click target:
```typescript
async function navigateToDisplayTab(page: Page) {
  // Click the Display top-level tab
  await page.locator('#bottom-nav .tab-link[data-target="tab-display"]').click();
  // Wait for the Settings sub-pill content to render
  await page.locator('#display-brightness').waitFor({ state: 'visible' });
}
```

> **Important:** After clicking `tab-display`, app.js shows the pill bar and auto-activates
> the first child sub-pill. The first child by order is `Settings` (order 10). App.js will
> dispatch `tab-changed` → `tab-settings`, which triggers `display.js` to populate
> `#tab-settings` with the controls including `#display-brightness`. The `waitFor` works
> because it finds the element regardless of which container it lives in.

**c) Scan for any other `tab-app` hardcoded locators** in the file and update them.

---

#### C3 — `test/web/tests/navigation.spec.ts`: update MOCK_TABS

The navigation spec's `MOCK_TABS` currently does not include the App tab. Add the Display
hierarchy and App tab to it:
```typescript
{ name: 'Display',    id: 'display',    parent: null,     order: 30 },
{ name: 'Settings',   id: 'settings',   parent: 'System', order: 10 },  // ← use 'Display' not 'System'
{ name: 'Screenshot', id: 'screenshot', parent: 'Display', order: 20 },
{ name: 'App',        id: 'app',        parent: null,      order: 40 },
```

> **Note:** Check the existing parent casing in this file — it uses title-case (`'System'`).
> Use `'Display'` for the display sub-pill parents.

Review whether any navigation test assertions (e.g., "only WiFi and System appear in the
bottom nav") need updating now that Display and App are also top-level tabs. Update as needed.

---

#### C4 — C++ unit tests: update `getTabName()` assertion

Check `test/test_unit/test_u8g2Display/` for any assertion like:
```cpp
EXPECT_STREQ("App", display.getTabName());
```
Update to:
```cpp
EXPECT_STREQ("Display", display.getTabName());
```

---

### ── BLOCK D: Verification ─────────────────────────────────────────────

#### D1 — Native unit tests

```bash
pio run -e native
```

All tests must pass. In particular, confirm the display tab name test passes.

#### D2 — Playwright web tests

```bash
cd test/web && npm test
```

All tests must pass. If tests fail, diagnose and fix before reporting.

---

## Completion Protocol

Once **both** D1 and D2 pass with zero failures:

1. **Do NOT commit, branch, merge, or push** — the PA handles git.
2. **Send a message back to the PA** summarising:
   - Files created/modified
   - Native test result (pass count)
   - Playwright test result (pass count)
   - Any design decisions you made and why
   - Any concerns or edge cases you noticed
3. Wait for PA's explicit approval.
4. Once approved, invoke `@distillery`, passing `.agents/plugins/principal-architect-workspace/pr-23-display-tab/wo1-display-tab/` as the target directory.
5. Append a status update to this `WORKORDER.md` and terminate your session.

---

## Status: ✅ COMPLETE

**Completed:** 2026-06-14T14:19:00Z  
**Session:** `7ffcffb9-f8e5-4893-9e3a-5d16ba01b00f` (continuation — blocks B–D)  
**Previous session:** Killed after completing blocks A1–A3 (C++ changes)

### Execution Summary

All 5 blocks executed and verified:

| Block | Status | Notes |
|---|---|---|
| A1 — `U8G2DisplayModule.hpp` | ✅ Done (previous worker) | `getTabName()` → `"Display"`, stubs added |
| A2 — `Application.hpp` | ✅ Done (previous worker) | `AppTabContributor` struct added |
| A3 — `Application.cpp` | ✅ Done (previous worker) | 4 contributors registered |
| B1 — `index.html` | ✅ Done | `tab-display`, `tab-settings`, `tab-app` added |
| B2 — `display.js` (new) | ✅ Done | Hardware controls extracted from appconfig.js |
| B3 — `appconfig.js` | ✅ Done | Trimmed to clock font size only |
| B4 — `screenshot.js` | ✅ Done | Header comment updated |
| B5 — `system.js` | ✅ Done | Screenshot link removed |
| B6 — `app.js` | ✅ Done | `display.js` added to loadScripts() |
| C1 — `test/web/server.js` | ✅ Done | Display/Settings/Screenshot/App mock tabs |
| C2 — `display.spec.ts` | ✅ Done | MOCK_TABS + navigateToDisplayTab() updated; clock font test navigates to App tab |
| C3 — `navigation.spec.ts` | ✅ Done | MOCK_TABS extended with Display hierarchy |
| C4 — C++ unit tests | ✅ No-op | No `getTabName()` assertion existed for U8G2DisplayModule |
| D1 — Native tests | ✅ **525/525 passed** | 16.05s |
| D2 — Playwright tests | ✅ **61/61 passed** | 6.1s |

### Distillery Artifacts

- `context_bridge.md` — Main index
- `sessions.md` — Session registry
- `context_artifacts/adr_tab_separation.md` — Architecture decision record
- `context_artifacts/adr_clock_font_test.md` — Clock font test update rationale
- `context_artifacts/graveyard_playwright_waits.md` — Failed wait strategies documented
