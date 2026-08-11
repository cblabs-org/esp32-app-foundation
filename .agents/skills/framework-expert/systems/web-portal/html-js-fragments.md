# Portal Tab JavaScript Structure

Because the web portal does not load external HTML fragments, each tab's entire user interface is constructed dynamically using JavaScript. Each tab module provides a script that writes its own markup and handles events.

## C++ Script Registration and Delivery

The tab's JavaScript file is stored as a raw-string literal in C++ and served via an HTTP GET route registered in `registerRoutes()`.

For example, in `WeatherModule.cpp`:

```cpp
void WeatherModule::registerRoutes(PsychicHttpServer& server) {
    // Serve the JavaScript file
    server.on("/weather.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        const char* js = R"rawliteral(
(function() {
    var initialized = false;

    window.addEventListener('tab-changed', function(e) {
        if (e.detail !== 'tab-weather') return;

        var container = document.getElementById('tab-weather');
        if (!container) return;

        if (!initialized) {
            initialized = true;
            container.innerHTML = `
                <article>
                    <header>Weather Configuration</header>
                    <form id="weather-form">
                        <label for="weather-lat">Latitude</label>
                        <input type="text" id="weather-lat">
                        <button type="submit" id="weather-save-btn">Save</button>
                    </form>
                </article>
            `;
            // Bind events here
        }
    });
})();
        )rawliteral";
        return response->send(200, "application/javascript", js);
    });
}
```

The route path (e.g. `/weather.js`) is declared in C++ by overriding `getTabScript()`:

```cpp
const char* getTabScript() const override { return "/weather.js"; }
```

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
