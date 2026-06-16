# Web Portal Overview

The App Foundation serves a rich, Single Page Application (SPA) web portal directly from the ESP32. It provides a modular UI where users can configure the device, view telemetry, and manage OTA updates.

## SPA Architecture

The portal is not a collection of static HTML pages navigated via full browser reloads. Instead:
1. The client loads a single `index.html` skeleton and the core `app.js` framework.
2. `app.js` queries the ESP32 for the available UI tabs.
3. As the user clicks tabs in the sidebar, `app.js` dynamically fetches HTML and JS fragments for that specific tab and injects them into the DOM.

## The Tab Registry and AppTabContributor

Modules inject their UI into the portal by implementing the `IWebContributor` interface (specifically via the `AppTabContributor` pattern).

During initialization, modules register their tabs with the `WebPortal`:

```cpp
// Registering a tab in a module's begin()
app->getWebPortal()->registerTab(new AppTabContributor(
    "weather",        // ID
    "Weather",        // Display Name
    "mdi-weather",    // Icon
    "/weather/tab.html", // HTML fragment path
    "/weather/tab.js"    // JS fragment path
));
```

## /api/tabs JSON Schema

When the SPA loads, it issues a GET request to `/api/tabs`. The ESP32 responds with a JSON array built dynamically from all registered `AppTabContributor`s.

```json
[
  {
    "id": "system",
    "name": "System",
    "icon": "mdi-cog",
    "htmlUrl": "/system/tab.html",
    "jsUrl": "/system/tab.js"
  },
  {
    "id": "weather",
    "name": "Weather",
    "icon": "mdi-weather",
    "htmlUrl": "/weather/tab.html",
    "jsUrl": "/weather/tab.js"
  }
]
```

## Dynamic Fragment Loading

Using the URLs provided in the `/api/tabs` response, the client application dynamically fetches the HTML and JS assets for the active tab. This lazy-loading approach keeps the initial page load extremely fast and conserves ESP32 memory, as the server only streams the files requested by the client, rather than compiling a massive monolithic HTML document in RAM.
