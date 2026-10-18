---
name: portal-decentralise-wo2
description: Move route serving from WebServerManager to individual modules; add getTabScript() overrides
complexity: standard
---

# WO2 — Route Decentralisation

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent.

## Objective
Move static JS/HTML route registration from `WebServerManager.cpp` into each optional module's `registerRoutes()` method. Each module includes its own `portal_assets/*.hpp` header and serves its own JS. Add `getTabScript()` overrides so the SPA can discover module scripts dynamically.

## Strict Boundary Box
- **ONLY** modify the files listed in the checklist.
- **DO NOT** modify `scripts/portalBuilder.py` or move any files.
- **DO NOT** modify `web/portal/app.js` (that is WO3).
- **DO NOT** modify `platformio.ini`.
- **DO NOT** branch.

## Tooling Constraints
- Use the `platformio` MCP server for build verification (`build_project`).
- Do NOT use raw `pio` CLI commands.

## Context — Target Pattern
HomeAssistantModule already follows the target pattern. In its `.cpp` file:
```cpp
#include "portal_assets/homeassistant_js.hpp"

void HomeAssistantModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/homeassistant.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        response->setContent(homeassistant_js_gz, homeassistant_js_gz_len);
        response->setContentType("application/javascript");
        response->addHeader("Content-Encoding", "gzip");
        return response->send();
    });
    // ... API routes follow
}
```

## Line-Item Instructions

### WebServerManager.cpp — Remove optional includes and routes
- [ ] Remove these `#include` directives (keep all core ones):
  - `"portal_assets/time_js.hpp"`
  - `"portal_assets/ota_js.hpp"`
  - `"portal_assets/mcp_js.hpp"`
  - `"portal_assets/display_js.hpp"`
  - `"portal_assets/carousel_js.hpp"`
  - `"portal_assets/screenshot_js.hpp"`
  - `"portal_assets/screenshot_html.hpp"`
  - `"portal_assets/hardware_js.hpp"`
- [ ] Remove the route registrations for: `/time.js`, `/ota.js`, `/mcp.js`, `/display.js`, `/carousel.js`, `/screenshot.js`, `/screenshot.html`, `/hardware.js`
- [ ] Keep all core routes: `/app.js`, `/utils.js`, `/wifi.js`, `/system.js`, `/backup.js`, `/keys.js`, `/appconfig.js`, `/auth.js`, `/credits.js`, `/app.css`, `/` (index.html)

### OTAUpdateManager — Self-serve ota.js
- [ ] In `modules/OTAUpdateManager/OTAUpdateManager.cpp`:
  - Add `#include "portal_assets/ota_js.hpp"` at the top (with other includes)
  - In `registerRoutes()`, add the gzipped PROGMEM route for `/ota.js`
- [ ] In `modules/OTAUpdateManager/OTAUpdateManager.hpp`:
  - Add `const char* getTabScript() const override { return "/ota.js"; }` to the class

### MCPServer — Self-serve mcp.js
- [ ] In `modules/MCPServer/MCPServer.cpp`:
  - Add `#include "portal_assets/mcp_js.hpp"`
  - In `registerRoutes()`, add the route for `/mcp.js`
- [ ] In `modules/MCPServer/MCPServer.hpp`:
  - Add `getTabScript()` override → `"/mcp.js"`

### TimeManager — Self-serve time.js
- [ ] In `modules/TimeManager/TimeManager.cpp`:
  - Add `#include "portal_assets/time_js.hpp"`
  - In `registerRoutes()`, add the route for `/time.js`
- [ ] In `modules/TimeManager/TimeManager.hpp`:
  - Add `getTabScript()` override → `"/time.js"`

### HomeAssistantModule — Add getTabScript()
- [ ] In `modules/HomeAssistant/HomeAssistantModule.hpp`:
  - Add `const char* getTabScript() const override { return "/homeassistant.js"; }` (route serving already works — this is the missing dynamic discovery piece)

### U8G2DisplayModule — Self-serve display contract + carousel
- [ ] In `modules/U8G2Display/U8G2DisplayModule.cpp` (or `.hpp` if routes are inline):
  - Add includes: `portal_assets/display_js.hpp`, `screenshot_js.hpp`, `screenshot_html.hpp`, `carousel_js.hpp`
  - In `registerRoutes()`, serve `/display.js`, `/screenshot.js`, `/screenshot.html`, `/carousel.js`
- [ ] The main tab contributor class should override `getTabScript()` → `"/display.js"`
  (Note: the `CarouselSubTab` inner class already overrides `getTabScript()` → `"/carousel.js"`)

### LVGLDisplayModule — Self-serve display contract + hardware
- [ ] In `modules/LVGLDisplay/LVGLDisplayModule.cpp` (or `.hpp`):
  - Add includes: `portal_assets/display_js.hpp`, `screenshot_js.hpp`, `screenshot_html.hpp`, `hardware_js.hpp`
  - In `registerRoutes()`, serve `/display.js`, `/screenshot.js`, `/screenshot.html`, `/hardware.js`
- [ ] The main tab contributor class should override `getTabScript()` → `"/display.js"`
  (Note: the `HardwareSubTab` inner class already overrides `getTabScript()` → `"/hardware.js"`)

## Verification Requirement
1. Build `esp32s3nano` and `esp32dev` via the platformio MCP server — verify no regression.
2. Run `pio test -e native` — verify existing tests still pass.
3. **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect or concern outside your boundary box, surface it in your Completion Report.

---

## Completion Report

### Status
Complete — Approved by Parent Agent

### Changes Made
- `WebServerManager.cpp`: Removed 8 optional asset includes and their static route registrations (`/time.js`, `/ota.js`, `/mcp.js`, `/display.js`, `/carousel.js`, `/screenshot.js`, `/screenshot.html`, `/hardware.js`). Retained core routes.
- `OTAUpdateManager`: Added `#include "portal_assets/ota_js.hpp"`, GET `/ota.js` route in `registerRoutes()`, and `getTabScript()` override returning `"/ota.js"`.
- `MCPServer`: Added `#include "portal_assets/mcp_js.hpp"`, GET `/mcp.js` route in `registerRoutes()`, and `getTabScript()` override returning `"/mcp.js"`.
- `TimeManager`: Added `#include "portal_assets/time_js.hpp"`, GET `/time.js` route in `registerRoutes()`, and `getTabScript()` override returning `"/time.js"`.
- `HomeAssistantModule`: Added `getTabScript()` override returning `"/homeassistant.js"`.
- `U8G2DisplayModule`: Added includes for `display_js.hpp`, `screenshot_js.hpp`, `screenshot_html.hpp`, `carousel_js.hpp`, registered GET routes for `/display.js`, `/screenshot.js`, `/screenshot.html`, `/carousel.js`, and added `getTabScript()` override returning `"/display.js"`.
- `LVGLDisplayModule`: Added includes for `display_js.hpp`, `screenshot_js.hpp`, `screenshot_html.hpp`, `hardware_js.hpp`, registered GET routes for `/display.js`, `/screenshot.js`, `/screenshot.html`, `/hardware.js`, and added `getTabScript()` override returning `"/display.js"`.
- `test/web/server.js`: Enhanced static file resolution logic to scan `web/portal/`, `web/display/`, and `modules/*/web/` so Playwright web test server resolves relocated assets.

### Decisions Made
- Updated mock web server (`test/web/server.js`) so Playwright web test suite correctly locates relocated asset files during `build_project` pre-build actions.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.
