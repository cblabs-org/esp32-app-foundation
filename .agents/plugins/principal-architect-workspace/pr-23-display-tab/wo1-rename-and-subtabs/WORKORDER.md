---
name: pr-23-wo1
description: Handoff instructions for Work Order 1 — rename "App" to "Display", add Settings + Screenshot sub-pill stubs, migrate JS, and update all tests
complexity: standard
---

# PR-23 / WO1 — Rename App→Display, Add Sub-Pills, Migrate JS & Tests

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Promote the `U8G2DisplayModule`'s web presence from a top-level `"App"` tab to a proper `"Display"` tab with two sub-pills: **Settings** (the existing display controls) and **Screenshot** (the existing framebuffer capture tool). This is a pure refactor — no new C++ functionality, no new REST routes.

---

## Strict Boundary Box

- ✅ **ALLOWED:** Modify `U8G2DisplayModule.hpp`, `U8G2DisplayModule.cpp`, `modules/Application/Application.cpp`, `web/portal/index.html`, `web/portal/app.js`, `web/portal/appconfig.js`, `web/portal/screenshot.js`, `web/portal/system.js`, `test/web/tests/display.spec.ts`, `test/web/tests/navigation.spec.ts`, `test/web/server.js`, and any unit test that asserts `getTabName()` on `U8G2DisplayModule`.
- ✅ **ALLOWED:** Run `pio run -e native` (unit tests) and `cd test/web && npm test` (Playwright).
- ❌ **DO NOT** create new REST routes, change the REST API surface, or alter any ESP-IDF config.
- ❌ **DO NOT** touch OTA, WiFi, ConfigManager, or any module other than `U8G2DisplayModule` and `Application`.
- ❌ **DO NOT** modify the `screenshot.html` standalone page or `WebServerManager` compressed asset serving — the `/screenshot.html` URL must still work.
- ❌ **DO NOT** branch, commit, or merge. The PA handles git.

---

## Background Context

**Current state:**
- `U8G2DisplayModule::getTabName()` returns `"App"` (top-level bottom-nav tab, no parent)
- `U8G2DisplayModule::getTabOrder()` returns `30`
- `appconfig.js` populates `#tab-app` with all display settings
- `screenshot.js` populates `#tab-screenshot` (orphan div in `index.html`)
- `system.js` contains a link ~line 115: `📷 Display Screenshot Tool` pointing to `/screenshot.html`
- The `IWebContributor` interface supports `getParentTab()` which defaults to `nullptr` (top-level)
- `Application::begin()` at line 187 does `_webContributors.push_back(&_display)`

**Target state:**
- `U8G2DisplayModule` exposes THREE `IWebContributor*` entries, all wired into `Application::begin()`
- Contributor 1 (the main module): tab `"Display"`, no parent, order 30
- Contributor 2 (lightweight stub): tab `"Settings"`, parent `"Display"`, order 10
- Contributor 3 (lightweight stub): tab `"Screenshot"`, parent `"Display"`, order 20
- `appconfig.js` targets `#tab-settings` and fires on `tab-changed` → `tab-settings`
- `screenshot.js` targets `#tab-screenshot` but sub-pill tab ID becomes `tab-screenshot`
  (same ID, different parent — the app.js tab-changed event fires `tab-screenshot` already)
- `index.html` updated: rename `tab-app` div to `tab-display`, add `tab-settings` div
  (`#tab-screenshot` div already exists and can remain as-is)
- Screenshot link removed from `system.js`

---

## Implementation Checklist

### Step 1 — Read ALL required files before touching anything

Read each of these files before writing a single line:

1. `modules/U8G2Display/U8G2DisplayModule.hpp` (focus on `IWebContributor` section, lines ~340–365)
2. `modules/U8G2Display/U8G2DisplayModule.cpp` (focus on `registerRoutes()`, ~line 695+)
3. `include/interfaces/IWebContributor.hpp` (understand `getParentTab()` semantics)
4. `modules/Application/Application.cpp` (focus on lines 179–200 — contributor registration)
5. `web/portal/index.html` (full file — 114 lines)
6. `web/portal/app.js` (focus on `buildBottomNav()`, `activateTopTab()`, `loadScripts()`)
7. `web/portal/appconfig.js` (full file — 350 lines — understand `#tab-app` references)
8. `web/portal/screenshot.js` (full file — 203 lines — understand `tab-screenshot` event handling)
9. `web/portal/system.js` (focus on the screenshot link, ~line 115)
10. `test/web/tests/display.spec.ts` (full — understand `MOCK_TABS`, `navigateToDisplayTab()`)
11. `test/web/tests/navigation.spec.ts` (focus on `MOCK_TABS`)
12. `test/web/server.js` (focus on the `MOCK_TABS` / `/api/tabs` response)

---

### Step 2 — C++: Add sub-tab stub nested classes to `U8G2DisplayModule.hpp`

In `U8G2DisplayModule.hpp`, add two private nested struct `IWebContributor` stubs after the
existing IWebContributor section (around line 364):

```cpp
// -------------------------------------------------------------------------
// Sub-tab navigation stubs — injected as separate IWebContributor* entries
// in Application::begin(). They carry no routes; they exist only to inject
// "Settings" and "Screenshot" child tab entries into /api/tabs.
// -------------------------------------------------------------------------

struct SettingsTabStub : public IWebContributor {
    void registerRoutes(AsyncWebServer& /*server*/) override {}
    const char* getTabName()   const override { return "Settings";  }
    const char* getParentTab() const override { return "Display";   }
    int         getTabOrder()  const override { return 10;          }
};

struct ScreenshotTabStub : public IWebContributor {
    void registerRoutes(AsyncWebServer& /*server*/) override {}
    const char* getTabName()   const override { return "Screenshot"; }
    const char* getParentTab() const override { return "Display";    }
    int         getTabOrder()  const override { return 20;           }
};
```

Then add two member fields in the `private:` section of `U8G2DisplayModule`:
```cpp
SettingsTabStub    _settingsTabStub;
ScreenshotTabStub  _screenshotTabStub;
```

Also add two public getter methods so `Application` can register them:
```cpp
IWebContributor* settingsTabStub()    { return &_settingsTabStub;    }
IWebContributor* screenshotTabStub()  { return &_screenshotTabStub;  }
```

And update the existing `getTabName()` to return `"Display"`:
```cpp
const char* getTabName() const override { return "Display"; }
```

Update the doc comment above `getTabName()` accordingly (line ~354).

**House style:** Follow the existing doc block pattern for every new method you add.

---

### Step 3 — C++: Update `Application.cpp` contributor registration

In `modules/Application/Application.cpp`, after line 187 (`_webContributors.push_back(&_display);`),
add:
```cpp
_webContributors.push_back(_display.settingsTabStub());
_webContributors.push_back(_display.screenshotTabStub());
```

No other changes to `Application.cpp`.

---

### Step 4 — Update `web/portal/index.html`

1. Rename the `<div id="tab-app"` div to `<div id="tab-display"` (and update its comment)
2. Add a new `<div id="tab-settings" class="tab-content">` after `tab-display`
   (with a `<!-- Populated by appconfig.js -->` comment)
3. The existing `<div id="tab-screenshot" class="tab-content">` div **remains unchanged** — it
   already has the right ID for the sub-pill (the C++ stub returns `"Screenshot"` → app.js
   generates `tab-screenshot` from it)

After edits the relevant section should look like:
```html
      <!-- Display tab (top-level, app module) -->
      <div id="tab-display" class="tab-content">
      </div>
      <!-- Display sub-pill: Settings (populated by appconfig.js) -->
      <div id="tab-settings" class="tab-content">
      </div>
      <!-- Display sub-pill: Screenshot (populated by screenshot.js) -->
      <div id="tab-screenshot" class="tab-content">
        <!-- Populated by screenshot.js (conditional) -->
      </div>
```

Also update the bottom-nav comment at line ~80:
```html
    <!-- Framework tabs (WiFi, System) are hardcoded with SVG icons.          -->
    <!-- App tabs are injected dynamically by app.js.                         -->
```
No change needed there unless you want to clarify.

---

### Step 5 — Update `web/portal/appconfig.js`

All occurrences of `'tab-app'` and `"tab-app"` must become `'tab-settings'` / `"tab-settings"`.
There are exactly three references to check:
- Line 26: `var container = document.getElementById('tab-app');`
- Line 334: `if (e.detail === 'tab-app') {`
- Line 343: `var container = document.getElementById('tab-app');`

Update the header comment (lines 1–6) to reflect the new tab name ("Display → Settings sub-pill").

---

### Step 6 — Update `web/portal/screenshot.js`

The `screenshot.js` listens for `tab-changed` → `tab-screenshot`. Since the new C++ stub
returns `"Screenshot"` as the tab name, app.js will generate the tab ID `tab-screenshot` —
same as before. **No changes needed to screenshot.js ID references.**

However, update the header comment (lines 1–3) to say:
```
// web/portal/screenshot.js
// Display → Screenshot sub-pill — Live display framebuffer capture.
// Captures raw U8G2 buffer, renders to canvas, supports zoom and download.
```

---

### Step 7 — Update `web/portal/system.js`

Remove the `📷 Display Screenshot Tool` link block (approximately lines 114–117):
```javascript
      <a href="/screenshot.html" style="display:flex; ...">
        📷 Display Screenshot Tool
      </a>
```
This is a standalone link inside the system status card HTML string. Remove the entire `<a>` element.

---

### Step 8 — Update `test/web/server.js` mock tabs

In `server.js`, find the `MOCK_TABS` (or equivalent `/api/tabs` response) array.
Currently it contains `{ name: 'App', id: 'app', parent: null, order: 50 }`.

Replace with the new Display tab structure:
```javascript
  { name: 'Display',    id: 'display',    parent: null,      order: 30 },
  { name: 'Settings',   id: 'settings',   parent: 'display', order: 10 },
  { name: 'Screenshot', id: 'screenshot', parent: 'display', order: 20 },
```

> **Note on casing:** The server.js uses lowercase parent IDs (`'system'`, `'display'`).
> The spec files use title-case (`'System'`, `'Display'`). Match the existing casing in
> each file — don't normalize. Check the existing entries in each file to confirm.

---

### Step 9 — Update `test/web/tests/display.spec.ts`

This file has:
- `MOCK_TABS` containing `{ name: 'App', id: 'app', parent: null, order: 50 }`
- `navigateToDisplayTab()` clicking `#bottom-nav .tab-link[data-target="tab-app"]`
- `#display-brightness` waitFor

**Changes required:**
1. In `MOCK_TABS`, replace the App entry with the three Display entries:
   ```typescript
   { name: 'Display',    id: 'display',    parent: null,     order: 30 },
   { name: 'Settings',   id: 'settings',   parent: 'Display', order: 10 },
   { name: 'Screenshot', id: 'screenshot', parent: 'Display', order: 20 },
   ```
   > Match the casing used by the other entries in the file (title-case parents).

2. Update `navigateToDisplayTab()`:
   ```typescript
   async function navigateToDisplayTab(page: Page) {
     await page.locator('#bottom-nav .tab-link[data-target="tab-display"]').click();
     // Wait for the Settings sub-pill to render its content
     await page.locator('#display-brightness').waitFor({ state: 'visible' });
   }
   ```
   > NOTE: After clicking the Display tab, app.js will show the pill bar and auto-activate the
   > first sub-pill (Settings, order 10). The `#display-brightness` element lives in `#tab-settings`
   > which is populated by `appconfig.js` when the `tab-settings` tab-changed event fires.
   > The `waitFor` still works because it doesn't care which tab-content div the element is in.

3. Scan for any other hardcoded `tab-app` references in the file and update them.

---

### Step 10 — Update `test/web/tests/navigation.spec.ts`

Find the `MOCK_TABS` array. Currently it does NOT include the App tab. Check if the navigation
tests need to be updated to include the Display tab hierarchy. If the navigation tests test
"only WiFi and System appear in the bottom nav by default", they may need updating now that
`Display` is a new top-level tab. Update `MOCK_TABS` and any assertions accordingly.

---

### Step 11 — Check for and update any C++ unit tests

Check `test/test_unit/test_u8g2Display/` for any test asserting:
```cpp
EXPECT_STREQ("App", display.getTabName());
```
Update to:
```cpp
EXPECT_STREQ("Display", display.getTabName());
```

Also check whether there are tests for `getParentTab()` and `getTabOrder()` on `U8G2DisplayModule`
and update or add them as needed.

---

## Verification

### Step 12 — Build and run native unit tests

```bash
pio run -e native
```

Expected: All tests pass. Specifically confirm the display tab name test passes.

### Step 13 — Run Playwright web tests

```bash
cd test/web && npm test
```

Expected: All 61+ tests pass (or more if new ones are added).
If tests fail, diagnose and fix before reporting back.

---

## Completion Protocol

Once **both** verification steps above pass:

1. **Do NOT commit, branch, or push** — the PA handles git.
2. **Stop and send a message back to the PA** summarising:
   - What you changed (file list)
   - Test results (native unit + Playwright counts)
   - Any design decisions you made and why
   - Any concerns or edge cases you noticed
3. Wait for the PA's explicit approval before invoking `@distillery`.
4. Once approved, invoke the `@distillery` skill, passing `.agents/plugins/principal-architect-workspace/pr-23-display-tab/wo1-rename-and-subtabs/` as the target directory.
5. Append a status update to this WORKORDER.md and terminate your session.
