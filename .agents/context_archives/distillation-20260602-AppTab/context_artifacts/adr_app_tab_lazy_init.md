# ADR: App Tab Lazy Initialization and Display Config Persistence

## Context
The ESP32 App Foundation required a new "App" configuration tab in the web portal to allow users to customize the clock font size on the `ScreensaverBoard`.
The frontend `app.js` architecture dynamically fetches the tab hierarchy from the `/api/tabs` endpoint during boot and asynchronously injects non-system tabs into the DOM.

## Decision
1. **ConfigManager Integration**: Implemented `IConfigurable` on `U8G2DisplayModule` (schema v2) to persist the `clockFontSize` value via the `ConfigManager` gateway under the `display` namespace.
2. **REST Endpoints**: Added a new POST route `/api/display/clockFontSize` to `U8G2DisplayModule` which accepts a JSON value, updates the live screensaver board, and triggers a save operation to flash memory.
3. **Lazy DOM Initialization**: In `appconfig.js`, the script cannot immediately bind to its container `<div id="tab-app">` on load because `app.js` has not yet dynamically built the DOM structure for it. To solve this race condition, we implemented a lazy-initialization pattern. The script waits for the `tab-changed` event to fire for `tab-app`, at which point it constructs the internal DOM elements and fetches the initial config state.

## Rationale
- Binding synchronously during script execution causes a silent crash (null container) for dynamically injected tabs.
- Waiting for `tab-changed` ensures the container strictly exists in the DOM and prevents unnecessary DOM operations or API requests if the user never clicks the App tab.
