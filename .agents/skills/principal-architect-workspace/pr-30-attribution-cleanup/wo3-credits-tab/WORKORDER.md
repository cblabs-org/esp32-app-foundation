---
name: pr-30-attribution-cleanup-wo3
description: Handoff instructions for Work Order 3 — Add a Credits tab to the web portal
complexity: standard
---

# WO3: Credits Tab

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

## Objective

Add a new top-level "Credits" tab to the web portal that acknowledges all
open-source libraries, the ESP-IDF platform, and the project itself. The tab
must use a stylised © (copyright) SVG icon in the bottom navigation bar,
consistent with the existing Feather-icon stroke style. The tab content is
purely client-side — no API endpoint is required.

## Architecture Overview

The portal's tab system works as follows:

1. **`/api/tabs`** returns a JSON array of tabs. Framework tabs (WiFi, System,
   Security) are hardcoded in `index.html`. Module-contributed tabs come from
   `IWebContributor` implementors. Additionally, WebServerManager manually
   injects tabs that are not IWebContributor-owned (e.g. the "Status" sub-pill).

2. **`app.js`** reads `/api/tabs`, builds bottom-nav links for any tab not
   already hardcoded in HTML, creates `tab-content` divs dynamically, and loads
   fragment JS files that populate their container div.

3. **`portalBuilder.py`** auto-discovers all files in `web/portal/` and generates
   gzip PROGMEM headers in `include/portal_assets/`. No manual registration
   needed — just drop the file in `web/portal/`.

4. **`WebServerManager.cpp`** serves each portal asset via a static route. New
   files need a route added, an `#include` for the generated header, and
   registration in the `loadScripts()` array in `app.js`.

## Strict Boundary Box

- ✅ Create `web/portal/credits.js` (new file)
- ✅ Modify `web/portal/index.html` — add Credits tab link in bottom-nav and
  `tab-credits` content div
- ✅ Modify `web/portal/app.js` — add `credits.js` to the `loadScripts()` array
- ✅ Modify `modules/WebServer/WebServerManager.cpp` — add `#include` for
  `portal_assets/credits_js.hpp` and add the `/credits.js` static route
- ❌ Do NOT create any API endpoints — Credits is a static, client-side tab
- ❌ Do NOT modify any other files
- ❌ Do NOT create new branches — work on whatever branch is checked out
- ❌ Do NOT make the tab an IWebContributor — it's a framework-owned static tab
- ✅ DO follow the existing code style: IIFE pattern in JS, Feather-icon SVG style

## Checklist

### 1. Create `web/portal/credits.js`

Create a new file following the existing fragment pattern (IIFE, `container`
reference, `innerHTML`). The file should:

- Target `document.getElementById('tab-credits')`
- Render a styled card listing the open-source dependencies, grouped by category
- Each entry should include: library name, author/org, licence type, and a
  clickable URL to the project homepage

**Dependencies to credit:**

| Library | Author/Org | Licence | URL |
|---|---|---|---|
| **ESP-IDF** | Espressif Systems | Apache-2.0 | https://github.com/espressif/esp-idf |
| **Arduino-ESP32** | Espressif Systems | LGPL-2.1 | https://github.com/espressif/arduino-esp32 |
| **ArduinoJson** | Benoît Blanchon | MIT | https://github.com/bblanchon/ArduinoJson |
| **PsychicHttp** | Hoeken / Zach Hoeken | LGPL-2.1 | https://github.com/hoeken/PsychicHttp |
| **U8g2** | Oliver Kraus (olikraus) | BSD-2-Clause | https://github.com/olikraus/u8g2 |
| **mbedTLS** | TrustedFirmware.org (ARM) | Apache-2.0 | https://github.com/Mbed-TLS/mbedtls |
| **GoogleTest** | Google | BSD-3-Clause | https://github.com/google/googletest |

**Styling:**
- Use existing portal CSS classes: `.card`, `.card-title`
- Each library entry should be a row showing name (bold), licence badge, and a
  link icon that opens the URL in a new tab
- Add a brief intro paragraph: "This firmware is built with the following
  open-source components. We gratefully acknowledge the work of their authors."
- At the bottom, include a "Built with" line crediting
  **ESP32 App Foundation** by CB Labs with a link to the GitHub repo

**Example structure:**
```javascript
(function() {
  var container = document.getElementById('tab-credits');
  if (!container) return;

  var libs = [
    { name: 'ESP-IDF', author: 'Espressif Systems', licence: 'Apache-2.0', url: '...' },
    // ... all entries
  ];

  var html = '<div class="card"><h3 class="card-title">Open Source Credits</h3>';
  html += '<p style="...">This firmware is built with the following open-source ...';
  // ... render each lib as a row
  html += '</div>';
  container.innerHTML = html;
})();
```

### 2. Add tab-credits content div to `index.html`

Add a Credits tab content div inside the `.tab-content-container`, **before**
the closing `</div>` of the container (after the OTA tab div). Place it after
the existing OTA tab div:

```html
      <div id="tab-credits" class="tab-content">
        <!-- Populated by credits.js -->
      </div>
```

### 3. Add Credits nav link to bottom-nav in `index.html`

Add a new `<a>` element to the bottom-nav, **after** the System tab link and
**before** the comment about app-contributed tabs. Use a stylised © icon in
the Feather stroke style (circle + "c" path — see Icon Design below):

```html
      <a href="#" class="tab-link" data-target="tab-credits">
        <span class="icon">
          <!-- Copyright / Credits SVG (Feather-icon style) -->
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"
               stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <circle cx="12" cy="12" r="10"/>
            <path d="M14.83 14.83a4 4 0 1 1 0-5.66"/>
          </svg>
        </span>
        CREDITS
      </a>
```

**Icon Design Notes:**
- The icon is a circle with an opening arc — visually reads as a "©" or "CC"
  symbol in the Feather-icon minimalist stroke style
- `circle cx=12 cy=12 r=10` — outer ring (matches Feather's standard 24×24 grid)
- `path d="M14.83 14.83a4 4 0 1 1 0-5.66"` — an arc that forms the "C" shape
  inside the circle, exactly like the Feather `at-sign` or `copyright` concept
- This is stroke-only with no fill, matching WiFi/Security/System icon style

### 4. Add `credits.js` to the `loadScripts()` array in `app.js`

In `web/portal/app.js`, find the `loadScripts` function (around line 441) and
add `'credits.js'` to the scripts array. Place it after `'auth.js'`:

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
      'display.js',
      'appconfig.js',
      'auth.js',
      'credits.js'     // ← ADD THIS
    ];
```

### 5. Add `#include` and route in `WebServerManager.cpp`

**5a. Add the include** (inside the `#ifndef NATIVE_BUILD` portal assets block,
after the `auth_js.hpp` include, around line 56):

```cpp
#include "portal_assets/credits_js.hpp"
```

**5b. Add the static route** (after the existing `auth.js` route, around line
273, inside the `#ifndef NATIVE_BUILD` block):

```cpp
    activeServer->on("/credits.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        response->setContent(credits_js_gz, credits_js_gz_len);
        response->setContentType("application/javascript");
        response->addHeader("Content-Encoding", "gzip");
        return response->send();
    });
```

### 6. Register Credits in the `/api/tabs` response

In `WebServerManager.cpp`, after the manual "Status" sub-pill injection
(around line 309), add a manual tab entry for Credits so app.js knows about it:

```cpp
    // Inject the "Credits" top-level tab — framework-owned, no IWebContributor
    JsonObject creditsTab = tabsArr.add<JsonObject>();
    creditsTab["name"]  = "Credits";
    creditsTab["id"]    = "Credits";
    creditsTab["order"] = 99;  // Last tab
```

**WAIT** — actually, since Credits is **hardcoded in index.html** (like WiFi,
System, Security), it does NOT need a `/api/tabs` entry and app.js will skip
it during `buildBottomNav()` (because `tabId === 'tab-credits'` will match a
hardcoded link). **Do NOT add a tabs entry.** The tab link is already in the
HTML.

**Correction:** Skip this step entirely.

## Verification

1. `pio run -e esp32dev` — firmware builds (validates the new include + route)
2. `pio test -e native` — all native tests pass (credits.js is client-side only,
   no server logic to test)
3. Manual: load the portal in a browser, confirm the Credits tab appears in the
   bottom nav with the © icon, and displays the dependency list with working links
