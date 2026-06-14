# ADR: Tab Ownership — Module vs Application

## Decision

`U8G2DisplayModule` owns the `"Display"` top-level bottom-nav tab. `Application` owns the `"App"` tab via a new `AppTabContributor` inner struct.

## Context

Prior to PR-23, `U8G2DisplayModule::getTabName()` returned `"App"` — it was piggy-backing on the application-level namespace. This violated the principle that a hardware module should not claim the application's primary config namespace.

The user explicitly corrected this during planning:
> "The display module should not register the App Tab — this should be registered by the application itself, it is not a module it's where we put the config for the application."

## Consequences

### Tab hierarchy in `/api/tabs` response:
```
WiFi       (order:10, framework, hardcoded in HTML)
App        (order:30, AppTabContributor in Application)
Display    (order:50, U8G2DisplayModule::getTabName())
  Settings   (order:10, SettingsTabStub — no routes, nav-only)
  Screenshot (order:20, ScreenshotTabStub — no routes, nav-only)
System     (order:100, framework, hardcoded in HTML)
```

### Key Structural Rules:
- Sub-pill stubs (`SettingsTabStub`, `ScreenshotTabStub`) carry **no REST routes** — `registerRoutes()` is a no-op. All `/api/display/*` routes remain on the main module.
- Stubs are `private` inner structs of `U8G2DisplayModule`, exposed via `settingsTabStub()` / `screenshotTabStub()` accessor methods, registered in `Application::begin()`.
- `AppTabContributor` is a `private` inner struct of `Application`, also registered in `Application::begin()`.

### IWebContributor Interface:
```cpp
virtual const char* getParentTab() const { return nullptr; }  // nullptr = top-level
virtual int         getTabOrder()  const { return 100; }
virtual const char* getTabName()   const { return nullptr; }  // nullptr = no tab
virtual void        registerRoutes(AsyncWebServer&) {}
```

## Tab Order Iteration
Initial orders were Display(30), App(40). User requested Display after App, so final orders are App(30), Display(50).
