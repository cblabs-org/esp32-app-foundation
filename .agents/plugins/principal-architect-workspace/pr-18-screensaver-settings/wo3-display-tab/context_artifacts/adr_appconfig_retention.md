# ADR: Retaining `appconfig.js` Instead of Creating `display.js`

## Context

WO3 specified creating a new `web/portal/display.js` file for the Display tab. However, the ESP32 web portal uses a PROGMEM-based asset serving pipeline:

1. `scripts/portalBuilder.py` scans `web/portal/` and generates gzipped C++ header files in `include/portal_assets/`
2. `WebServerManager.cpp` has **hardcoded** `#include` and route registrations for each file (e.g., line 180: `_server.on("/appconfig.js", ...)`)
3. Creating a new `display.js` would generate `display_js.hpp` via the build script, but no route exists in the C++ to serve it

## Decision

Expand `appconfig.js` in-place with all Display tab functionality instead of creating a separate `display.js`. The file retains its name but its content is completely replaced — from a minimal clock-font-only module to a comprehensive Display Settings tab.

## Rationale

- **No C++ changes needed** — `WebServerManager.cpp` already includes and serves `appconfig_js.hpp`
- **Tab ID compatibility** — `U8G2DisplayModule::getTabName()` returns `"App"`, so the tab container is `tab-app`. The display module naturally hooks into this.
- **Build pipeline compatibility** — `portalBuilder.py` regenerates `appconfig_js.hpp` with the new content automatically
- **Zero regression risk** — no changes to the serving pipeline

## Consequences

- The file is named `appconfig.js` but contains Display tab code — a semantic mismatch
- Future work to rename properly would require:
  1. `WebServerManager.cpp`: Add `#include "portal_assets/display_js.hpp"` + route for `/display.js`
  2. `WebServerManager.cpp`: Remove `appconfig_js.hpp` include + route
  3. `U8G2DisplayModule.hpp`: Change `getTabName()` from `"App"` to `"Display"`
  4. `app.js`: Update the script loading array

## Status

Approved by Principal Architect (2026-06-04). The retention is correct given boundary constraints.
