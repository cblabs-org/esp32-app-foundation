---
name: pr-29-wo8
description: Handoff instructions for Work Order 8 — Separate password/auth from TLS in Security tab
complexity: standard
---

# Work Order 8 — Separate Password/Auth from TLS in Security Tab

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Decouple password authentication from TLS so that setting a portal password does
NOT implicitly enable HTTPS. On `esp32dev` (no PSRAM, ~320 KB SRAM), the TLS
web server consumes ~30-40 KB resident DRAM, making OTA impossible under memory
pressure. Password-protected HTTP is acceptable on local/home networks.

## Versioning

Read `.agents/rules/03-versioning.md`. This includes new functionality
(independent auth/TLS controls) — a dev tag increment is warranted when building.

## Strict Boundary Box

- You **MAY** modify `modules/AuthManager/AuthManager.hpp`
- You **MAY** modify `modules/AuthManager/AuthManager.cpp`
- You **MAY** modify `modules/WebServer/WebServerManager.hpp`
- You **MAY** modify `modules/WebServer/WebServerManager.cpp`
- You **MAY** modify `web/portal/auth.js`
- You **MAY** modify `web/portal/app.css` (if styling changes needed)
- Do **NOT** modify `sdkconfig.defaults`, `platformio.ini`, or partition tables
- Do **NOT** create any branches or make git commits

## Background

### Current Coupling

In `WebServerManager.cpp:103-146`, TLS mode is enabled when:
```cpp
bool useTls = (!apMode || apTlsEnabled) && authEnabled && hasCert;
```

This means: password enabled + cert present = TLS auto-enabled. There is no way
to have a password without TLS.

### Target State

Two independent controls in the Security tab:
1. **Password & MCP Token** — enables session cookie auth + MCP bearer token
   over HTTP (port 80). No TLS required.
2. **TLS/HTTPS** — optionally enables HTTPS on port 443 with self-signed cert.
   Requires cert generation, independent of password.

### Password Flow (current)

1. User sets password via `POST /api/auth/password`
2. `AuthManager` hashes password, stores in NVS, generates cert
3. Returns `{restartRequired: true, reason: "restart_for_tls"}`
4. `auth.js` calls `POST /api/system/restart`
5. On reboot, `WebServerManager::begin()` sees `authEnabled && hasCert` → starts HTTPS

### Password Flow (target)

1. User sets password via `POST /api/auth/password`
2. `AuthManager` hashes password, stores in NVS. Does NOT auto-generate cert.
3. Returns `{restartRequired: false}` (no restart needed for password-only change)
4. Auth is active immediately via session cookies on HTTP
5. TLS is a separate toggle: user can later generate cert and enable TLS independently

## Line-Item Instructions

### Step 1: Decouple cert generation from password setting

- [ ] **1a.** Read `modules/AuthManager/AuthManager.cpp` — find the `POST /api/auth/password`
  handler (~line 900-908).

- [ ] **1b.** Remove or guard the automatic cert generation call that happens
  when a password is set. The password handler should:
  - Hash and store the password
  - Enable auth (`_authEnabled = true`)
  - Save config
  - Return `{restartRequired: false}` if no TLS change is needed
  - Only return `{restartRequired: true}` if TLS state actually needs to change

### Step 2: Decouple TLS decision in WebServerManager

- [ ] **2a.** Read `modules/WebServer/WebServerManager.cpp` — find the TLS
  mode decision (~line 103-146).

- [ ] **2b.** Change the TLS decision to be based on a separate `tlsEnabled`
  config flag rather than the `authEnabled` flag:
  ```cpp
  bool useTls = tlsEnabled && hasCert;
  ```
  Where `tlsEnabled` is a new config field that the user controls independently.

### Step 3: Add TLS toggle to AuthManager config

- [ ] **3a.** Add a `_tlsEnabled` field to `AuthManager` (default `false`).

- [ ] **3b.** Persist it in the config schema. Consider bumping the schema
  version if the migration requires it.

- [ ] **3c.** Add a new API endpoint `POST /api/auth/tls` to toggle TLS on/off:
  ```json
  {"enabled": true}
  ```
  When enabling, generate cert if not present. Return `{restartRequired: true}`
  since the server needs to restart to bind port 443.

- [ ] **3d.** Update `GET /api/auth/status` to include `tlsEnabled` in the response.

### Step 4: Update Security tab UI

- [ ] **4a.** Read `web/portal/auth.js` to understand the current Security tab layout.

- [ ] **4b.** Split the Security tab into two visual sections:
  1. **Password & MCP Token** section — password set/change, MCP bearer token reveal
  2. **TLS/HTTPS** section — toggle switch for TLS, cert generation button, cert status

- [ ] **4c.** The password section should work independently of TLS:
  - Setting a password should NOT trigger cert generation
  - Setting a password should NOT require a device restart
  - Session cookies work over HTTP (no `Secure` flag when TLS is off)

- [ ] **4d.** The TLS section should show:
  - On/off toggle
  - Current cert status (generated, expiry date, or "no cert")
  - "Generate Certificate" button (only when TLS is being enabled)
  - Warning: "Enabling TLS requires ~30-40 KB additional RAM"

### Step 5: Handle session cookie security

- [ ] **5a.** When TLS is disabled, the session cookie must NOT have the `Secure`
  flag (browsers won't send `Secure` cookies over HTTP). Check
  `AuthManager::_generateSessionToken()` for cookie flags.

- [ ] **5b.** When TLS is enabled, the `Secure` flag should be set.

### Step 6: Build and verify

- [ ] **6a.** Build for esp32s3nano:
  ```bash
  pio run -e esp32s3nano
  ```

- [ ] **6b.** Build for esp32dev:
  ```bash
  pio run -e esp32dev
  ```

- [ ] **6c.** Run native tests:
  ```bash
  pio test -e native
  ```

### Step 7: Verify behavior

- [ ] **7a.** Flash to a test device and verify:
  1. Set password without enabling TLS → portal is password-protected on HTTP
  2. Enable TLS separately → device restarts, portal serves on HTTPS
  3. Disable TLS → device restarts, portal falls back to HTTP (still password-protected)

## Verification Requirement

After completing all checklist items:
1. Confirm both boards build cleanly
2. Confirm native tests pass
3. Confirm password can be set independently of TLS
4. Confirm TLS toggle works independently of password
5. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
