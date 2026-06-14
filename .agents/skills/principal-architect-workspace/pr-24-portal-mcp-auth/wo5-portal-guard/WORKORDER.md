# WO5 — Portal API Auth Guard Integration

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo5-portal-guard`  
**Base:** `pr-24-portal-mcp-auth/wo4-webserver-tls` + `wo2-auth-crypto` (both merged)  
**Dependencies:** WO2 + WO4 merged  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Wire the auth guard into all protected `/api/*` routes so that when
`auth.enabled = true`, any request without a valid session cookie receives
`401 Unauthorized`. Exempt routes remain open. The `Secure` attribute is
added to the session cookie now that TLS is active (WO4).

---

## Acceptance Criteria

- [ ] All `/api/*` routes (except exempt list) return 401 when `auth.enabled=true`
      and no valid session cookie is present
- [ ] All `/api/*` routes return 200 normally when a valid session cookie is present
- [ ] All `/api/*` routes return 200 normally when `auth.enabled=false` (no guard)
- [ ] Session cookie issued by `/api/auth/login` now includes `Secure` attribute
      (since TLS is active when auth is enabled)
- [ ] Exempt routes always return 200 regardless of auth state:
      `GET /`, static assets, `GET /api/auth/status`, `POST /api/auth/login`,
      `GET /api/tabs`, `GET /api/app`
- [ ] Factory reset route `POST /api/system/factory-reset` is auth-gated
      (user must be logged in to factory-reset via portal — this is intentional)
- [ ] AP mode: all routes open (guard returns true without checking cookie when in AP mode)
- [ ] `pio run -e native` — all tests pass
- [ ] New native tests: 401 on protected routes without cookie; 200 with valid cookie;
      exempt routes always 200; AP mode always 200

---

## Guard Implementation

### Strategy: `AuthManager` passed to each `IWebContributor::registerRoutes()`

After evaluating the `AsyncWebServer` API, the centralised WebServerManager
pre-handler approach is not viable — `AsyncWebServer` does not support middleware
hooks that fire before route handlers. Route handlers are matched and called
directly. Therefore, the guard is injected via the per-module approach:

`WebServerManager::begin()` passes `AuthManager*` to each contributor's
`registerRoutes()` call. Each contributor uses it at the top of its protected
route lambdas.

### IWebContributor interface update

```cpp
// webServerManager calls this instead of registerRoutes(server) when auth is available
virtual void registerRoutes(AsyncWebServer& server, AuthManager* auth) {
    registerRoutes(server);  // default: fall back to no-auth registration
}
```

This is backward compatible — existing contributors that don't override the
two-argument version fall through to the single-argument version.

**`AuthManager`** overrides the two-argument version (it knows about itself):
```cpp
void registerRoutes(AsyncWebServer& server, AuthManager* auth) override;
// auth == this in this case — same pointer
```

Each other contributor (`WifiManager`, `OTAUpdateManager`, `KeyManager`,
`TimeManager`, `MCPServer`, etc.) overrides the two-argument version to add the
guard. The single-argument version is kept as a no-auth fallback for tests.

### Guard macro / inline

Add to `modules/AuthManager/AuthManager.hpp` (public, callable without auth pointer):
```cpp
// Convenience macro for contributor route handlers.
// auth may be nullptr (no guard); if non-null and check fails, sends 401 and the
// lambda must 'return'.
#define AUTH_PORTAL_GUARD(auth, request)              \
    do {                                              \
        if ((auth) && !(auth)->checkPortalAuth(request)) return; \
    } while(0)
```

Usage inside a contributor lambda:
```cpp
server.on("/api/wifi/status", HTTP_GET, [this, auth](AsyncWebServerRequest* req) {
    AUTH_PORTAL_GUARD(auth, req);
    // ... normal handler code ...
});
```

### `checkPortalAuth()` — full implementation (replaces WO1/WO2 stubs)

```cpp
bool AuthManager::checkPortalAuth(AsyncWebServerRequest* request) {
    if (!_authEnabled) return true;              // auth disabled — open
    if (_checkSessionCookie(request)) return true; // valid session
    request->send(401, "application/json",
                  "{\"error\":\"unauthorized\"}");
    return false;
}
```

### `Secure` cookie attribute

In `AuthManager::registerRoutes()` login handler, update the cookie string:
```cpp
// Before WO5 (HTTP):
"Set-Cookie: esp32_session=<hex>; HttpOnly; SameSite=Strict"

// After WO5 (HTTPS active when auth is on):
"Set-Cookie: esp32_session=<hex>; HttpOnly; SameSite=Strict; Secure"
```

Since `Secure` is only meaningful when the connection is HTTPS, and auth is only
active when HTTPS is running (auth.enabled requires cert — see WO4), it is safe
to add `Secure` unconditionally here.

---

## Contributors to Update

The following contributors need the two-argument `registerRoutes()` override with
guard on all protected routes:

| Module | Protected routes |
|--------|-----------------|
| `WifiManager` | All `/api/wifi/*` routes |
| `OTAUpdateManager` | `POST /api/ota` |
| `KeyManager` | All `/api/keys/*` routes |
| `TimeManager` | All `/api/time/*` routes |
| `MCPServer` | `/api/mcp`, `/api/mcp/status`, `/api/mcp/config` (bearer guard in WO6; portal session guard here too) |
| `WebServerManager` | `GET /api/system` (status), `POST /api/system/factory-reset`, `POST /api/system/restart`, `GET /api/app`, `/api/backup/*` |
| `AuthManager` | `POST /api/auth/logout`, `POST /api/auth/password` (if passwordSet=true), `GET /api/auth/mcp-token` |

**NOT guarded** (exempt):
- `GET /api/auth/status`, `POST /api/auth/login` (AuthManager)
- `GET /api/tabs`, `GET /api/app` (WebServerManager — public metadata)
- All static asset routes

---

## `WebServerManager::begin()` loop update

Replace the current loop:
```cpp
contributor->registerRoutes(_server);
```
With:
```cpp
contributor->registerRoutes(activeServer, _authManager);
```

---

## AP Mode Handling

`AuthManager::checkPortalAuth()` needs to know if the device is in AP mode.
Add a flag:
```cpp
void AuthManager::setAPMode(bool apMode) { _apMode = apMode; }
bool _apMode = false;
```

`Application::begin()` calls `_authManager.setAPMode(_wifiManager.getAPMode())`
after `_authManager.begin()`. `checkPortalAuth()` returns `true` immediately if
`_apMode = true`.

---

## Native Tests

Extend `test/native/test_auth_manager/test_auth_manager.cpp`:
- `test_portal_guard_open_when_disabled` — `checkPortalAuth()` returns true when `_authEnabled=false`
- `test_portal_guard_blocks_without_cookie` — returns false when enabled, no cookie
- `test_portal_guard_passes_with_valid_cookie` — returns true when token matches
- `test_portal_guard_open_in_ap_mode` — returns true when `_apMode=true`, regardless of auth

Also add basic route-level guard tests simulating the AUTH_PORTAL_GUARD macro
using the mock `AsyncWebServerRequest` stub.

---

## Implementation Notes

- The `IWebContributor` default two-argument `registerRoutes()` falls through to
  the single-argument version — this is a non-breaking change to the interface.
- Any `IWebContributor` implementation that does NOT override the two-argument
  version will run its routes without auth guards — acceptable only for modules
  that have no protected routes (e.g. a read-only status module). All framework
  modules do expose sensitive routes and MUST override.
- Order of operations: guard check fires before any JSON parsing — this prevents
  the ESP32 from deserializing untrusted bodies before rejecting them.
- The `auth` lambda capture is by pointer. `AuthManager` is a framework singleton
  owned by `Application` and outlives all route handler invocations — safe to
  capture as raw pointer.

---

## Handoff to WO7

After WO5, the portal is fully authenticated when auth is enabled. WO7 (portal UI)
can then build the login overlay JS knowing that:
- `GET /api/auth/status` reliably returns auth state
- `POST /api/auth/login` correctly issues a cookie
- All other API calls return 401 when not logged in (the overlay intercepts these)
