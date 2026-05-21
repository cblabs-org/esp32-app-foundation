# System State — Web Portal Modules

## C++ Core Components

### 1. `WebServerManager` (modules/webServer/)
Manages the web server listening on port 80.
- **Boot Sequence:**
  1. Instantiates `AsyncWebServer` on port 80.
  2. Registers static routes mapped to gzip compressed PROGMEM arrays:
     - `/` -> `index_html_gz`
     - `/app.css` -> `app_css_gz`
     - `/app.js` -> `app_js_gz`
     - `/system.js` -> `system_js_gz`
     - `/wifi.js` -> `wifi_js_gz`
     - `/ota.js` -> `ota_js_gz`
  3. Loops through registered `IWebContributor` instances:
     - Calls `registerRoutes(server)` to attach module-specific API handlers.
     - Fetches `getTabName()` to construct the JSON tab list.
  4. Exposes `GET /api/tabs` containing the list of registered tab IDs and names.
  5. Exposes `GET /api/system` yielding heap telemetry and uptime.

### 2. `IWebContributor` Interface (include/interfaces/IWebContributor.hpp)
```cpp
class IWebContributor {
public:
    virtual ~IWebContributor() = default;
    virtual void registerRoutes(AsyncWebServer& server) = 0;
    virtual const char* getTabName() const = 0;
};
```
Implemented by `WifiManager` (and other modules) to expose settings and status screens dynamically.

### 3. `WifiManager` API Integration (modules/wifiManager/)
- **Endpoints Registered:**
  - `GET /api/wifi` - Retrieves active SSID, softAP status, connection IP, and password presence.
  - `POST /api/wifi` - Accepts JSON payload to update Wi-Fi SSID and Password.
- **Inactivity Timeout:**
  - When in softAP mode, `tick()` checks the elapsed duration since the last API request (`lastApiRequestTime`). If it exceeds 10 minutes (600,000 ms), the manager disconnects the AP, disables the Wi-Fi radio, and shuts down DNS.

---

## Python Build Script

### `portalBuilder.py` (scripts/portalBuilder.py)
A pre-build python script hooked into PlatformIO (`platformio.ini` `extra_scripts = pre:scripts/portalBuilder.py`).
- **Functionality:**
  1. Scans `web/portal/` for frontend source files.
  2. Compresses each file using gzip.
  3. Formats the output as a C++ header containing a `const uint8_t[] PROGMEM` array and its length.
  4. Saves headers to `include/portal_assets/` for inclusion in `webServer.cpp`.

---

## SPA Frontend Architecture

Located in `web/portal/`:
- **`index.html`**: Host page. Features a clean tab layout (Wi-Fi, System, OTA, Config).
- **`app.css`**: Styling using a modern dark mode theme.
- **`app.js`**: Bootstraps the application, loads registered tabs from `/api/tabs`, manages tab transitions, and coordinates the sequential API request queue.
- **Sub-modules (`wifi.js`, `system.js`, `ota.js`)**: Handle API interactions and render the status/forms for their respective screens.

### Sequential Request Queue (Serialization)
To prevent ESP32 TCP socket starvation, `app.js` runs a sequential executor:
```javascript
class RequestQueue {
    constructor() {
        this.queue = [];
        this.running = false;
    }
    
    add(url, options = {}) {
        return new Promise((resolve, reject) => {
            this.queue.push({ url, options, resolve, reject });
            this.next();
        });
    }
    
    next() {
        if (this.running || this.queue.length === 0) return;
        this.running = true;
        const task = this.queue.shift();
        
        fetch(task.url, task.options)
            .then(res => res.json())
            .then(data => task.resolve(data))
            .catch(err => task.reject(err))
            .finally(() => {
                this.running = false;
                this.next();
            });
    }
}
const apiQueue = new RequestQueue();
```
All frontend files invoke `apiQueue.add(url, options)` instead of raw `fetch()`.
