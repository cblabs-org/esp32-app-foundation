# Portal Tab JavaScript Structure

Because the web portal does not load external HTML fragments, each tab's entire user interface is constructed dynamically using JavaScript. Each tab module provides a script that writes its own markup and handles events.

## Delivery: gzip+PROGMEM pipeline (canonical)

**The canonical way to deliver a tab script is the framework's gzip+PROGMEM asset pipeline** — the same pipeline the framework uses for its own scripts (`app.js`, `mcp.js`, `hardware.js`, …). This applies equally to framework modules **and to consuming apps**.

Why it is canonical: `PsychicHttp::send(body)` copies the entire response body into **internal DRAM** socket buffers. For a non-trivial tab script (an 18 KB script is routine) this can drop `heap_min_free` to near-OOM, take many seconds to deliver, and cause intermittent `0 bytes received` browser timeouts. The gzip pipeline serves the script as a gzip-compressed `PROGMEM` byte array straight from flash — **zero DRAM impact** — via `setContent()` + `Content-Encoding: gzip`. (See issue #111 for the failure this pipeline prevents.)

### How it works

1. **Put the JS in a `web/` directory.**
   - A framework module: `modules/<Name>/web/<name>.js`
   - A consuming app: `<projectDir>/web/<name>.js` (the build script scans the consuming project's `web/` dir automatically)
2. **The build compresses it.** `scripts/portalBuilder.py` runs at build time, gzip-compresses every scanned asset, and emits a header `portal_assets/<name>_js.hpp` exposing three symbols:
   - `const uint8_t <name>_js_gz[] PROGMEM` — the compressed bytes
   - `const size_t <name>_js_gz_len` — the compressed length
   - `const char <name>_js_gz_etag[]` — a build-time HTTP ETag (CRC32 of the compressed bytes) for caching

   The header lands in a **per-project** include dir so two consumers sharing one framework checkout never overwrite each other's output:
   - framework-sourced assets → `<framework>/include/portal_assets/` (reached via a consumer's `-I <framework>/include`)
   - **consumer-sourced assets → `<yourProject>/include/portal_assets/`** — add `-I include` to your `build_flags` so `#include "portal_assets/<name>_js.hpp"` resolves.

   > Basename collisions across all scanned directories are a hard build error (`sys.exit(1)`), so give your asset a unique filename.
3. **Serve it from `registerRoutes()`** using the shared helper `serveGzipAsset()` (from `web/serveGzipAsset.hpp`), passing the request (for conditional GET), the `_gz`/`_gz_len`/`_gz_etag` triple, and the MIME type:

```cpp
#include "web/serveGzipAsset.hpp"
#include "portal_assets/weather_js.hpp"   // generated from web/weather.js

void WeatherModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/weather.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response,
                              weather_js_gz, weather_js_gz_len, weather_js_gz_etag,
                              "application/javascript");
    });
    // ... your /api/... routes ...
}
```

4. **Declare the route** by overriding `getTabScript()`:

```cpp
const char* getTabScript() const override { return "/weather.js"; }
```

The sample app's `WeatherModule` (`sample-app/web/weather.js` + `-I include`) is the reference implementation of this pattern end-to-end.

> [!NOTE]
> **Caching is handled for you.** `serveGzipAsset()` sends the ETag and `Cache-Control: no-cache`, and answers a matching `If-None-Match` with a bodiless `304 Not Modified`. Because portal assets sit at a fixed URL but change across OTA updates, this is deliberately *not* an `immutable`/long-`max-age` policy (which would serve a stale script after an update) — the browser revalidates cheaply and only re-downloads when the content actually changes.

## Delivery: raw-string literal (secondary — small fragments only)

For a **small** inline fragment (roughly **< 1 KB** — a few controls, no large markup blocks) the DRAM cost is negligible and the gzip pipeline's build-time indirection isn't worth it. In that case only, it is acceptable to serve the script as a C++ raw-string literal:

```cpp
server.on("/tiny.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
    const char* js = R"rawliteral(
(function() {
    // small tab-scoped logic only
})();
    )rawliteral";
    return response->send(200, "application/javascript", js);
});
```

> [!WARNING]
> **Do not use the raw-string pattern for non-trivial scripts.** `response->send(body)` copies the whole string into internal DRAM. Anything beyond ~1 KB should use the gzip pipeline above. If you are unsure, use the gzip pipeline — it is always safe.

The remaining conventions below (the `tab-changed` contract, IIFE wrapper, ID scoping, security, design system, save policy) apply identically regardless of which delivery mechanism you choose.

## The `tab-changed` Event Contract

The frontend framework (`app.js`) creates a container `div` for each tab inside the DOM, and dispatches a `tab-changed` event whenever the active tab changes. Tab scripts must listen to this event to render their UI and bind event listeners.

* **Target Container ID**: Derives from the tab display name: `'tab-' + name.toLowerCase().replace(/\s+/g, '-')`.
* **Event Listener**: The script must filter events by checking `e.detail` against its target container ID.
* **Initialization Guard**: Use an initialization flag to ensure the HTML skeleton is only injected once.

```javascript
window.addEventListener('tab-changed', function(e) {
    if (e.detail !== 'tab-weather') return; // Filter events

    var container = document.getElementById('tab-weather');
    if (!container) return;

    if (!initialized) {
        initialized = true;
        // 1. Inject DOM markup
        container.innerHTML = `...`;

        // 2. Query elements and bind listeners
        var saveBtn = document.getElementById('weather-save-btn');
        // ...
    }
});
```

## IIFE Wrapper Guidance

To prevent variables, constants, and helper functions from leaking into the global scope and colliding with other tabs, all tab JavaScript code **MUST** be enclosed within an Immediately Invoked Function Expression (IIFE):

```javascript
(function() {
    // Tab-scoped variables and functions
    var initialized = false;
    // ...
})();
```

## ID Scoping & Prefixing Rules

Although each tab renders into its own container, all tabs share the same DOM document. To avoid collision bugs and styling issues:
1. **Prefix all element IDs**: Prefix IDs with the tab's identifier (e.g., use `id="weather-save-btn"` instead of `id="save-btn"`).
2. **Limit query selectors**: When querying elements, search within the tab's container instead of `document` (e.g., use `container.querySelector('#weather-save-btn')` or `document.getElementById('weather-save-btn')` with a prefixed ID).

## Security & Authentication Note

> [!WARNING]
> Tab scripts are served **unauthenticated**, similar to other static assets (e.g., `index.html`, `app.js`).
> Do **NOT** embed credentials, API keys, or private system data directly inside the raw JavaScript string.
> All sensitive configuration and state data must be fetched at runtime via authenticated `/api/` REST endpoints (using `window.apiFetch()`).

## Design System Compliance

All portal tab markup **must** use the project's design system tokens and components rather than hard-coded colours, spacing, or ad-hoc CSS classes. The design system provides ready-made components (Drawer, PillBar, InfoRow, ToggleSwitch, Input, Select, etc.) and a module-tab scaffold template. See `../../../framework-design-system/readme.md` for the full reference and `../../../framework-design-system/templates/module-tab/` for a starter template.

## Persistence & Save Policy

All configuration modifications made within the UI should follow the framework's strict four-tier save policy. This guarantees a consistent user experience with appropriate user feedback (toasts or reboot overlays) for every action. Refer to the [Save Policy](save-policy.md) documentation for full implementation patterns.
