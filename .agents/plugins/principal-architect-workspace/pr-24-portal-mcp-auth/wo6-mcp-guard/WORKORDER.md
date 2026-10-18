---
name: pr-24-wo6
description: Handoff instructions for PR-24 Work Order 6 — MCP server HTTP bearer token authentication
complexity: standard
---

# WO6 — MCP Server Bearer Token Auth

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo6-mcp-guard`  
**Base:** `pr-24-portal-mcp-auth/wo2-auth-crypto`  
**Dependencies:** WO2 merged (can run in parallel with WO3 + WO4)  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Gate the MCP HTTP endpoints (`/api/mcp`, `/api/mcp/status`, `/api/mcp/config`)
behind a bearer token check. The bearer token is the hex SHA-256 hash of the
portal password (already stored as `"portal_password_hash"` in KeyManager) — no
extra state, no new credential. The Serial/USB MCP transport is explicitly
left unauthenticated.

---

## Acceptance Criteria

- [ ] When `auth.enabled = false`: all MCP routes remain open (status quo)
- [ ] When `auth.enabled = true`:
  - No `Authorization` header → 401
  - `Authorization: Bearer <wrong-token>` → 401
  - `Authorization: Bearer <correct-token>` → normal MCP response
- [ ] `mcpReadOnly` semantics preserved and extended:
  - `auth.enabled=false` → readOnly gate only
  - `auth.enabled=true`, valid bearer, `mcpReadOnly=true` → reads only (writes return 403)
  - `auth.enabled=true`, valid bearer, `mcpReadOnly=false` → full access
- [ ] Serial `_processLine()` path explicitly NOT guarded (documented in code comment)
- [ ] `pio run -e native` — all tests pass
- [ ] New native tests: 401 without bearer; 401 with wrong bearer; 200 with correct bearer;
      readOnly enforcement with valid bearer

---

## `AuthManager::checkMcpAuth()` — full implementation

```cpp
bool AuthManager::checkMcpAuth(AsyncWebServerRequest* request) {
    if (!_authEnabled) return true;

    // Extract bearer token from Authorization header
    if (!request->hasHeader("Authorization")) {
        request->send(401, "application/json", "{\"error\":\"missing_authorization\"}");
        return false;
    }

    String authHeader = request->header("Authorization");
    if (!authHeader.startsWith("Bearer ")) {
        request->send(401, "application/json", "{\"error\":\"invalid_auth_scheme\"}");
        return false;
    }

    String bearerToken = authHeader.substring(7);  // strip "Bearer "
    bearerToken.trim();

    // Load stored hash (which IS the bearer token)
    std::string storedHash = _keyManager->getKeyById("portal_password_hash");
    if (storedHash.empty()) {
        request->send(401, "application/json", "{\"error\":\"no_credentials_set\"}");
        return false;
    }

    // Constant-time compare
    if (bearerToken.length() != storedHash.length() ||
        mbedtls_ssl_safer_memcmp(bearerToken.c_str(),
                                 storedHash.c_str(),
                                 storedHash.length()) != 0) {
        request->send(401, "application/json", "{\"error\":\"invalid_token\"}");
        return false;
    }

    return true;
}
```

For native builds: replace `mbedtls_ssl_safer_memcmp` with a volatile XOR
accumulator constant-time compare.

---

## `MCPServer` Changes

### `MCPServer.hpp`

Add `AuthManager*` parameter to `begin()`:
```cpp
void begin(const std::vector<IMCPContributor*>& contributors,
           ConfigManager* configManager = nullptr,
           TimeManager*   timeManager   = nullptr,
           WifiManager*   wifiManager   = nullptr,
           AuthManager*   authManager   = nullptr);   // NEW — nullable, backward compatible
```

Add private member:
```cpp
AuthManager* _authManager = nullptr;
```

### `MCPServer.cpp`

In `begin()`: `_authManager = authManager;`

In `registerRoutes()`, add bearer guard to all three MCP HTTP routes:

**`POST /api/mcp`** (main JSON-RPC endpoint):
```cpp
server.on("/api/mcp", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        // NOTE: Serial transport (_processLine) is intentionally NOT guarded —
        // Serial/USB access is physical access and is treated as trusted.
        if (_authManager && !_authManager->checkMcpAuth(req)) return;
        processRequest(req, data, len, index, total);
    }
);
```

**`GET /api/mcp/status`**:
```cpp
server.on("/api/mcp/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (_authManager && !_authManager->checkMcpAuth(req)) return;
    // ... existing status handler ...
});
```

**`POST /api/mcp/config`**:
```cpp
server.on("/api/mcp/config", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        if (_authManager && !_authManager->checkMcpAuth(req)) return;
        // ... existing config handler ...
    }
);
```

### `mcpReadOnly` extended semantics documentation

Update the `_mcpReadOnly` field doc in `MCPServer.hpp`:
```cpp
/**
 * When auth.enabled = false: mcpReadOnly gates write operations regardless of caller.
 * When auth.enabled = true:  unauthenticated requests are rejected (401) before
 *                            mcpReadOnly is evaluated. Authenticated requests with
 *                            mcpReadOnly=true are restricted to read-only tool calls.
 *                            Authenticated requests with mcpReadOnly=false have full access.
 */
```

---

## `Application.cpp` Changes

Pass `_authManager` to `MCPServer::begin()`:
```cpp
_mcpServer.begin(_mcpContributors, &_configManager, &_timeManager,
                 &_wifiManager, &_authManager);
```

---

## Native Tests

File: `test/native/test_mcp_auth/test_mcp_auth.cpp` (new file)

Required test cases:
- `test_mcp_guard_open_when_auth_disabled` — `checkMcpAuth()` returns true when `_authEnabled=false`
- `test_mcp_guard_blocks_missing_header` — 401 when no Authorization header
- `test_mcp_guard_blocks_wrong_token` — 401 when bearer != stored hash
- `test_mcp_guard_passes_correct_token` — true when bearer == stored hash
- `test_mcp_guard_blocks_non_bearer_scheme` — 401 when scheme is not "Bearer"
- `test_mcp_readonly_with_valid_bearer` — read-only enforcement still applies after auth

---

## Implementation Notes

- `MCPServer` is the only contributor that uses `checkMcpAuth()` rather than
  `checkPortalAuth()`. The distinction: bearer token is stateless (no cookie),
  suitable for programmatic AI agent access.
- The `nullptr` default for `authManager` in `MCPServer::begin()` ensures
  all native tests that don't pass an `AuthManager` compile and run unchanged.
- The guard `if (_authManager && ...)` — when `_authManager` is null (native tests
  without auth wired), the guard is skipped entirely. This preserves existing
  test compatibility.
- Token comparison must be constant-time. `String` comparison in Arduino/ESP-IDF is
  NOT constant-time. Use `mbedtls_ssl_safer_memcmp` after length check.
