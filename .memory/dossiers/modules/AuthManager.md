# modules/AuthManager

## Invariants & contracts

### [docseed-06e19bf1] **Bearer token** — `Authorization: Bearer <token>` header. The token is the stored password hash, retrievable via `GET /api/auth/mcp-token` (requires an active portal session).
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-08a7af25] Generated as 16 bytes of `esp_random()` (or `std::random_device` on native builds).
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-198de871] **Check `AuthManager` middleware** → Verify all sensitive endpoints are protected. Check the path whitelist for overly broad exceptions.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-1d85e04a] **Auth disabled:** Returns `true` unconditionally — open access.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-30b08b74] **Auth enabled:** Validates the session cookie first (browser-based MCP access), then falls back to `Authorization: Bearer <token>` header validation (constant-time comparison).
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-348cd8ea] Validated via constant-time comparison (`_safeMemcmp()`) on every protected request.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-6557bd10] A session is active when `_sessionToken` is not all-zero. Logging out clears it to all-zero.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-6e7bc585] `managedBy`: Identifies the subsystem that registered the secret (e.g. `"WiFi"`, `"AuthManager"`).
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-70c69835] **Storage:** DER-encoded at `/etc/server.key` and `/etc/server.crt`
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-73a81ce2] Looks for an `Authorization: Bearer <token>` HTTP header
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-7de70cf5] Falls back to checking for an `auth_token` cookie
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-85efcea0] **Session cookie** — `esp32_session=<hex>` (for browser-based MCP access).
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/mcp-server.md · evidence: doc:docs/developer/mcp-server.md`

### [docseed-8ee1dc57] **Concurrency:** Generation uses an `std::atomic<bool>` guard (`_certGenRunning`) to prevent concurrent generation
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-902b7ea9] You do **NOT** need to write authentication verification code inside individual route handlers. All `/api/` routes are automatically protected by the global JWT middleware. Simply register your routes via `registerRoutes()` and the framework handles security.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-9067ca44] Generating JWT tokens upon successful login
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-9c580ab5] **Static buffers:** Certificate loaded into a 1024-byte static buffer, key into a 256-byte static buffer (avoids heap fragmentation)
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-ab4a6112] Validating incoming JWT tokens on subsequent API requests
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-e5558e2f] Delivered to the client as a cookie: `esp32_session=<32 hex chars>; HttpOnly; SameSite=Strict`.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-ee7f0049] **WiFi Event Binding:** Subscribe to `wifi.ap_started` to automatically enable `AuthManager` AP-mode authentication bypass.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f247a525] **Serial/USB transport:** MCP requests via serial are intentionally NOT guarded — physical serial access is treated as trusted.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`
