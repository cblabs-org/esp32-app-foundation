---
name: pr-24-wo1
description: Handoff instructions for PR-24 Work Order 1 — AuthManager skeleton, IConfigurable wiring, and IWebContributor getTabIcon() extension
complexity: standard
---

# WO1 — AuthManager Skeleton + IWebContributor Icon Extension

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo1-auth-manager`  
**Base:** `main`  
**Dependencies:** none  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Create the `AuthManager` module skeleton with full `IConfigurable` and
`IWebContributor` implementations (routes stub returning 501 until WO2). Wire
it into `Application`. Simultaneously extend `IWebContributor` with a new
`getTabIcon()` method and propagate it through `WebServerManager` and the
`/api/tabs` response.

No cryptography in this WO. No TLS. No auth enforcement. Just the module
scaffold, config persistence, and the interface extension.

---

## Acceptance Criteria

- [ ] `AuthManager` compiles for both hardware and native targets
- [ ] `IConfigurable` round-trip: `serializeConfig()` → `applyConfig()` preserves
      all three fields (`enabled`, `passwordSet`, `certPresent`)
- [ ] `resetConfig()` sets all flags false and calls `deleteKey()` on both KeyManager entries
- [ ] `AuthManager` registered in `Application::begin()` before `beginAll()`
- [ ] `/api/auth/status` returns `{enabled:false, sessionActive:false, passwordSet:false, certPresent:false}`
- [ ] `/api/auth/login`, `/api/auth/logout`, `/api/auth/password`, `/api/auth/mcp-token`
      all return `501 Not Implemented` (stubs — will be implemented in WO2)
- [ ] `getTabIcon()` on `IWebContributor` compiles with default `nullptr` return
- [ ] `AuthManager::getTabIcon()` returns the lock SVG path string
- [ ] `/api/tabs` response includes `"icon"` field for tabs that declare one
- [ ] `app.js` `buildBottomNav()` uses `tab.icon` if present (fallback to generic icon)
- [ ] Security tab appears in bottom nav with lock icon when `auth.js` is loaded
- [ ] `pio run -e native` — all existing 525+ tests still pass
- [ ] New native tests: config round-trip (3 fields), resetConfig clears state,
      getConfigNamespace returns "auth", getSchemaVersion returns 1

---

## Files to Create

### `modules/AuthManager/AuthManager.hpp`
Full house-style header. Class `AuthManager : public IWebContributor, public IConfigurable`.

Key members:
```cpp
private:
    bool     _authEnabled   = false;
    bool     _passwordSet   = false;
    bool     _certPresent   = false;
    uint8_t  _sessionToken[16];   // all-zero = no session
    ConfigManager* _configManager = nullptr;
    KeyManager*    _keyManager    = nullptr;
```

Public API (stubs in this WO — real impl in WO2/WO3):
```cpp
void begin(ConfigManager* configManager, KeyManager* keyManager);

// Auth guards (WO2/WO5 implement these — return true here for now)
bool checkPortalAuth(AsyncWebServerRequest* request);
bool checkMcpAuth(AsyncWebServerRequest* request);

// Cert helpers (WO3 implements — stubs here)
bool generateAndStoreCert(const char* hostname);
bool hasCert() const { return _certPresent; }
bool loadCert(uint8_t** certDer, size_t* certLen,
              uint8_t** keyDer,  size_t* keyLen) const;

// IWebContributor
void registerRoutes(AsyncWebServer& server) override;
const char* getTabName()   const override { return "Security"; }
const char* getParentTab() const override { return nullptr; }   // top-level tab
int         getTabOrder()  const override { return 5; }          // before WiFi(10)? or after... TBD
const char* getTabIcon()   const override;  // returns lock SVG path

// IConfigurable
const char* getConfigNamespace() const override;
int         getSchemaVersion()   const override;
void        applyConfig(const JsonObject& section)  override;
void        serializeConfig(JsonObject& section) const override;
void        resetConfig() override;
```

> **Tab order note:** Security at order 5 puts it before WiFi (currently leftmost).
> A lock icon as the first tab makes sense UX-wise (visible immediately, emphasises
> security). Implementer should confirm with a visual check. Alternatively order 50
> puts it after System (rightmost). Choose whichever looks better in the nav.

### `modules/AuthManager/AuthManager.cpp`

Implement `begin()`, `registerRoutes()` (501 stubs for all auth routes),
`getTabIcon()` (return the SVG path `d` attribute string for a padlock),
and all IConfigurable methods.

`getTabIcon()` returns the inner SVG content (path elements only, no outer `<svg>` tag).
Use the feather-icons lock path — it matches the existing stroke-based icon style:
```
M19 11H5a2 2 0 0 0-2 2v7a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7a2 2 0 0 0-2-2z
M7 11V7a5 5 0 0 1 10 0v4
```
(Two `<path>` elements — implementer embeds the full inner SVG as a string literal.)

---

## Files to Modify

### `include/interfaces/IWebContributor.hpp`

Add after `getTabOrder()`:
```cpp
/**
 * @brief Return the inner SVG content string for this tab's bottom-nav icon.
 *
 * Return nullptr to use the generic grid icon injected by app.js for
 * dynamically contributed tabs. When non-null, the string is included in
 * the /api/tabs response as the "icon" field and used by app.js to render
 * the icon. Only the SVG body is returned (path/circle/rect elements);
 * the outer <svg viewBox="0 0 24 24" ...> wrapper is added by the client.
 *
 * @return SVG inner content string, or nullptr for the generic icon.
 */
virtual const char* getTabIcon() const { return nullptr; }
```

Update the file header doc to include `getTabIcon()` in the exported API list.

### `modules/WebServer/WebServerManager.cpp`

In the tab-building loop (lines ~202–211), add icon to tab object:
```cpp
const char* icon = contributor->getTabIcon();
if (icon != nullptr) tabObj["icon"] = icon;
```

### `web/portal/app.js`

In `buildBottomNav()`, replace the hardcoded generic icon SVG with:
```js
var iconInner = tab.icon || DEFAULT_GENERIC_ICON;
link.innerHTML = '<span class="icon"><svg viewBox="0 0 24 24" fill="none" ' +
  'stroke="currentColor" stroke-width="2" stroke-linecap="round" ' +
  'stroke-linejoin="round">' + iconInner + '</svg></span>' + tab.name.toUpperCase();
```
Where `DEFAULT_GENERIC_ICON` is the existing rectangle grid path (extracted as a
named constant at the top of `buildBottomNav()`).

### `web/portal/index.html`

Add Security tab `<div>` to the tab-content-container (before WiFi or after System
— match the order chosen for `getTabOrder()`):
```html
<!-- Security tab (top-level, contributed by AuthManager) -->
<div id="tab-security" class="tab-content">
  <!-- Populated by auth.js -->
</div>
```

Add Security to the bottom nav **hardcoded** (same pattern as WiFi/System — both
icons and skip logic will remain until a future cleanup PR):
```html
<a href="#" class="tab-link" data-target="tab-security">
  <span class="icon">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"
         stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
      <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
      <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
    </svg>
  </span>
  SECURITY
</a>
```

Add `"tab-security"` to the skip-list in `buildBottomNav()`:
```js
if (tabId === 'tab-wifi' || tabId === 'tab-system' || tabId === 'tab-security') continue;
```

### `modules/Application/Application.hpp`

Add `AuthManager _authManager;` member (forward-declare in header, include in .cpp).

### `modules/Application/Application.cpp`

Wire in per the integration points in DESIGN.md (Step 7, Step 14).

---

## Native Tests

New test file: `test/native/test_auth_manager/test_auth_manager.cpp`

Required test cases:
- `test_config_namespace` — `getConfigNamespace()` returns `"auth"`
- `test_schema_version` — `getSchemaVersion()` returns `1`
- `test_serialize_defaults` — fresh instance serializes `{enabled:false, passwordSet:false, certPresent:false}`
- `test_apply_config_roundtrip` — serialize → apply → serialize produces identical JSON
- `test_reset_config` — after setting flags true, `resetConfig()` returns all false
- `test_tab_name` — `getTabName()` returns `"Security"`
- `test_parent_tab` — `getParentTab()` returns `nullptr`
- `test_tab_icon_not_null` — `getTabIcon()` returns non-null string
- `test_check_portal_auth_open` — when `_authEnabled = false`, `checkPortalAuth()` returns true
- `test_check_mcp_auth_open` — when `_authEnabled = false`, `checkMcpAuth()` returns true

---

## Implementation Notes

- Follow house-style documentation exactly (see `house-style-docs` skill if needed)
- `AuthManager` stubs for `generateAndStoreCert()` and `loadCert()` should return
  `false` / do nothing — WO3 provides the real implementation
- `checkPortalAuth()` and `checkMcpAuth()` should return `true` in this WO
  (no enforcement until WO2/WO5) — this is safe because no routes are protected yet
- Native build: `AsyncWebServerRequest*` in the guard stubs can be a `nullptr` or
  mock — guards don't fire in this WO
- The `app.js` change to `buildBottomNav()` must not break the existing skip
  logic for WiFi and System

---

## Handoff to WO2

WO2 requires:
- `AuthManager` class exists with `_passwordSet`, `_authEnabled`, `_sessionToken`
- `begin(ConfigManager*, KeyManager*)` wires dependencies
- `KeyManager::addKey()`, `getKeyById()`, `deleteKey()` accessible via `_keyManager`
- All auth API routes registered (501 stub handlers are fine — WO2 replaces them)
