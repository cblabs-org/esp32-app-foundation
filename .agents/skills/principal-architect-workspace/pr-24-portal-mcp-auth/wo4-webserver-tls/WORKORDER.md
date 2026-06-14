# WO4 — WebServerManager Dual-Server TLS Mode

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo4-webserver-tls`  
**Base:** `pr-24-portal-mcp-auth/wo3-tls-cert`  
**Dependencies:** WO3 merged  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Extend `WebServerManager` to run in one of three modes based on runtime state:

| Mode | Trigger | Servers |
|------|---------|---------|
| **HTTP open** | AP mode OR `!auth.enabled` | `AsyncWebServer` on port 80, all routes |
| **TLS + auth** | `auth.enabled && certPresent && !apMode` | `AsyncWebServerSecure` on port 443 (all routes) + `AsyncWebServer` on port 80 (redirect only) |

The protocol model is the governing rule of PR-24. After this WO, the firmware
serves the correct protocol. Auth enforcement (guard on each route) comes in WO5.

---

## Acceptance Criteria

- [ ] In HTTP-open mode: all routes available on port 80, behaviour identical to pre-PR-24
- [ ] In TLS mode: HTTPS server starts on port 443 with the cert loaded from LittleFS
- [ ] In TLS mode: `GET http://<ip>/anything` → `301 Moved Permanently https://<ip>/anything`
- [ ] In TLS mode: `AsyncWebServerSecure` initialised with DER key + cert from `AuthManager::loadCert()`
- [ ] In AP mode: always HTTP open regardless of cert/auth state
- [ ] No crash when cert files are missing but `auth.enabled=true` (fallback to HTTP open, log warning)
- [ ] `WebServerManager::begin()` signature updated to accept `AuthManager*` and `bool apMode`
- [ ] Native build still compiles and all tests pass
- [ ] `pio run -e native` — all tests pass
- [ ] `pio run -e esp32dev` — builds without error (TLS branches `#ifndef NATIVE_BUILD` guarded)
- [ ] Hardware verification: Chrome/Safari shows cert warning on first HTTPS access;
      click-through allows access; port 80 redirects correctly

---

## `WebServerManager` Changes

### Header (`WebServerManager.hpp`)

```cpp
// New includes (hardware only)
#ifndef NATIVE_BUILD
#include <ESPAsyncWebServer.h>   // already included
// AsyncWebServerSecure is in the same ESPAsyncWebServer header in ESP32Async fork
#endif

class WebServerManager {
public:
    WebServerManager();
    ~WebServerManager();

    void begin(const std::vector<IWebContributor*>& contributors,
               AuthManager* authManager = nullptr,
               bool apMode = false);

    AsyncWebServer& getServer();   // returns the active server (HTTPS or HTTP)

private:
    AsyncWebServer  _httpServer;                  // port 80 — always exists
#ifndef NATIVE_BUILD
    AsyncWebServerSecure* _httpsServer = nullptr; // port 443 — heap-allocated in TLS mode
#endif
    bool            _tlsMode = false;
    AuthManager*    _authManager = nullptr;
};
```

**Destructor:** If `_httpsServer` is non-null, delete it. Use RAII pattern.

### `getServer()` returns the right instance:
```cpp
AsyncWebServer& WebServerManager::getServer() {
#ifndef NATIVE_BUILD
    if (_tlsMode && _httpsServer) return *_httpsServer;
#endif
    return _httpServer;
}
```

### `begin()` logic

```cpp
void WebServerManager::begin(const std::vector<IWebContributor*>& contributors,
                             AuthManager* authManager, bool apMode) {
    _authManager = authManager;

    bool useTls = false;
#ifndef NATIVE_BUILD
    if (!apMode && authManager && authManager->_authEnabled && authManager->hasCert()) {
        uint8_t *certDer, *keyDer;
        size_t certLen, keyLen;
        if (authManager->loadCert(&certDer, &certLen, &keyDer, &keyLen)) {
            _httpsServer = new AsyncWebServerSecure(443);
            _httpsServer->setServerKeyAndCertificate(keyDer, keyLen, certDer, certLen);
            useTls = true;
            _tlsMode = true;
            ESP_LOGI(TAG, "WebServerManager: TLS mode — HTTPS on port 443");
        } else {
            ESP_LOGW(TAG, "WebServerManager: cert load failed — falling back to HTTP");
        }
    }
#endif

    // Select the active server reference for route registration
    AsyncWebServer& activeServer = useTls ? *_httpsServer : _httpServer;

    // Register static assets + all contributor routes on the active server
    _registerStaticAssets(activeServer);
    _buildAndRegisterTabsRoute(activeServer, contributors);
    for (auto* c : contributors) {
        if (c) c->registerRoutes(activeServer);
    }
    _registerSystemRoutes(activeServer);

#ifndef NATIVE_BUILD
    if (useTls) {
        // Port 80: redirect-only server
        _httpServer.onNotFound([](AsyncWebServerRequest* req) {
            String url = "https://";
            url += req->host();
            url += req->url();
            req->redirect(url.c_str());
        });
        _httpServer.begin();
        _httpsServer->begin();
    } else {
        _httpServer.begin();
    }
#endif

    ESP_LOGI(TAG, "WebServerManager: server started (TLS=%d, AP=%d)", useTls, apMode);
}
```

**Notes:**
- `authManager->_authEnabled` — this is a private member. Expose it via a public
  accessor: `bool isAuthEnabled() const { return _authEnabled; }` on `AuthManager`.
  Do NOT make `WebServerManager` a friend class.
- The redirect lambda captures nothing (stateless). The `req->host()` call returns
  the Host header value (the device IP or hostname) — suitable for a local redirect.
- The `onNotFound` handler on the HTTP server ensures even unknown paths redirect to HTTPS.

---

## `AuthManager` additions required in this WO

Add public accessor:
```cpp
bool isAuthEnabled() const { return _authEnabled; }
```

---

## Static Asset Routes Refactor

Currently, static asset route registration (`/`, `/app.js`, `/app.css`, etc.) is
interleaved in `WebServerManager::begin()`. Extract it to a private helper:
```cpp
void _registerStaticAssets(AsyncWebServer& server);
void _buildAndRegisterTabsRoute(AsyncWebServer& server,
                                const std::vector<IWebContributor*>& contributors);
void _registerSystemRoutes(AsyncWebServer& server);
```

This refactor is necessary because in TLS mode, routes register on `*_httpsServer`
while the redirect registers on `_httpServer`. The separation makes the logic clear.

---

## `platformio.ini` — Verify No New Library Needed

`ESP32Async/ESPAsyncWebServer@^3.7.7` (already declared) includes
`AsyncWebServerSecure`. Confirm this is the case by checking that
`AsyncWebServerSecure` is defined in the installed library headers:
```
.pio/libdeps/esp32dev/ESPAsyncWebServer/src/ESPAsyncWebServer.h
```
If `AsyncWebServerSecure` is not present in that header (it may be in a separate
`AsyncWebServerSecureKey.h` or similar), add the necessary include and document it.
Do NOT add a new library entry to `platformio.ini` unless strictly required.

---

## Native Build Stubs

`AsyncWebServerSecure` does not exist in the native mock. Guard all secure server
code with `#ifndef NATIVE_BUILD`. The native `WebServerManager` continues to use
the existing `AsyncWebServer` mock unchanged.

---

## Testing

### Native
- `WebServerManager` mock tests — pass `authManager=nullptr` → HTTP mode selected
- No new hardware-specific tests in native

### Hardware
1. Flash with auth disabled → port 80 serves portal normally
2. Set password (via WO2 route, running over HTTP) → restart
3. After restart with cert present + auth enabled: confirm port 443 TLS, port 80 redirects
4. Check that `curl -v http://<ip>/` returns `301` with `Location: https://<ip>/`
5. Check that `curl --insecure https://<ip>/api/auth/status` returns 200 JSON

---

## Handoff to WO5

WO5 (`wo5-portal-guard`) will add the auth check to each protected route. It
requires `_authManager` to be accessible inside `WebServerManager::begin()` during
route registration — which is available after this WO through the stored `_authManager`
pointer.

WO5 will wrap each contributor's route handler with a pre-check lambda. The
mechanism is: WO5 modifies the route registration loop to inject the guard, or
`AuthManager::checkPortalAuth()` is called at the top of each route lambda
(per-module approach). WO5 implementer should evaluate and choose; see DESIGN.md
for the two options.
