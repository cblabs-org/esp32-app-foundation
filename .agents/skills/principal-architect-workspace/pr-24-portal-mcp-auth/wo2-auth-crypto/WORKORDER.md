---
name: pr-24-wo2
description: Handoff instructions for PR-24 Work Order 2 — SHA-256 password hashing, session token lifecycle, all /api/auth/* route implementations
complexity: standard
---

# WO2 — SHA-256 Password Store + Auth API Routes

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo2-auth-crypto`  
**Base:** `pr-24-portal-mcp-auth/wo1-auth-manager`  
**Dependencies:** WO1 merged  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Implement the full authentication logic in `AuthManager`: SHA-256 password hashing
with per-device salt, session token lifecycle, and all five `/api/auth/*` endpoints.
After this WO the firmware can authenticate portal users, but TLS is not yet active
(WO4) and route guards are not yet wired (WO5). The auth API itself works over HTTP.

---

## Acceptance Criteria

- [ ] `POST /api/auth/password` sets a password: salt generated, hash computed,
      both stored in `KeyManager`, `_passwordSet=true`, `_authEnabled=true`
- [ ] `POST /api/auth/login` with correct password returns 200 + session cookie
- [ ] `POST /api/auth/login` with wrong password returns 401
- [ ] `POST /api/auth/logout` clears `_sessionToken` (all-zero), returns 200
- [ ] `GET /api/auth/status` returns `{enabled, sessionActive, passwordSet, certPresent}`
      with correct live values
- [ ] `GET /api/auth/mcp-token` returns `{token: "<hex>"}` when authenticated,
      401 when not
- [ ] `resetConfig()` removes both KeyManager entries and resets all flags
- [ ] Session token: 16 bytes from `esp_random()`, stored in `_sessionToken`
- [ ] Cookie header format: `esp32_session=<32-hex-chars>; HttpOnly; SameSite=Strict`
      (no `Secure` attribute yet — TLS not active until WO4; added in WO4/WO5)
- [ ] `pio run -e native` — all tests pass (525+ existing + new)
- [ ] New native tests: correct password → token issued; wrong password → no token;
      logout clears token; mcp-token derivation; resetConfig removes keys

---

## Crypto Specification

### Password storage
```
salt         = 16 bytes from esp_random()
stored_hash  = SHA-256( salt ‖ password )
KeyManager:
  "portal_password_salt"  ← hex(salt)    [16 bytes → 32 hex chars]
  "portal_password_hash"  ← hex(hash)    [32 bytes → 64 hex chars]
```

### Session token
```
token        = 16 bytes from esp_random()
cookie_value = hex(token)              [32 hex chars]
```

### MCP bearer token (deterministic, no extra state)
```
bearer       = hex(SHA-256( salt ‖ password ))
             = the same value as stored_hash
```
This means the bearer token equals the stored hash. The client computes it from
the same password; the server compares the header value directly against the
stored hash. No additional KeyManager entry needed.

### mbedTLS calls (hardware) / stubs (native)
```cpp
// SHA-256 of (salt || password)
mbedtls_md_context_t ctx;
mbedtls_md_init(&ctx);
mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
mbedtls_md_starts(&ctx);
mbedtls_md_update(&ctx, salt, 16);
mbedtls_md_update(&ctx, (const uint8_t*)password, strlen(password));
mbedtls_md_finish(&ctx, hash);
mbedtls_md_free(&ctx);
```

For native builds, provide a stub that:
- `_setPassword()` stores a known test hash (without calling mbedTLS)
- `_verifyPassword()` performs a constant-time `memcmp` of pre-hashed test vectors

---

## Files to Modify

### `modules/AuthManager/AuthManager.hpp`

Add private methods:
```cpp
bool _setPassword(const char* password);
bool _verifyPassword(const char* candidate) const;
void _generateSessionToken();
bool _isSessionActive() const;   // checks _sessionToken != all-zero
bool _checkSessionCookie(AsyncWebServerRequest* request) const;

// Hex encode/decode helpers
static void _toHex(const uint8_t* bin, size_t len, char* out);   // out must be 2*len+1
static bool _fromHex(const char* hex, uint8_t* out, size_t len); // len = byte count
```

### `modules/AuthManager/AuthManager.cpp`

Replace the 501 stub handlers with real implementations for all five routes.

**`POST /api/auth/password`** (body limit 256 B):
```
Fields: {"password": "<string>"}
Requires: sessionActive OR passwordSet == false
Actions:
  1. Validate: password non-empty, length ≤ 64
  2. _setPassword(password)  → generates salt, computes hash, saves to KeyManager
  3. _authEnabled = true, _passwordSet = true
  4. configManager->saveModuleConfig(this)
  5. If certPresent: respond {status:"ok"}
     If !certPresent: respond {status:"ok", restartRequired:true, reason:"cert_pending"}
     (cert generation happens in WO3; this WO just reports the flag state)
```

**`POST /api/auth/login`** (body limit 256 B):
```
Fields: {"password": "<string>"}
Actions:
  1. If !_passwordSet → 401 {error:"no_password_set"}
  2. _verifyPassword(password) → if false → 401 {error:"invalid_password"}
  3. _generateSessionToken()
  4. Respond 200 + Set-Cookie header
```

**`POST /api/auth/logout`**:
```
Actions:
  1. memset(_sessionToken, 0, 16)
  2. Respond 200 + Set-Cookie: esp32_session=; Max-Age=0 (clear cookie)
```

**`GET /api/auth/status`**:
```
Response: {
  "enabled": _authEnabled,
  "sessionActive": _isSessionActive(),
  "passwordSet": _passwordSet,
  "certPresent": _certPresent
}
```

**`GET /api/auth/mcp-token`** (session-gated):
```
Actions:
  1. If !_checkSessionCookie(request) → 401
  2. Load salt from KeyManager "portal_password_salt"
  3. Load stored hash from KeyManager "portal_password_hash"
  4. Respond 200 {token: "<stored_hash_hex>"}
     (stored_hash IS the bearer token — no recomputation needed)
```

---

## Constant-time comparison

Use `mbedtls_ssl_safer_memcmp()` (available in mbedTLS) or implement a manual
constant-time compare for the password hash check. Do NOT use `memcmp()` directly
(timing side-channel).

For native builds, `mbedtls_ssl_safer_memcmp()` may not be available — use a
simple volatile XOR accumulator as the constant-time fallback.

---

## Native Test Specification

File: `test/native/test_auth_manager/test_auth_manager.cpp` (extend from WO1)

Additional test cases:
- `test_set_password_stores_keys` — after `_setPassword()`, KeyManager has both entries
- `test_verify_password_correct` — same password verifies correctly
- `test_verify_password_wrong` — different string fails
- `test_session_token_generated` — after `_generateSessionToken()`, token is non-zero
- `test_logout_clears_token` — `memset` to all-zero after logout
- `test_mcp_bearer_equals_hash` — bearer token = stored hash (verify derivation)
- `test_reset_removes_keys` — `resetConfig()` calls `deleteKey()` for both entries
- `test_set_password_enables_auth` — `_authEnabled` and `_passwordSet` both true after set
- `test_hex_encode_decode` — round-trip `_toHex` + `_fromHex` for 16 and 32-byte inputs

---

## Implementation Notes

- Body size limit for password/login routes: 256 bytes (password + field name ≤ 256)
- Cookie parsing: extract `esp32_session` from the `Cookie:` header. In
  `AsyncWebServerRequest`, use `request->header("Cookie")` and parse manually.
  No external cookie library.
- `_checkSessionCookie()` should be constant-time with respect to the token value
  (use constant-time compare, not early-exit string compare).
- Native build guard: `esp_random()` is not available in native. Use `std::random_device`
  as the native fallback inside `#ifdef NATIVE_BUILD`.
- The `Secure` cookie attribute is intentionally omitted in WO2 — added in WO5 when
  the HTTPS guard is wired. Adding it now would break testing over HTTP.

---

## Handoff to WO3 and WO5

**WO3** (can run in parallel): needs `_authEnabled`, `_passwordSet` flags and
`configManager->saveModuleConfig(this)` pattern — all available after WO2.

**WO5**: needs `checkPortalAuth()` and `checkMcpAuth()` to actually enforce the
session/bearer check. WO2 provides `_checkSessionCookie()` and the bearer
comparison logic; WO5 wires them into the route guards.
