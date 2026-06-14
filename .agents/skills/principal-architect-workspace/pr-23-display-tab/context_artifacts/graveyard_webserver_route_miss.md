# Graveyard: Missing /display.js Route in WebServerManager

## Symptom

Display → Settings sub-pill appeared with its pill button visible in the nav bar, but the content area was completely empty. No JavaScript errors in the browser console.

## Root Cause

The worker agent added `display.js` to three of four required places:
1. ✅ Created `web/portal/display.js`
2. ✅ Added `'display.js'` to `loadScripts()` in `app.js`
3. ❌ **Did NOT add** `#include "portal_assets/display_js.hpp"` to `WebServerManager.cpp`
4. ❌ **Did NOT add** `_server.on("/display.js", ...)` route handler to `WebServerManager.cpp`

The existing pattern (every other portal JS file has both include + route) was not followed for the new file.

## Failure Chain

1. Browser requested `/display.js` on page load via `loadScripts()`
2. Server returned 404 (no handler registered)
3. `app.js` `loadScripts()` gracefully continued past the error (no blocking, no console error surfaced prominently)
4. `display.js` IIFE never executed → `tab-changed: 'tab-settings'` listener never registered
5. User clicked Display tab → `activateSubPill('tab-settings', 'Display')` fired correctly
6. `showContentDiv('tab-settings')` showed the div (it exists in HTML, has `active` class)
7. `tab-changed: 'tab-settings'` was dispatched — but no listener caught it
8. `#tab-settings` div remained empty

## Fix

Commit `3e35277`:
```cpp
// WebServerManager.cpp
#include "portal_assets/display_js.hpp"   // ← added

_server.on("/display.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse *response = request->beginResponse(
        200, "application/javascript", display_js_gz, display_js_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
});
```

## Prevention

Any future agent adding a new portal JS file MUST update both:
- `WebServerManager.cpp` includes block (lines ~33–47)
- `WebServerManager::begin()` route registration block (lines ~114–185)

See `adr_js_split_pattern.md` for the complete registration requirement.
