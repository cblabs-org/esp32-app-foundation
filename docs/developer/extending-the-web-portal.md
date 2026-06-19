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
Create an HTML snippet representing your tab UI inside the `/www/<namespace>/tab.html` folder. It should contain only the body components, not wrapping `<html>` or `<body>` elements.

> [!IMPORTANT]
> **HTML ID Scoping Rule:** All element IDs must be prefixed with your module name to avoid collisions with other tabs loaded into the same SPA DOM. Use `id="myapp-save-btn"` — never `id="save-btn"`. Multiple tabs coexist in a single HTML document, so unprefixed IDs will clash.

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

### The `tab.js` IIFE Pattern

Because all tab scripts execute in the same global `window` scope, variable names from one tab can collide with another. To prevent this namespace pollution, **all tab JavaScript MUST be wrapped in an Immediately Invoked Function Expression (IIFE)**:

```javascript
(function() {
    'use strict';
    // All tab logic lives inside this closure.
    // Variables declared here are NOT visible to other tabs.
})();
```

> [!WARNING]
> If you omit the IIFE wrapper, any `const`, `let`, or `function` declarations at the top level will collide with identically-named symbols from other tabs, causing hard-to-debug runtime errors.

### The JS File (`/www/myapp/tab.js`)
Wire up event listeners and fetch data from your API endpoints:

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
            
            window.showToast('Configuration saved.', 'success');
            loadStatus(); // Reload values
        } catch (err) {
            window.showToast('Save failed: ' + err.message, 'error');
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

## 6. Save & Reboot Policy

When adding settings to your tab, follow the framework's standard save policy to ensure UX consistency across all portal tabs.

### The Four Tiers

| Tier | Name | When to Use | UI Pattern |
|------|------|-------------|------------|
| **Instant** | Single-field settings that take effect immediately (no reboot) | Auto-save on each change + `showToast()` feedback |
| **Batched** | Multiple related fields that should be committed atomically | Explicit "Save" button + `showToast()` feedback |
| **Disruptive** | Settings that require a device reboot to take effect | Auto-save or button + `showRebootOverlay()` |
| **Destructive** | Irreversible actions (reset, delete, erase) | `armButton()` or typed confirmation + `showRebootOverlay()` |

> [!IMPORTANT]
> **Every save must produce user feedback.** A silent save (no toast, no overlay) is a policy violation. Use `window.showToast()` for transient feedback and `window.showRebootOverlay()` for reboot paths.

### Shared Utility Functions

The framework provides these global functions in `utils.js` (loaded before all tab scripts):

- **`window.showToast(message, type, duration)`** — Slide-in toast notification. Types: `'success'`, `'error'`, `'warning'`, `'info'`.
- **`window.armButton(button, action, timeout)`** — Armed confirmation pattern. First click arms the button, second click within timeout executes the action.
- **`window.showRebootOverlay(message, subMessage)`** — Full-screen overlay with spinner and reconnection polling. Auto-reloads the page when the device comes back online.
- **`window.apiFetch(url, options)`** — Serialised fetch wrapper. **Always use this instead of raw `fetch()`** — it queues requests to protect ESP32 socket pools.

### Decision Rule

> Auto-save is the default. Only use an explicit "Save" button when the user is editing **multiple related fields** that should be committed atomically.

---

## 7. POST Safety (ADR-15)

Handling POST requests on the ESP32 requires strict safety constraints due to RAM limitations. **ADR-15** governs all incoming POST body handling.

### 4096-Byte Body Cap

The framework enforces a hard limit of **4096 bytes** on incoming POST bodies. The `PsychicHttp` server rejects larger payloads before the route handler is invoked. This protects against memory exhaustion from malicious or oversized requests.

### Mandatory Stack-Allocated JSON Parsing

When parsing JSON from a POST body, you **MUST** use `StaticJsonDocument` allocated on the stack. **Never** use `DynamicJsonDocument` (heap-allocated) for incoming request parsing — concurrent requests will fragment the heap and cause panics.

```cpp
server.on("/api/myapp/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
    const char* body = request->bodyCStr();
    if (!body || strlen(body) == 0) {
        return response->send(400, "application/json", "{\"error\":\"body_required\"}");
    }

    // ADR-15: MUST use StaticJsonDocument (stack allocation)
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        return response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    }

    // Process parsed data...
    return response->send(200, "application/json", "{\"status\":\"saved\"}");
});
```

> [!WARNING]
> Using `DynamicJsonDocument` or `JsonDocument` (which heap-allocates in ArduinoJson v7) in route handlers is **unsafe** under concurrent load. Stick to `StaticJsonDocument` with a size that fits within the 4096-byte body cap. A `StaticJsonDocument<1024>` is usually sufficient for configuration payloads.

---

## 8. The Tab Registry Contract: `/api/tabs`

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

## 9. Authentication & Security

The framework's web portal uses JSON Web Tokens (JWT) for API endpoint security. When the `AuthManager` is enabled, downstream modules get authentication enforcement for free — no per-route auth code is required.

### AuthManager

The [AuthManager](../../modules/WebServerManager/AuthManager.hpp) is a core framework service responsible for:
1. Validating user credentials (defined via `FrameworkConfig` or stored in a secure partition)
2. Generating JWT tokens upon successful login
3. Validating incoming JWT tokens on subsequent API requests

### JWT Middleware Flow

The `WebServerManager` attaches an authentication middleware to the `PsychicHttpServer` instance. This middleware intercepts **all** requests to `/api/*` paths.

```mermaid
sequenceDiagram
    participant Client
    participant Middleware as JWT Middleware
    participant Handler as Route Handler

    Client->>Middleware: GET /api/myapp/status
    Middleware->>Middleware: Check Authorization header<br/>or auth_token cookie
    alt Valid JWT
        Middleware->>Handler: Forward request
        Handler-->>Client: 200 OK + payload
    else Missing or invalid JWT
        Middleware-->>Client: 401 Unauthorized
    end
```

When a request arrives, the middleware performs a **dual-check**:
1. Looks for an `Authorization: Bearer <token>` HTTP header
2. Falls back to checking for an `auth_token` cookie

If either provides a valid, non-expired JWT, the request proceeds to the module's route handler. If both are missing or invalid, the middleware immediately returns `401 Unauthorized` — the route handler is never invoked.

### Public Path Bypass

Certain routes must be accessible without authentication. The middleware maintains a bypass list:

| Path Pattern | Reason |
|:---|:---|
| `/api/auth/login` | Client must be able to submit credentials to obtain a JWT |
| `/api/tabs` | Tab registry must load before authentication to render the login page |
| Non-`/api/` paths | Static assets (`.html`, `.css`, `.js`, images) are served without auth |

### What Downstream Developers Need to Know

> [!IMPORTANT]
> You do **NOT** need to write authentication verification code inside individual route handlers. All `/api/` routes are automatically protected by the global JWT middleware. Simply register your routes via `registerRoutes()` and the framework handles security.

The SPA client is responsible for intercepting `401` responses from the API, destroying the local token, and redirecting the user to the login screen.
