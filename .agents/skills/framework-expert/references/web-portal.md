# Web Portal Extension Reference

## The IWebContributor Interface

To contribute navigation tabs and register REST API endpoints, inherit from `IWebContributor` and implement these methods:

*   `virtual void registerRoutes(PsychicHttpServer& server) = 0;`
    Registers C++ REST API endpoints at boot time.
*   `virtual const char* getTabName() const { return nullptr; }`
    Returns the user-facing tab name in the UI. Return `nullptr` to expose routes only (no tab).
*   `virtual const char* getParentTab() const { return nullptr; }`
    Return `nullptr` to display as a top-level tab. Return `"System"` to group under the system pane.
*   `virtual int getTabOrder() const { return 100; }`
    Determines UI render ordering (lower numbers appear first).
*   `virtual const char* getTabIcon() const { return nullptr; }`
    Returns inner SVG path string (e.g. `<path d="..." />`). Return `nullptr` for a default grid icon.

## C++ Route Registration Pattern

Register routes using Lambda callback functions:
```cpp
#ifndef NATIVE_BUILD
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // GET Route
    server.on("/api/myapp/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["status"] = "ok";
        String output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
    });

    // POST Route
    server.on("/api/myapp/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        JsonDocument doc;
        if (deserializeJson(doc, body) != DeserializationError::Ok) {
            return response->send(400, "application/json", "{\"error\":\"invalid_json\"}");
        }
        _value = doc["value"] | 0;
        app.getConfigManager().saveModuleConfig(this);
        return response->send(200, "application/json", "{\"status\":\"saved\"}");
    });
}
#else
void MyApp::registerRoutes(PsychicHttpServer&) {} // Mocked out for host-based tests
#endif
```

## HTML UI Fragment Pattern
*   **Path:** Saved on LittleFS at `/www/<namespace>/tab.html` (e.g., `/www/myapp/tab.html`).
*   **Structure:** No wrapping `<html>` or `<body>` elements. Contain only clean, responsive HTML wrappers and elements:
```html
<div class="tab-content" id="myapp-tab">
    <h2>My App Settings</h2>
    <div class="card">
        <label>Configuration Value: <input type="number" id="myapp-val"></label>
        <button id="myapp-save-btn">Save Settings</button>
    </div>
</div>
```

## JS Fragment Fetch Pattern
*   **Path:** Saved on LittleFS at `/www/<namespace>/tab.js` (e.g., `/www/myapp/tab.js`).
*   **IIFE Wrapper:** Must wrap the entire script in an Immediately Invoked Function Expression (IIFE) to prevent namespace pollution.
```javascript
(function() {
    'use strict';
    const inputEl = document.getElementById('myapp-val');
    const saveBtn = document.getElementById('myapp-save-btn');

    async function fetchConfig() {
        const res = await fetch('/api/myapp/status');
        const data = await res.json();
        inputEl.value = data.value;
    }

    async function saveConfig() {
        await fetch('/api/myapp/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ value: parseInt(inputEl.value) })
        });
        alert('Config Saved');
    }

    saveBtn?.addEventListener('click', saveConfig);
    fetchConfig();
})();
```

## The Tab Registry: `/api/tabs`
At boot, `WebServerManager` queries all registered `IWebContributor` modules and exposes a unified JSON array listing tabs. The SPA client queries `/api/tabs` to build the navigation layout dynamically.
*   **JSON Schema:**
```json
[
  {
    "name": "MyApp",
    "id": "MyApp",
    "order": 100,
    "icon": "<path d='...'/>"
  }
]
```
*   **Client Loading:** SPA shell binds click events on tabs, dynamically fetching `/www/myapp/tab.html` and `/www/myapp/tab.js` and injecting them into the DOM.

## Portal Security
All `/api/` endpoints are intercepted by a global JWT auth middleware. Individual route handlers do **not** need to handle auth checks; public paths like `/api/auth/login` and static SPA assets are bypassed automatically.
