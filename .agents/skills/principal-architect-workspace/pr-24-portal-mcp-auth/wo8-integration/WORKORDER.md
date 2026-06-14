# WO8 — Integration + Hardware Verification

**Project:** PR-24 — Portal + MCP Auth + TLS  
**Branch:** `pr-24-portal-mcp-auth/wo8-integration`  
**Base:** `pr-24-portal-mcp-auth/wo7-portal-ui` (all WO1–WO7 merged)  
**Dependencies:** WO1–WO7 all merged  
**Design doc:** [DESIGN.md](../DESIGN.md)

---

## Objective

Final integration gate. Verify the complete PR-24 feature set end-to-end on
hardware, confirm native and Playwright test suites are fully green, update
programme documentation, and prepare the PR for merge.

This WO does not write new feature code. It may make small fixes to issues
discovered during hardware testing.

---

## Acceptance Criteria

### Automated
- [ ] `pio run -e native` — **all tests green** (525+ existing + PR-24 additions)
- [ ] `npm test` (Playwright) — **all tests green** (61 existing + PR-24 additions)
- [ ] `pio run -e esp32dev` — **clean build**, no warnings promoted to errors
- [ ] `pio run -e esp32s3nano` — **clean build**, no warnings promoted to errors

### Hardware — esp32dev
- [ ] **H1** Flash PR-24 firmware. Open `http://<ip>/` → portal on port 80, no auth.
      All tabs load normally. Security tab visible with lock icon.
- [ ] **H2** Navigate to Security tab → "Authentication disabled. Set a password to enable."
      form shown.
- [ ] **H3** Set password "testPass1!" → success response + "Restart required" message shown.
- [ ] **H4** Restart device (via portal restart button or power cycle).
- [ ] **H5** Open `http://<ip>/` → 301 redirect to `https://<ip>/`.
- [ ] **H6** Open `https://<ip>/` in Chrome → cert warning shown. Click "Advanced → Proceed".
- [ ] **H7** Login overlay appears. Enter wrong password → inline error. Enter correct password
      → overlay dismisses, active tab content loads.
- [ ] **H8** Open `https://<ip>/api/auth/status` (with session cookie) → 200 JSON.
- [ ] **H9** Navigate to Security tab → cert status shown, MCP token displayed (masked).
      Reveal button shows token. Copy button copies to clipboard.
- [ ] **H10** MCP authenticated access:
      `curl --insecure -X POST https://<ip>/api/mcp -H "Authorization: Bearer <token>" -d '{"jsonrpc":"2.0","method":"system_info","id":1}'` → 200 JSON-RPC response.
- [ ] **H11** MCP unauthenticated access:
      `curl --insecure -X POST https://<ip>/api/mcp -d '{"jsonrpc":"2.0","method":"system_info","id":1}'` → 401.
- [ ] **H12** Logout from Security tab → session cleared → next API call shows login overlay.
- [ ] **H13** HTTP redirect: `curl -v http://<ip>/api/wifi/status` → 301 Moved Permanently
      with `Location: https://<ip>/api/wifi/status`.
- [ ] **H14** Heap guard: during TLS handshake on esp32dev, confirm heap does not fall
      below the existing guard threshold. Check via `/api/system` `heap_free` field.
      If heap drops below threshold, document the minimum observed free heap.
- [ ] **H15** OTA still works: trigger GitHub check via portal (logged in) →
      OTA task starts (check log for "GitHub OTA check triggered via API" message).
      Outbound HTTPS to GitHub succeeds.
- [ ] **H16** Factory reset: `POST /api/system/factory-reset` (while logged in) →
      device restarts → comes up on HTTP port 80, no auth, Security tab shows
      "Authentication disabled."
- [ ] **H17** AP mode HTTP: force AP mode (disconnect from WiFi via WiFi tab → remove SSID,
      restart). Connect to device AP → open `http://192.168.4.1/` → portal on port 80,
      no auth, Security tab accessible.

### Hardware — esp32s3nano
- [ ] **S1** Flash PR-24 firmware. Repeat H1–H16 (skip H14 heap check — PSRAM available,
      no heap pressure concern). Confirm TLS handshake completes without restart.

---

## Documentation Updates

### `MEMORY.md`

Add PR-24 entry to the programme index:
```markdown
### PR-24 — Web Portal + MCP Server Authentication + TLS
**Status:** COMPLETE  
**Branch merged:** `pr-24-portal-mcp-auth` → `main`  
**New modules:** `AuthManager`  
**Key changes:**
- Session cookie auth (HTTPS port 443) + bearer token (MCP HTTP)
- Self-signed EC P-256 cert via mbedTLS, stored in LittleFS
- `IWebContributor::getTabIcon()` extension
- Security top-level tab in portal (lock icon)
- HTTP in AP mode and when no password set (status quo preserved)
- SHA-256 + per-device salt, credentials in KeyManager
```

### Todo

Mark the following item `[x]` in `.agents/todo/todo.md` (if it exists on the `todo_list` branch):
> "Add password protection to the web portal and MCP server"

---

## MEMORY.md PR-24 Entry Location

Insert after the PR-23 entry in `.agents/skills/principal-architect-workspace/MEMORY.md`.

---

## Small-Fix Scope

This WO is authorised to make **small code fixes** discovered during hardware testing:
- Off-by-one errors in cookie parsing
- Missing `#ifndef NATIVE_BUILD` guards
- Incorrect HTTP status codes in error responses
- Build warnings (unused variables, deprecated APIs)

Any fix that requires changing the auth guard logic, TLS cert parameters,
or route structure must be escalated to the PA (raise in the session; do not
self-approve architectural changes).

---

## PR Merge Checklist

Before requesting merge to `main`:
- [ ] All acceptance criteria above marked complete
- [ ] `git log --oneline pr-24-portal-mcp-auth..main` — confirm no merge conflicts
- [ ] PR description written summarising the feature
- [ ] Version bump proposed (minor version — PR-24 is a significant security feature)
      but NOT applied until user approval per rule `03-versioning.md`
- [ ] MEMORY.md updated on main (or via PR)
- [ ] Todo item marked if applicable
