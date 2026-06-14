---
id: pr-23
title: "Module-Owned Display Tab + App Tab Separation"
status: "✅ COMPLETE — WO1 merged `c1bde82`, 525/525 native + 61/61 Playwright"
branch: "pr-23-display-tab"
depends_on: []
---

# PR-23: Module-Owned Display Tab + App Tab Separation

## Goal

Cleanly separate display-hardware settings from application-level configuration in
the web portal. The display module owns a `"Display"` top-level tab (with
`"Settings"` and `"Screenshot"` sub-pills). The application owns an `"App"` top-level
tab for app-level configuration (currently: clock font size).

This satisfies two principles:
1. **Module ownership:** if U8G2 is swapped for another display driver, its successor
   registers its own `"Display"` tab — the framework shell stays generic.
2. **App-level separation:** the `"App"` tab is a first-class application concern, not
   a side-effect of a hardware module's `getTabName()`.

---

## Current State (pre-PR-23)

| Concern | Current location | Problem |
|---------|-----------------|---------|
| Display hardware settings (brightness, flip, screensaver, dim/blank) | `appconfig.js` → `#tab-app` (top-level `"App"` tab, owned by U8G2DisplayModule) | Hardware module owns application-level tab name; no sub-navigation |
| Clock font size | Same `appconfig.js` → `#tab-app` | Should be in an app-owned tab |
| Screenshot tool | `screenshot.js` → `#tab-screenshot` (orphan div) | Not accessible via portal nav; only via link in system.js or standalone `/screenshot.html` |
| Screenshot link | `system.js` ~line 115 | Orphaned link once screenshot gets a proper home |
| App tab contributor | None — `U8G2DisplayModule::getTabName()` returns `"App"` | Wrong owner |

---

## Target State

### Navigation hierarchy

```
Bottom nav: [ WiFi ] [ Display ] [ App ] [ System ]
                         │
                   [ Settings ] [ Screenshot ]
```

| Tab | Level | Owner | Content |
|-----|-------|-------|---------|
| `"App"` | Top-level (bottom nav) | `AppTabContributor` (new class, owned by `Application`) | Clock font size (app-level config) |
| `"Display"` | Top-level (bottom nav) | `U8G2DisplayModule` | *(parent stub — no content)* |
| `"Settings"` | Sub-pill of Display | U8G2 nested stub | Brightness, flip, screensaver enable, dim/blank timeouts, wake/sleep |
| `"Screenshot"` | Sub-pill of Display | U8G2 nested stub | Live framebuffer capture |

### Key file changes

| File | Change |
|------|--------|
| `modules/U8G2Display/U8G2DisplayModule.hpp/.cpp` | `getTabName()` → `"Display"`, add `SettingsTabStub` + `ScreenshotTabStub` nested structs, expose getter methods |
| `modules/Application/Application.hpp/.cpp` | Add `AppTabContributor` member, register all three display stubs + app stub as web contributors |
| `web/portal/appconfig.js` | Split into `appconfig.js` (clock font → `#tab-app`) and `display.js` (hardware settings → `#tab-settings`) |
| `web/portal/screenshot.js` | Update header comment; ID `tab-screenshot` unchanged (generated from `"Screenshot"` → `tab-screenshot`); remove system.js link |
| `web/portal/system.js` | Remove `📷 Display Screenshot Tool` link |
| `web/portal/index.html` | Rename `#tab-app` to `#tab-display`, add `#tab-settings` div; keep `#tab-screenshot`; add new `#tab-app` div for `AppTabContributor` |
| `web/portal/app.js` | Add `display.js` to `loadScripts()` list |
| `test/web/server.js` | Update `/api/tabs` mock to include Display hierarchy + App tab |
| `test/web/tests/display.spec.ts` | Update MOCK_TABS, `navigateToDisplayTab()`, locators |
| `test/web/tests/navigation.spec.ts` | Update MOCK_TABS |
| Unit tests | Update `getTabName()` assertion on U8G2DisplayModule |

---

## Architecture Decision: AppTabContributor

`Application` does not currently implement `IWebContributor`. Rather than burdening
the `Application` class with the interface, a **lightweight standalone struct** is
the cleanest approach:

```cpp
// In Application.hpp (private section):
struct AppTabContributor : public IWebContributor {
    void registerRoutes(AsyncWebServer& /*server*/) override {}  // JS handles everything
    const char* getTabName()  const override { return "App";  }
    const char* getTabOrder() const override { return 20;     }  // before System (100), after Display (30)? TBC
};
AppTabContributor _appTabContributor;
```

`Application::begin()` then adds: `_webContributors.push_back(&_appTabContributor);`

All route handling for app-level config remains in JS only (REST calls via existing
`/api/display/clockFontSize` for now; future app modules add their own routes).

> **Tab order for "App":** Display=30, App=40(?), System=100. Worker should confirm
> a sensible order relative to the existing tabs (WiFi=0, System=100).

---

## Architecture Decision: appconfig.js Split

`appconfig.js` currently contains both display-hardware controls and the clock font
size selector in one 350-line file. These must be separated:

- **`display.js`** (new file) — brightness, flip, screensaver, dim/blank timeouts,
  wake/sleep. Targets `#tab-settings`. Fires on `tab-changed` → `tab-settings`.
- **`appconfig.js`** (trimmed) — clock font size only. Targets `#tab-app`.
  Fires on `tab-changed` → `tab-app`.

Both files loaded in `app.js`'s `loadScripts()` array.

---

## Work Order Ledger

- **[x] [wo1-display-tab](wo1-display-tab/WORKORDER.md):** Full implementation — C++ stubs, AppTabContributor, JS split, index.html, system.js link removal, all tests — `c1bde82`

---

## Sequencing

Single WO — all changes are interdependent (splitting `appconfig.js` must happen
alongside the tab ID changes; tests must be updated atomically). Parallelising
would require touching the same files simultaneously with guaranteed conflicts.

## Pre-Reading for Workers

1. `modules/U8G2Display/U8G2DisplayModule.hpp` — IWebContributor section (~lines 340–365)
2. `modules/U8G2Display/U8G2DisplayModule.cpp` — `registerRoutes()`
3. `include/interfaces/IWebContributor.hpp` — interface definition
4. `modules/Application/Application.hpp` — private member section
5. `modules/Application/Application.cpp` — contributor registration (~lines 179–200)
6. `web/portal/index.html` — full file
7. `web/portal/app.js` — `buildBottomNav()`, `loadScripts()`
8. `web/portal/appconfig.js` — full file (350 lines)
9. `web/portal/screenshot.js` — full file
10. `web/portal/system.js` — screenshot link (~line 115)
11. `test/web/tests/display.spec.ts` — MOCK_TABS, `navigateToDisplayTab()`
12. `test/web/tests/navigation.spec.ts` — MOCK_TABS
13. `test/web/server.js` — `/api/tabs` mock
14. `test/test_unit/test_u8g2Display/` — any `getTabName()` assertions
