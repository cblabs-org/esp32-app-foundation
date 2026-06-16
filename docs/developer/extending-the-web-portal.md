# Extending the Web Portal

> ESP32 App Foundation (c) 2026 CB Labs
>
> Tutorial on adding custom bottom-nav tabs, HTML/JS fragments, and REST API endpoints to the framework's SPA web portal.

---

The ESP32 App Foundation features a local Single Page Application (SPA) web portal hosted on the device. Downstream developers can extend the portal nav bar with custom tabs and register REST API endpoints by implementing the [IWebContributor](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IWebContributor.hpp) interface.

For a detailed blueprint, see the [Web Portal Architecture](../design_specification.md#10-web-portal-architecture) section in the Design Specification.

---

## 1. The IWebContributor Interface

To add a tab or expose web routes, your module must inherit from `IWebContributor` and implement its virtual functions.

### Interface Method Walkthrough:

#### `registerRoutes(PsychicHttpServer& server)`
Called during the boot phase (`WebServerManager::begin()`). This is where you register HTTP route handers (GET, POST, etc.) and bind them to C++ lambda functions.
```cpp
void registerRoutes(PsychicHttpServer& server) override;
```

#### `getTabName()`
Returns the user-facing display label for the tab (e.g., "MyApp"). If a module only needs to register REST API routes but does not require a frontend tab, return `nullptr` (default).
```cpp
const char* getTabName() const override { return "MyApp"; }
```

#### `getParentTab()`
Returns the name of a parent tab to create hierarchical sub-navigation.
- Return `nullptr` (default) to render the tab as a top-level item in the main bottom navigation bar.
- Return `"System"` to nest the tab under the system sub-navigation (rendered as pills in the System pane).
```cpp
const char* getParentTab() const override { return nullptr; } // Renders in bottom-nav
```

#### `getTabOrder()`
Returns an integer that controls the rendering order relative to other tabs at the same level. Lower values are positioned first. Default is `100`.
```cpp
int getTabOrder() const override { return 100; }
```

#### `getTabIcon()`
Returns the SVG path body (inner content of the `<svg>` block) to use as the nav icon. If you return `nullptr` (default), the web portal automatically displays a generic grid icon.
```cpp
const char* getTabIcon() const override {
    // Example: simple checkmark icon path
    return "<path d='M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z'/>";
}
```

---

## 2. End-to-End Pattern: C++ to DOM

When a user clicks your tab, the web portal loads your HTML/JS code from LittleFS and interacts with the REST endpoints registered in C++.

### C++ Web API Routes (Backend)
Here is the route registration from the template file [myApp.cpp](../../template/src/myApp.cpp). It registers a GET route to retrieve status, and a POST route to update settings:

```cpp
#ifndef NATIVE_BUILD
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // --- GET Endpoint: Retrieve Status ---
    server.on("/api/myapp/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["refreshMs"] = _refreshIntervalMs;
        doc["enabled"] = _featureEnabled;
        doc["gateOpen"] = _gateOpen;

        String output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
    });

    // --- POST Endpoint: Save Configuration ---
    server.on("/api/myapp/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            return response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }

        // Apply new settings
        if (doc["refreshMs"].is<int>()) {
            _refreshIntervalMs = doc["refreshMs"].as<int>();
        }
        if (doc["enabled"].is<bool>()) {
            _featureEnabled = doc["enabled"].as<bool>();
        }

        // Persist config to LittleFS
        app.getConfigManager().saveModuleConfig(this);

        return response->send(200, "application/json", "{\"status\":\"saved\"}");
    });
}
#endif
```

---

## 3. The HTML Fragment Pattern

Rather than reloading the entire webpage, the portal's main Single Page Application shell (`app.js`) fetches raw HTML fragments dynamically from the filesystem when a tab is clicked and injects them into the main DOM container.

### The HTML File (`/www/myapp/tab.html`)
Create an HTML snippet representing your tab UI inside the `/www/<namespace>/tab.html` folder. It should contain only the body components, not wrapping `<html>` or `<body>` elements:

```html
<!-- MyApp tab content fragment -->
<div class="tab-content" id="myapp-tab">
    <h2>My Application</h2>
    <div class="card">
        <h3>Status</h3>
        <div id="myapp-status">Loading...</div>
    </div>
    <div class="card">
        <h3>Configuration</h3>
        <label>
            Refresh Interval (ms):
            <input type="number" id="myapp-refresh-ms" value="5000" min="1000" max="60000">
        </label>
        <label>
            <input type="checkbox" id="myapp-enabled" checked>
            Feature Enabled
        </label>
        <button id="myapp-save-btn">Save</button>
    </div>
</div>
```

---

## 4. The JavaScript Fragment Pattern

Immediately after loading the HTML snippet, the SPA shell loads and executes your matching Javascript fragment located at `/www/<namespace>/tab.js`.

### The JS File (`/www/myapp/tab.js`)
To prevent namespace pollution, your script must be wrapped in an **Immediately Invoked Function Expression (IIFE)**. Wire up event listeners and fetch data from your API endpoints:

```javascript
(function() {
    'use strict';

    const statusEl = document.getElementById('myapp-status');
    const refreshInput = document.getElementById('myapp-refresh-ms');
    const enabledInput = document.getElementById('myapp-enabled');
    const saveBtn = document.getElementById('myapp-save-btn');

    // Fetch current status from the C++ REST endpoint
    async function loadStatus() {
        try {
            const response = await fetch('/api/myapp/status');
            if (!response.ok) throw new Error('Failed to fetch status');
            const data = await response.json();
            
            // Render data to DOM
            statusEl.textContent = JSON.stringify(data, null, 2);
            
            // Pre-populate input fields
            refreshInput.value = data.refreshMs;
            enabledInput.checked = data.enabled;
        } catch (err) {
            statusEl.textContent = 'Error: ' + err.message;
        }
    }

    // Post new settings back to the C++ REST endpoint
    async function saveConfig() {
        const refreshMs = refreshInput.value;
        const enabled = enabledInput.checked;
        
        try {
            const response = await fetch('/api/myapp/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ refreshMs: parseInt(refreshMs), enabled })
            });
            if (!response.ok) throw new Error('Failed to save config');
            
            alert('Configuration saved successfully');
            loadStatus(); // Reload values
        } catch (err) {
            alert('Save failed: ' + err.message);
        }
    }

    // Wire up event listeners
    saveBtn?.addEventListener('click', saveConfig);

    // Initial load
    loadStatus();
})();
```

---

## 5. API Route Conventions

- **Path Naming:** All API paths must start with `/api/` followed by your module's namespace: `/api/<namespace>/<endpoint>`.
- **Method Usage:**
  - `HTTP_GET` for queries (e.g., retrieving status or reading variables).
  - `HTTP_POST` for triggering actions or saving configurations.
- **Payload Format:** Request and response payloads must utilize the `application/json` mime type.
- **Resource Constraints:** Always parse JSON bodies safely. If payload size is large, verify memory heap space prior to allocations to avoid low-memory conditions.

---

## 6. The Tab Registry Contract: `/api/tabs`

The main web page generates the navigation interface by querying `/api/tabs` on load. 

The `WebServerManager` builds this registry dynamically at boot by querying all registered `IWebContributor` pointers. It returns a JSON array of configuration objects.

### JSON Schema Response:
```json
[
  {
    "name": "Status",
    "id": "Status",
    "parent": "System",
    "order": 10
  },
  {
    "name": "MyApp",
    "id": "MyApp",
    "order": 100,
    "icon": "<path d='M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z'/>"
  }
]
```

### Nav Resolution logic:
When the client SPA sees this array, it:
1. Sorts all entries based on the `order` key.
2. Group tabs with a `parent` key under the respective parent dropdown/view (e.g., nesting "Status" as a sub-pill under the "System" tab).
3. Renders top-level entries (no `parent` field) in the bottom-nav bar.
4. Binds click handlers to load `/www/<id-lowercase>/tab.html` and `/www/<id-lowercase>/tab.js`.

---

## 7. Web Portal Security

When the framework's `AuthManager` is enabled:
- The global route middleware in `WebServerManager` automatically blocks requests to `/api/` endpoints that do not supply a valid JSON Web Token (JWT) cookie or authorization header.
- Downstream developers **do NOT** need to write authentication verification code inside individual route handlers. Route-level security is managed centrally.
- Public endpoints (like `/api/auth/login`, `/api/tabs`, and static web portal resources) bypass this authentication filter automatically to allow the initial login page to load.
