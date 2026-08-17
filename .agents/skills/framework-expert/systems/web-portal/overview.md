# Web Portal Overview

The App Foundation serves a rich, Single Page Application (SPA) web portal directly from the ESP32. It provides a modular UI where users can configure the device, view telemetry, and manage OTA updates.

## SPA Architecture

The portal is a Single Page Application driven by `app.js`. Unlike a traditional multi-page website or a framework loading HTML fragments dynamically over HTTP:
1. The client loads a single `index.html` skeleton and the core `app.js` framework.
2. `app.js` queries the ESP32 for the available UI tabs via the `/api/tabs` endpoint during boot.
3. The boot sequence dynamically loads the declared scripts for each tab.
4. Once scripts are loaded, navigation elements are built.
5. As the user clicks tabs, a `tab-changed` event is dispatched. The loaded tab JavaScript hears this event and writes its own markup directly into its designated container.

## Tab Contribution via IWebContributor

Modules inject their UI and routes into the portal by implementing the `IWebContributor` interface. The `WebServerManager` queries all registered contributors during initialization to register their routes and build the navigation structure.

> [!NOTE]
> There is a private struct called `AppTabContributor` within `Application.hpp`, but this is an internal framework helper and is not part of the public tab-registration API. Modules should implement `IWebContributor` directly.

To contribute a tab, implement the following virtual overrides in your module:

* `virtual void registerRoutes(PsychicHttpServer& server) = 0`
  Registers the module's REST API route handlers with the web server. It is also where the module registers the route serving its JavaScript file — canonically via the gzip+PROGMEM pipeline and the shared `serveGzipAsset()` helper (see [html-js-fragments.md](html-js-fragments.md)), not a raw-string literal.
* `virtual const char* getTabName() const`
  Returns the display name for the UI tab (e.g., `"Weather"`). Return `nullptr` if the module registers API routes but does not require a UI tab.
* `virtual const char* getParentTab() const`
  Returns the display name of the parent tab (e.g., `"System"`) to nest this tab as a sub-section/pill. Return `nullptr` for a top-level tab (displayed in the main bottom navigation bar).
* `virtual int getTabOrder() const`
  Returns the integer sort order (lower values appear first). Default is `100`.
* `virtual const char* getTabIcon() const`
  Returns the inner SVG elements string (e.g. `<path ... />`) for the bottom-nav icon. Return `nullptr` to use the default generic grid icon. The outer `<svg>` tag wrapper is supplied by the client.
* `virtual const char* getTabScript() const`
  Returns the route path for this tab's JavaScript file (e.g., `"/weather.js"`). Return `nullptr` if the tab requires no script. The path MUST start with `/` (same-origin only) or it will be ignored by `WebServerManager`.

## /api/tabs JSON Schema

During boot, the SPA fetches `GET /api/tabs`. The response is a JSON array containing metadata for all UI tabs:

```json
[
  {
    "name": "WiFi",
    "id": "WiFi",
    "order": 0
  },
  {
    "name": "Weather",
    "id": "Weather",
    "order": 50,
    "icon": "<path d=\"M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z\"/>",
    "script": "/weather.js"
  },
  {
    "name": "Status",
    "id": "Status",
    "parent": "System",
    "order": 10
  }
]
```

Each tab object includes:
- `name` (string): The tab's display name.
- `id` (string): The identifier of the tab (derived from name).
- `parent` (string, optional): The name of the parent tab if nested.
- `order` (number): The sort order.
- `icon` (string, optional): The inner SVG path data if custom icon is defined.
- `script` (string, optional): The route to the tab's JavaScript file.

## Boot and Script Loading Sequence

The web portal boots via the following sequential steps:

1. **Skeleton Load**: The browser loads `index.html` and executes `app.js`.
2. **Metadata Fetch**: `app.js` issues a request to `/api/tabs` to fetch the navigation structure.
3. **Script Extraction**: `app.js` extracts all non-null `script` properties from the tabs response.
4. **Sequential Loading**: `app.js` sequentially appends `<script>` elements to the DOM for all built-in scripts, followed by the dynamically discovered tab scripts.
5. **DOM Container Preparation**: Once all scripts are loaded, `app.js` processes the tab list. For any top-level tab, it checks if a corresponding content div exists. If not, it creates:
   `<div id="tab-slug" class="tab-content" role="tabpanel" tabindex="0" aria-labelledby="nav-slug">`
   where the container ID slug is derived from the name: `'tab-' + name.toLowerCase().replace(/\s+/g, '-')`. Sub-pill content divs follow the same pattern but reference their pill button: `aria-labelledby="pill-slug"`.
6. **Navigation Render**: The bottom navigation links and sub-pill buttons are rendered. Each bottom-nav link carries `role="tab"`, `aria-selected`, and `aria-controls` pointing at its specific panel. Sub-pill buttons carry the same ARIA attributes, and the pill bar container has `role="tablist"`.
7. **Initial Activation**: The default tab is activated, triggering the `tab-changed` event and causing the script to render its content.

## Web Server Stack & DRAM Tuning

The web server task stack size and socket limits are board-configurable, and deep call stack operations (such as PNG screenshot compression) are offloaded to PSRAM via `DeepStackExecutor` to preserve internal DRAM. For full details on the concurrency model, capability matrix, and mandatory OTA flash-safety rules, see [Web Server Stack & DRAM Tuning Architecture](dram-tuning.md).

## Design System

When building or modifying portal UI, use the project's design system for all styling decisions — colour tokens, typography, spacing, component patterns (Drawer, PillBar, InfoRow, ToggleSwitch, etc.), and the module-tab scaffold template. See the sibling skill at `../../../framework-design-system/readme.md`.
