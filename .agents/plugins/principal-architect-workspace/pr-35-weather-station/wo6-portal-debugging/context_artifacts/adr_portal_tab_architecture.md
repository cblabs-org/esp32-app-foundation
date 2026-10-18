# ADR: Portal Tab Architecture (Modules serving UI)

## Context
When dynamically populating the frontend Single-Page Application (SPA) portal, the `esp32-app-foundation` framework aggregates tabs via the `/api/tabs` endpoint, and uses a hardcoded script-loader inside `web/portal/app.js` to execute fragments.

## Decision
All web contributors (like `HomeAssistantModule` or `WeatherModule`) that want to render a UI inside their respective tabs **MUST** serve a self-contained JavaScript snippet (e.g. `/weather.js`) instead of a raw HTML template (e.g. `tab.html`). 
Furthermore, the injected script must wait to populate the DOM until the `tab-changed` event fires, otherwise `app.js` will not have finished dynamically allocating the empty `div.tab-content` container for that specific tab, causing early exits or null exceptions.

## Consequences
- Modules cannot blindly assign `innerHTML` immediately on script execution. They must listen to `window.addEventListener('tab-changed', ...)` to do the assignment.
- Downstream application modules like `WeatherModule` MUST have their frontend scripts appended to `app.js`'s script loader array, and baked into the framework using `scripts/portalBuilder.py`.
