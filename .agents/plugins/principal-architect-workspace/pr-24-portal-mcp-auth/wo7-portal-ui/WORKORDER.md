# WO7 — Security Top-Level Tab + Login Overlay + Cert Status UI

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo7-portal-ui`  
**Base:** `pr-24-portal-mcp-auth/wo5-portal-guard` + `wo6-mcp-guard` (both merged)  
**Dependencies:** WO5 + WO6 merged  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Build the portal UI for the authentication feature:
1. **Security tab** (top-level, lock icon in bottom nav, available in AP mode)
2. **Login overlay** (full-page, intercepts 401 responses, reloads the active tab on success)
3. **MCP bearer token display** with copy-to-clipboard
4. **Cert status** info (subject, valid until dates — read from `/api/auth/status` extension)

The portal must handle three states correctly:
- **Auth disabled** (no password): Security tab shows "Set password" form only
- **Auth enabled, not logged in**: login overlay covers all tabs; Security tab form shows after login
- **Auth enabled, logged in**: Security tab shows full management UI (change password, token, cert info, logout)

---

## Acceptance Criteria

- [ ] Security tab visible in bottom nav with lock icon in both AP mode and STA mode
- [ ] Security tab content loads when the tab is activated
- [ ] "Set password" form works when `passwordSet=false` (no auth required)
- [ ] After setting password: `{restartRequired:true}` response shown as restart prompt
- [ ] Login overlay appears on any 401 response from any API call
- [ ] Correct password → overlay dismisses + active tab reloads
- [ ] Wrong password → inline error shown, overlay stays open
- [ ] Logout button clears session → overlay reappears on next API call
- [ ] MCP bearer token displayed (masked, show/copy button)
- [ ] Cert status section shows cert present/not-present state
- [ ] `app.js` loadScripts array includes `auth.js`
- [ ] `WebServerManager` includes the new `auth_js.hpp` PROGMEM asset
- [ ] Existing Playwright tests still pass (no regressions)
- [ ] New Playwright tests: login flow, set-password, logout, 401 interception

---

## New File: `web/portal/auth.js`

Structure:
```
// auth.js — Security tab UI and global login overlay

// ─── 1. Login Overlay ─────────────────────────────────────────────────────
//   - Injected once into <body> on load
//   - Shown when any apiFetch returns 401 (patched into ApiQueue)
//   - Hidden on successful login

// ─── 2. Security Tab (tab-security) ─────────────────────────────────────
//   - Loaded when 'tab-changed' event fires for 'tab-security'
//   - Renders different UI based on auth state from /api/auth/status

// ─── 3. Auth State Polling ────────────────────────────────────────────────
//   - Fetches /api/auth/status once on load and on tab-changed
```

### Login Overlay HTML (injected into body)

```html
<div id="login-overlay" class="login-overlay hidden">
  <div class="login-card">
    <div class="login-icon">🔒</div>
    <h2>Device Portal</h2>
    <p class="login-subtitle">Enter your password to continue</p>
    <form id="login-form">
      <div class="input-group">
        <input type="password" id="login-password" placeholder="Password"
               autocomplete="current-password" autofocus>
      </div>
      <div id="login-error" class="login-error hidden"></div>
      <button type="submit" id="login-submit" class="btn-primary">Unlock</button>
    </form>
  </div>
</div>
```

The overlay uses the same CSS design language as the rest of the portal (CSS vars,
dark mode support). The login card should be visually polished — centred, slightly
glassmorphic card with a prominent lock icon.

### Global 401 Intercept

Patch `ApiQueue.processNext()` to intercept 401 responses before rejecting them:
```js
if (response.status === 401) {
    showLoginOverlay();
    // Store the failed request so it can be retried after login
    pendingRetryAfterLogin = { url: item.url, options: item.options,
                               resolve: item.resolve, reject: item.reject };
    return; // do not reject — wait for login
}
```

After successful login:
```js
if (pendingRetryAfterLogin) {
    var retry = pendingRetryAfterLogin;
    pendingRetryAfterLogin = null;
    api.enqueue(retry.url, retry.options).then(retry.resolve).catch(retry.reject);
}
```

### Security Tab Content

Render based on `/api/auth/status` response:

**State A — auth.enabled=false, passwordSet=false:**
```html
<div class="security-section">
  <h3>Authentication</h3>
  <p class="muted">Authentication is currently disabled. Set a password to
     secure the portal and enable HTTPS.</p>
  <div class="form-group">
    <label>New Password</label>
    <input type="password" id="new-password" placeholder="Choose a password">
  </div>
  <div class="form-group">
    <label>Confirm Password</label>
    <input type="password" id="confirm-password" placeholder="Confirm password">
  </div>
  <button id="set-password-btn" class="btn-primary">Enable Authentication</button>
</div>
```

**State B — auth.enabled=true, logged in:**
```html
<!-- Cert status section -->
<div class="security-section">
  <h3>TLS Certificate</h3>
  <div class="cert-status">
    <span class="badge badge-ok">✓ Self-signed cert active</span>
    <p class="muted">Certificate subject: CN=<hostname>.local</p>
  </div>
</div>

<!-- MCP Bearer Token section -->
<div class="security-section">
  <h3>MCP Bearer Token</h3>
  <p class="muted">Use this token in your AI agent config to authenticate MCP requests.</p>
  <div class="token-display">
    <code id="mcp-token-value" class="token-masked">••••••••••••••••</code>
    <button id="reveal-token-btn" class="btn-icon" title="Show/hide token">👁</button>
    <button id="copy-token-btn" class="btn-icon" title="Copy to clipboard">📋</button>
  </div>
</div>

<!-- Change Password section -->
<div class="security-section">
  <h3>Change Password</h3>
  <div class="form-group">
    <input type="password" id="new-password" placeholder="New password">
  </div>
  <div class="form-group">
    <input type="password" id="confirm-password" placeholder="Confirm new password">
  </div>
  <button id="change-password-btn" class="btn-secondary">Change Password</button>
</div>

<!-- Logout section -->
<div class="security-section">
  <button id="logout-btn" class="btn-danger">Log Out</button>
</div>
```

**Cert info extension to `/api/auth/status`:**

The status endpoint (modified in this WO) adds cert metadata when `certPresent=true`:
```json
{
  "enabled": true,
  "sessionActive": true,
  "passwordSet": true,
  "certPresent": true,
  "certSubject": "CN=esp32-device.local",
  "certNotAfter": "2040-01-01"
}
```

This requires `AuthManager::registerRoutes()` to return the cert metadata in the
status response — implement this as part of WO7 using mbedTLS cert-parse APIs to
read the stored DER and extract the subject/validity fields. If this is complex,
a simpler alternative: store the hostname used for cert generation in
`/mod/auth.json` as `"certSubject"` at generation time (WO3 already saves config
after cert gen — just add the field).

**Recommended:** Store `certSubject` in `/mod/auth.json` during cert generation (simpler,
no mbedTLS parse on every status request).

---

## `app.js` Changes

### `loadScripts()` addition

Add `'auth.js'` to the scripts array (before `appconfig.js`):
```js
'auth.js',
```

### 401 intercept wiring

The intercept patches into `ApiQueue`. Since `auth.js` loads after `app.js`,
`auth.js` must call a registration function exposed by `app.js`:
```js
// app.js exposes:
window.registerAuthInterceptor = function(showOverlayFn) {
    _authInterceptor = showOverlayFn;
};
```
`auth.js` calls `window.registerAuthInterceptor(showLoginOverlay)` on load.
`ApiQueue.processNext()` checks `_authInterceptor && response.status === 401` and
calls the interceptor instead of rejecting.

---

## `index.html` Change

The Security tab `<div>` and bottom-nav `<a>` were added in WO1. WO7 verifies
they are in place. No new HTML changes required unless WO1 omitted them.

---

## `WebServerManager.cpp` — New Portal Asset

After `auth.js` is written and tested, run the portal asset build script
(`scripts/compress_portal.py` or equivalent) to generate `auth_js.hpp`. Add the
include to `WebServerManager.cpp` and register the route:
```cpp
#include "../include/portal_assets/auth_js.hpp"

server.on("/auth.js", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* r = req->beginResponse(200, "application/javascript",
                                                    auth_js_gz, auth_js_gz_len);
    r->addHeader("Content-Encoding", "gzip");
    req->send(r);
});
```

This route must be in the **exempt** list (not auth-gated) — same as all static assets.

---

## Playwright Tests

New test file: `tests/portal/auth.spec.ts`

Required tests:
- `should show Security tab in bottom nav`
- `should show set-password form when auth disabled`
- `should enable auth after setting password`
- `should show login overlay when 401 received`
- `should dismiss overlay and retry after correct login`
- `should show inline error on wrong password`
- `should logout and show overlay on next API call`
- `should display MCP bearer token with reveal/copy`
- `should show cert status when cert present`

---

## Implementation Notes

- Password validation client-side: non-empty, minimum 8 chars, passwords match.
  Do not over-validate — server is authoritative. Client-side validation is UX only.
- The token display uses `type="password"`-style masking via CSS (`letter-spacing`,
  `font-family: monospace`, `filter: blur()`). The reveal button toggles a class.
- Copy-to-clipboard: use `navigator.clipboard.writeText()` (modern, no polyfill
  needed — embedded portal is used from modern browsers only).
- The login overlay CSS must use `z-index` above the bottom nav and content container.
  The existing `#toast-container` uses high z-index — check for conflicts.
- `auth.js` uses `window.apiFetch` for all API calls (serialized through ApiQueue).
- The "restart required" message after first password set should use the existing
  toast notification system (`showToast()` from `utils.js`) plus a prominent
  inline banner in the Security tab.
