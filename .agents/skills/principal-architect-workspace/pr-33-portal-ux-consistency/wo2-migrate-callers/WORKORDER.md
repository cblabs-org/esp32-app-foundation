---
name: pr-33-wo2
description: Handoff instructions for Work Order 2 — Migrate all reboot callers + backup restore safety gate
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Remove the two bespoke reboot overlay implementations from `auth.js` and `wifi.js`, migrate all 11 reboot-triggering paths across the portal to use the shared `window.showRebootOverlay()` API created in WO1, and add a missing confirmation gate to the backup restore flow.

# Strict Boundary Box

- **DO** modify: `web/portal/auth.js`, `web/portal/wifi.js`, `web/portal/system.js`, `web/portal/ota.js`, `web/portal/backup.js`
- **DO NOT** modify: `utils.js`, `app.css`, `app.js`, `display.js`, `appconfig.js`, `mcp.js`, `keys.js`, `index.html`, or any other file
- **DO NOT** touch any C++ firmware files
- **DO NOT** change the login overlay (`#login-overlay`) or the 401 auth interceptor — those are unrelated
- **DO NOT** change any API endpoints or request payloads
- **DO NOT** create branches or make architectural decisions

# Tooling Constraints

- Use the `platformio` MCP server tools for any build/test operations.
- This WO is pure JS — no firmware build needed.

# Pre-Requisite

WO1 must be merged first. The `window.showRebootOverlay(message, subMessage)` function must exist in `utils.js` before you begin.

# Line-Item Instructions

## Task 1: Clean up `auth.js`

### 1a. Remove the `showReconnecting()` function
- [ ] Delete the entire `showReconnecting()` function (currently at approximately lines 161–205 in `auth.js`). This includes the inline DOM creation of `#auth-reconnect-overlay`, the injected `<style>` with `@keyframes auth-spin`, and the function body.

### 1b. Migrate `handleSetPassword()` reboot path
- [ ] In `handleSetPassword()`, find the block where `data.restartRequired` is true (approx line 526–537). Replace:
  ```javascript
  setTimeout(async function () {
    try {
      await window.apiFetch('/api/system/restart', { method: 'POST' });
    } catch (e) {}
    showReconnecting();
  }, 2000);
  ```
  With:
  ```javascript
  setTimeout(function () {
    window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Setting up security…', 'The device will restart to enable HTTPS.');
  }, 2000);
  ```

### 1c. Migrate `handleTlsChange()` reboot path
- [ ] In `handleTlsChange()`, find the `data.restartRequired` block (approx line 643–654). Replace:
  ```javascript
  setTimeout(async function () {
    try {
      await window.apiFetch('/api/system/restart', { method: 'POST' });
    } catch (e) {}
    showReconnecting();
  }, 2000);
  ```
  With:
  ```javascript
  setTimeout(function () {
    window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Applying TLS settings…', 'The device will restart to apply the secure connection.');
  }, 2000);
  ```

### 1d. Migrate `handleRegenCert()` reboot path
- [ ] In `handleRegenCert()`, find the `data.restartRequired` block (approx line 675–684). Replace:
  ```javascript
  setTimeout(async function() {
    try {
      await window.apiFetch('/api/system/restart', { method: 'POST' });
    } catch (e) {}
    showReconnecting();
  }, 2000);
  ```
  With:
  ```javascript
  setTimeout(function () {
    window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Regenerating certificate…', 'The device will restart with the new certificate.');
  }, 2000);
  ```

### 1e. Migrate `handleUploadCert()` reboot path
- [ ] In `handleUploadCert()`, find the `data.restartRequired` block (approx line 726–735). Replace:
  ```javascript
  setTimeout(async function() {
    try {
      await window.apiFetch('/api/system/restart', { method: 'POST' });
    } catch (e) {}
    showReconnecting();
  }, 2000);
  ```
  With:
  ```javascript
  setTimeout(function () {
    window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Installing certificate…', 'The device will restart with the uploaded certificate.');
  }, 2000);
  ```

## Task 2: Clean up `wifi.js`

### 2a. Remove the bespoke restart overlay HTML
- [ ] In the `container.innerHTML` template literal, remove the entire `<!-- Restarting Overlay -->` block (approx lines 180–197), including:
  - The `#wifi-restart-overlay` div
  - The `<style>` block with `@keyframes wifi-spin`
- [ ] Remove the `const restartOverlay = document.getElementById('wifi-restart-overlay');` DOM reference line.

### 2b. Migrate "Save & Connect" reboot path
- [ ] In the save button handler (approx line 543–544), replace:
  ```javascript
  restartOverlay.style.display = 'flex';
  ```
  With:
  ```javascript
  window.showRebootOverlay('Connecting to WiFi…', 'The device will restart and attempt to join the selected network.');
  ```

### 2c. Migrate "Forget Network" reboot path
- [ ] In the forget button handler (approx line 584–585), replace:
  ```javascript
  restartOverlay.style.display = 'flex';
  ```
  With:
  ```javascript
  window.showRebootOverlay('Restarting device…', 'The device will restart in AP mode.');
  ```

## Task 3: Add overlay to `system.js`

### 3a. Restart Device
- [ ] In the restart button handler (approx line 286–297), after the `restartBtn.textContent = 'Restarting...'` line and after the `apiFetch` call, add:
  ```javascript
  window.showRebootOverlay('Restarting device…', 'Please wait for the device to come back online.');
  ```
  Place it right after the try/catch block for the restart API call.

### 3b. Factory Reset
- [ ] In the factory reset handler (approx line 320–331), after the `apiFetch` call, add:
  ```javascript
  window.showRebootOverlay('Factory reset in progress…', 'All configuration will be erased. The device will restart in AP mode.');
  ```
  Place it right after the try/catch block for the factory-reset API call.

## Task 4: Add overlay to `ota.js`

### 4a. OTA update complete
- [ ] In `pollOtaStatus()`, in the `case 'complete':` block (approx line 363–368), after `statusEl.textContent = '...'`, add:
  ```javascript
  window.showRebootOverlay('Applying firmware update…', 'The device will restart with the new firmware.');
  ```

### 4b. OTA rollback
- [ ] In the rollback button handler (approx line 639–656), after the `apiFetch` call (inside the try block), add:
  ```javascript
  window.showRebootOverlay('Rolling back firmware…', 'The device will restart with the previous firmware version.');
  ```

### 4c. OTA manual upload success
- [ ] In the `xhr.onload` handler (approx line 589–610), in the success branch (`xhr.status >= 200 && xhr.status < 300`), after `statusEl.textContent = result.msg || ...`, add:
  ```javascript
  window.showRebootOverlay('Applying firmware update…', 'The device will restart with the new firmware.');
  ```

## Task 5: Add confirmation gate + reboot overlay to `backup.js`

The Backup → Restore flow is the **most dangerous action in the portal** (overwrites ALL device config and reboots) but currently has NO confirmation gate. Fix this.

### 5a. Add armed confirmation to the Restore button
- [ ] In `web/portal/backup.js`, modify the `btn-backup-restore` click handler (approx lines 151–200). Wrap the existing restore logic with the `window.armButton()` utility from `utils.js`. The flow should be:
  1. First click → arms the button (shows "Confirm?" in red for 3s)
  2. Second click within 3s → executes the restore

Replace the click handler with:
```javascript
document.getElementById('btn-backup-restore').addEventListener('click', function() {
  const restoreBtn = document.getElementById('btn-backup-restore');
  const fileInput = document.getElementById('backup-restore-file');
  const statusEl = document.getElementById('backup-restore-status');

  // Validate file is selected BEFORE arming
  if (!fileInput.files.length) {
    statusEl.style.color = 'var(--error)';
    statusEl.textContent = 'Please select a .json file first.';
    return;
  }

  window.armButton(restoreBtn, async function() {
    const file = fileInput.files[0];

    // Validate file size
    if (file.size > 16384) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'File too large (max 16 KB).';
      return;
    }

    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Uploading and restoring...';

    try {
      const text = await file.text();
      const parsed = JSON.parse(text);
      const payload = { ...parsed };
      delete payload._meta;

      await window.apiFetch('/api/config/restore', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });

      fileInput.value = '';
      window.showRebootOverlay(
        'Restoring configuration…',
        'All settings will be overwritten. The device will restart.'
      );
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      if (error instanceof SyntaxError) {
        statusEl.textContent = 'Invalid JSON file.';
      } else {
        statusEl.textContent = 'Restore failed. Check file and connection.';
      }
    }
  });
});
```

**Key changes:**
- Uses `window.armButton()` for the 3s armed confirmation gate
- Shows `showRebootOverlay()` on success instead of inline text
- File validation still happens before arming (so user isn't confused by an armed button with no file selected)

# Verification Requirement

1. Search the entire `web/portal/` directory for any remaining references to:
   - `showReconnecting` — should be ZERO occurrences
   - `auth-reconnect-overlay` — should be ZERO occurrences
   - `wifi-restart-overlay` — should be ZERO occurrences
   - `wifi-spin` — should be ZERO occurrences
   - `auth-spin` — should be ZERO occurrences
2. Search for `showRebootOverlay` — should be exactly **11** call sites across **5** files (`auth.js`, `wifi.js`, `system.js`, `ota.js`, `backup.js`).
3. Verify `backup.js` restore flow uses `window.armButton()` and `window.showRebootOverlay()`.
4. Verify no syntax errors by reviewing all modified files for matching braces.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill.
