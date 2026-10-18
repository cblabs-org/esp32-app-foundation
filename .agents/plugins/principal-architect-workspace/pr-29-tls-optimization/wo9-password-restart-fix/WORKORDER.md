---
name: pr-29-wo9
description: Handoff instructions for Work Order 9 — Fix device restart on password change
complexity: standard
---

# Work Order 9 — Fix Device Restart on Password Change

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the bug where setting or changing a password in the Security tab causes the
device to fail to restart cleanly. Investigate the root cause and implement a
reliable shutdown sequence.

## Versioning

Read `.agents/rules/03-versioning.md`. This is a bug fix — a dev tag increment
is warranted when building.

## Strict Boundary Box

- You **MAY** modify `modules/AuthManager/AuthManager.cpp`
- You **MAY** modify `modules/WebServer/WebServerManager.cpp`
- You **MAY** modify `web/portal/auth.js`
- Do **NOT** modify `sdkconfig.defaults`, `platformio.ini`, or partition tables
- Do **NOT** create any branches or make git commits

## Background

### How Restart Currently Works

1. User sets password via Security tab
2. `POST /api/auth/password` handler in `AuthManager.cpp` (~line 900-908):
   - Hashes password, stores in NVS
   - Generates cert (spawns FreeRTOS task, 12KB DRAM stack)
   - Returns `{restartRequired: true, reason: "restart_for_tls"}`
3. `auth.js` (line ~456) calls `POST /api/system/restart`
4. `WebServerManager.cpp:430` handles restart:
   ```cpp
   delay(500);
   esp_restart();
   ```

### Likely Root Causes

1. **Race with cert generation**: The cert generation FreeRTOS task may still be
   running when `esp_restart()` is called. The task writes to LittleFS — restarting
   mid-write could corrupt the cert files.
2. **PsychicHttp shutdown**: The HTTP server may have pending connections or
   TLS handshakes in progress. `esp_restart()` without stopping the server first
   could leave sockets in a bad state.
3. **NVS commit timing**: If `Preferences::end()` is not called before restart,
   buffered NVS writes may be lost.

> **NOTE**: If WO8 (TLS decoupling) is completed first, the password-set flow
> may no longer trigger a restart at all (password without TLS doesn't need
> restart). In that case, this WO focuses on the TLS-enable restart path and
> the general `POST /api/system/restart` reliability.

## Line-Item Instructions

### Step 1: Investigate the restart path

- [ ] **1a.** Read `modules/WebServer/WebServerManager.cpp` — find the
  `POST /api/system/restart` handler (~line 430).

- [ ] **1b.** Read `modules/AuthManager/AuthManager.cpp` — find the
  `POST /api/auth/password` handler. Identify:
  - Does it call `generateAndStoreCert()` synchronously or async?
  - If async (FreeRTOS task), is there a flag or semaphore the restart
    handler can wait on?

- [ ] **1c.** Read `web/portal/auth.js` — find where the restart is triggered
  after password change. Is there a delay? Does it wait for the cert generation
  to complete?

### Step 2: Implement safe shutdown sequence

- [ ] **2a.** In the `POST /api/system/restart` handler, implement a clean
  shutdown sequence:
  ```cpp
  // 1. Send the HTTP response FIRST (so the client gets confirmation)
  response->setCode(200);
  response->setContentType("application/json");
  response->setBody("{\"status\":\"restarting\"}");

  // 2. Schedule restart on the next tick (give the response time to flush)
  _restartRequested = true;
  ```

- [ ] **2b.** In the WebServerManager tick loop, handle the deferred restart:
  ```cpp
  if (_restartRequested) {
      // Wait for any in-flight cert generation to complete
      // (check AuthManager::isCertGenerating() or similar)

      // Stop HTTP/HTTPS servers cleanly
      if (_httpsServer) _httpsServer->end();
      _httpServer.end();

      // Allow final NVS flush
      delay(200);

      // Restart
      esp_restart();
  }
  ```

- [ ] **2c.** If cert generation is async, ensure the restart waits for it:
  - Check if `AuthManager` exposes a "cert generation in progress" flag
  - If not, add one: `bool isCertGenerating() const { return _certTaskRunning; }`
  - The restart should poll this flag with a timeout (e.g., 10 seconds max wait)

### Step 3: Fix the client-side flow

- [ ] **3a.** In `auth.js`, ensure the restart call has a proper delay and
  error handling:
  ```javascript
  // After password set response:
  if (data.restartRequired) {
      showStatus('Password set. Device restarting...', 'info');
      // Give the server time to complete any background tasks
      setTimeout(async () => {
          try {
              await apiFetch('/api/system/restart', { method: 'POST' });
          } catch (e) {
              // Expected — device is restarting, connection will drop
          }
          // Show reconnection UI
          showReconnecting();
      }, 2000);  // 2 second delay before restart request
  }
  ```

### Step 4: Build and verify

- [ ] **4a.** Build for esp32s3nano:
  ```bash
  pio run -e esp32s3nano
  ```

- [ ] **4b.** Build for esp32dev:
  ```bash
  pio run -e esp32dev
  ```

- [ ] **4c.** Run native tests:
  ```bash
  pio test -e native
  ```

### Step 5: Test on device

- [ ] **5a.** Flash and test the full password-set → restart flow:
  1. Open Security tab
  2. Set a password
  3. Confirm device restarts cleanly (serial monitor shows clean reboot)
  4. Confirm portal is accessible after restart

## Verification Requirement

After completing all checklist items:
1. Confirm both boards build cleanly
2. Confirm native tests pass
3. Confirm the restart path is clean (serial output shows orderly shutdown)
4. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
