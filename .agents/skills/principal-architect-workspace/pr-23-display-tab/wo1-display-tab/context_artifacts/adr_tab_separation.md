# ADR: Tab Separation Architecture

## Decision: Two-Level Contributor Pattern (stubs + main)

### Context

`U8G2DisplayModule` previously registered as a single top-level `"App"` tab. The requirement was to:
1. Give Display its own top-level tab with sub-pills (Settings, Screenshot)
2. Give Application its own `"App"` top-level tab for app-level config

### Options Considered

**Option A**: Create separate full `IWebContributor` classes for each sub-pill, each capable of registering routes.

**Option B (chosen)**: Use private nested `struct` stubs inside `U8G2DisplayModule` and `Application`. The stubs implement `IWebContributor` but have empty `registerRoutes()`. Only the main module registers the actual REST routes.

### Why Option B

- Minimal change to `registerRoutes()` — all existing REST routes remain on the main module
- No new files needed for stubs; they're private implementation details
- `Application::begin()` owns the registration order, maintaining the existing contribution pattern
- Stubs are allocated as value members (not heap), so no ownership complexity

### AppTabContributor Location

`AppTabContributor` lives inside `Application` because:
- The `"App"` tab is not owned by any hardware module
- Application is the natural owner of app-level configuration
- Avoids creating a new module just for one top-level nav entry

### Tab Ordering

| Tab | Order | Parent |
|---|---|---|
| WiFi | 10 | null |
| Display | 30 | null |
| Settings | 10 | Display |
| Screenshot | 20 | Display |
| App | 40 | null |
| System | 100 | null |

Display (30) appears before App (40) in the bottom nav, matching the visual hierarchy.

### JS Split Strategy

- `display.js` → hardware controls (`#tab-settings`) — brightness, flip, screensaver
- `appconfig.js` → app config (`#tab-app`) — clock font size only
- Both listen on `tab-changed` events for their respective tab IDs
- All REST endpoints (`/api/display/*`) remain unchanged — only the UI container changed
