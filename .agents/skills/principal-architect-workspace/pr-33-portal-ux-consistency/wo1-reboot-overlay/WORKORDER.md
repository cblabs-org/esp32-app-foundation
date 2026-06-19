---
name: pr-33-wo1
description: Handoff instructions for Work Order 1 — Universal reboot overlay component + save-policy toast alignment
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Create the shared universal reboot overlay component (`showRebootOverlay` / `hideRebootOverlay`) in `utils.js`, add CSS definitions to `app.css`, and align all Tier 1 (Instant) settings across the portal to use consistent success toasts.

# Strict Boundary Box

- **DO** modify: `web/portal/utils.js`, `web/portal/app.css`, `web/portal/display.js`, `web/portal/appconfig.js`, `web/portal/mcp.js`, `web/portal/ota.js`, `web/portal/keys.js`
- **DO NOT** modify: `auth.js`, `wifi.js`, `system.js`, `backup.js`, `app.js`, `index.html`, or any other file
- **DO NOT** remove existing overlay implementations (that's WO2)
- **DO NOT** create branches or make architectural decisions
- **DO NOT** touch any C++ firmware files

# Tooling Constraints

- Use the `platformio` MCP server tools for any build/test operations (e.g., `build_project`).
- This WO is pure JS/CSS — no firmware build needed. Verify by opening the HTML in a browser or reviewing code logic.

# Line-Item Instructions

## Task 1: Add reboot overlay CSS to `app.css`

- [ ] Add the following CSS classes at the end of `web/portal/app.css` (before any media queries if present):

```css
/* ===================================================================
   Reboot Overlay — shared full-screen spinner for all device restarts
   =================================================================== */
.reboot-overlay {
  position: fixed;
  inset: 0;
  z-index: 10000;
  background: rgba(0, 0, 0, 0.85);
  backdrop-filter: blur(6px);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  color: white;
  gap: 1.25rem;
  opacity: 0;
  visibility: hidden;
  transition: opacity 0.3s ease, visibility 0.3s ease;
}
.reboot-overlay.visible {
  opacity: 1;
  visibility: visible;
}
.reboot-spinner {
  width: 44px;
  height: 44px;
  border: 3px solid rgba(255, 255, 255, 0.25);
  border-top-color: white;
  border-radius: 50%;
  animation: reboot-spin 0.8s linear infinite;
}
@keyframes reboot-spin {
  to { transform: rotate(360deg); }
}
.reboot-overlay .reboot-msg {
  font-size: 1.15rem;
  font-weight: 600;
  text-align: center;
}
.reboot-overlay .reboot-sub {
  font-size: 0.85rem;
  opacity: 0.7;
  text-align: center;
  max-width: 320px;
  line-height: 1.5;
}
```

## Task 2: Add `showRebootOverlay` / `hideRebootOverlay` to `utils.js`

- [ ] At the end of the `utils.js` IIFE (before the closing `})();`), add the following:

```javascript
// ===========================================================================
// Universal Reboot Overlay
// ===========================================================================

/**
 * @brief Inject the reboot overlay DOM element (once).
 *        Called automatically on first use.
 */
function ensureRebootOverlay() {
  if (document.getElementById('reboot-overlay')) return;
  var overlay = document.createElement('div');
  overlay.id = 'reboot-overlay';
  overlay.className = 'reboot-overlay';
  overlay.innerHTML =
    '<div class="reboot-spinner"></div>' +
    '<div class="reboot-msg">Restarting device…</div>' +
    '<div class="reboot-sub">Please wait for the device to come back online.</div>';
  document.body.appendChild(overlay);
}

/**
 * @brief Show the universal reboot overlay with a context-aware message.
 *        Starts reconnection polling that auto-reloads when the device
 *        responds.
 * @param {string} message - Primary status message (e.g. "Applying TLS settings…").
 * @param {string} subMessage - Secondary explanation text.
 */
window.showRebootOverlay = function(message, subMessage) {
  ensureRebootOverlay();
  var overlay = document.getElementById('reboot-overlay');
  if (!overlay) return;

  var msgEl = overlay.querySelector('.reboot-msg');
  var subEl = overlay.querySelector('.reboot-sub');
  if (msgEl) msgEl.textContent = message || 'Restarting device…';
  if (subEl) subEl.textContent = subMessage || 'Please wait for the device to come back online.';

  overlay.classList.add('visible');

  // Start reconnection polling after a brief delay (give device time to begin restart)
  if (window._rebootPollTimer) clearInterval(window._rebootPollTimer);
  setTimeout(function() {
    window._rebootPollTimer = setInterval(function() {
      fetch('/api/app', { method: 'GET', cache: 'no-store' })
        .then(function(resp) {
          if (resp.ok) {
            clearInterval(window._rebootPollTimer);
            window._rebootPollTimer = null;
            window.location.reload();
          }
        })
        .catch(function() {
          // Device still rebooting — keep polling
        });
    }, 2000);
  }, 3000);
};

/**
 * @brief Hide the reboot overlay and stop reconnection polling.
 *        Normally not needed (page reloads on reconnection), but
 *        available for error recovery.
 */
window.hideRebootOverlay = function() {
  var overlay = document.getElementById('reboot-overlay');
  if (overlay) overlay.classList.remove('visible');
  if (window._rebootPollTimer) {
    clearInterval(window._rebootPollTimer);
    window._rebootPollTimer = null;
  }
};
```

## Task 3: Add success toasts to `display.js`

- [ ] In `web/portal/display.js`, update the `postSetting` function to show a success toast when the API call succeeds. Currently it only shows a toast on error. Add a success toast:

```javascript
async function postSetting(endpoint, payload) {
  try {
    await window.apiFetch(endpoint, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    window.showToast('Setting saved.', 'success');
  } catch (err) {
    window.showToast('Failed to save setting.', 'error');
  }
}
```

**Note:** The brightness slider uses a debounced 300ms timer, so the toast will only fire after the user stops dragging — this is the correct behaviour.

## Task 4: Add success toast to `appconfig.js`

- [ ] In `web/portal/appconfig.js`, the `postSetting` function (lines 81–91) is identical to `display.js` — it only shows error toasts. Add a success toast on the happy path:

```javascript
async function postSetting(endpoint, payload) {
  try {
    await window.apiFetch(endpoint, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    window.showToast('Setting saved.', 'success');
  } catch (err) {
    window.showToast('Failed to save setting.', 'error');
  }
}
```

## Task 5: Add success toast to `mcp.js`

- [ ] In `web/portal/mcp.js`, the `handleToggle` function (lines 98–112) silently succeeds and only reloads on error. Add a success toast:

```javascript
async function handleToggle(key, value) {
  try {
    const body = {};
    body[key] = value;
    await window.apiFetch('/api/mcp/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    window.showToast('MCP setting saved.', 'success');
  } catch (error) {
    console.error('Failed to update MCP config:', error);
    // Reload to restore correct state
    loadMcpInfo();
  }
}
```

## Task 6: Migrate OTA settings to use toasts

- [ ] In `web/portal/ota.js`, four OTA settings currently use an inline `#ota-settings-status` element that auto-clears after 3s. Migrate these to use `window.showToast()` for consistency. The affected handlers are:

### 6a. Channel change (lines 396–412)
Replace:
```javascript
statusEl.style.color = 'var(--success)';
statusEl.textContent = 'Channel set to ' + channel;
setTimeout(() => { statusEl.textContent = ''; }, 3000);
```
With:
```javascript
window.showToast('Channel set to ' + channel, 'success');
```
And replace the error path:
```javascript
statusEl.style.color = 'var(--error)';
statusEl.textContent = 'Failed to set channel.';
```
With:
```javascript
window.showToast('Failed to set channel.', 'error');
```

### 6b. Enable firmware updates toggle (lines 417–441)
Replace the success path:
```javascript
statusEl.style.color = 'var(--success)';
statusEl.textContent = 'Firmware updates ' + (enabled ? 'enabled' : 'disabled');
setTimeout(() => { statusEl.textContent = ''; }, 3000);
```
With:
```javascript
window.showToast('Firmware updates ' + (enabled ? 'enabled' : 'disabled'), 'success');
```
And replace the error path:
```javascript
statusEl.style.color = 'var(--error)';
statusEl.textContent = 'Failed to save setting.';
```
With:
```javascript
window.showToast('Failed to save setting.', 'error');
```

### 6c. Auto-check toggle (lines 446–470)
Same pattern — replace success inline text with `window.showToast('Auto-check ' + (enabled ? 'enabled' : 'disabled'), 'success')` and error with `window.showToast('Failed to save setting.', 'error')`.

### 6d. Daily check hour (lines 475–494)
Replace success with `window.showToast('Daily check set to ' + String(hour).padStart(2, '0') + ':00', 'success')` and error with `window.showToast('Failed to save setting.', 'error')`.

### 6e. Clean up
- [ ] Once all four handlers have been migrated, the `const statusEl = document.getElementById('ota-settings-status')` references in each handler are no longer needed. Remove them.
- [ ] The `#ota-settings-status` div in the HTML scaffold (line 72) can be removed.

## Task 7: Migrate Keys → Add Key to use toasts

- [ ] In `web/portal/keys.js`, the Add Key handler (lines 137–181) uses an inline `#keys-add-status` element. Migrate to toasts:

Replace the success feedback (line 169):
```javascript
statusEl.innerHTML = '<span style="color:#2e7d32;">Key saved successfully.</span>';
```
With:
```javascript
window.showToast('Key saved successfully.', 'success');
```

Replace the error feedback (lines 172–176):
```javascript
if (error && error.message && error.message.indexOf('403') !== -1) {
  statusEl.innerHTML = '<span style="color:var(--error);">This key is managed by a module and cannot be overwritten.</span>';
} else {
  statusEl.innerHTML = '<span style="color:var(--error);">Failed to save key.</span>';
}
```
With:
```javascript
if (error && error.message && error.message.indexOf('403') !== -1) {
  window.showToast('This key is managed by a module and cannot be overwritten.', 'error');
} else {
  window.showToast('Failed to save key.', 'error');
}
```

- [ ] The validation error at line 147 (`'Both name and value are required.'`) should remain as inline text OR use a warning toast — worker's choice, but be consistent.

# Verification Requirement

1. Review your changes for syntactic correctness (matching braces, no missing semicolons).
2. Verify the CSS class names in `app.css` match exactly what `utils.js` references (`reboot-overlay`, `visible`, `reboot-spinner`, `reboot-msg`, `reboot-sub`).
3. Verify `showRebootOverlay` and `hideRebootOverlay` are attached to `window` so they're globally accessible from other scripts.
4. Verify every `postSetting()` or `handleToggle()` function now shows a success toast.
5. Verify that the OTA settings status element has been removed and all 4 handlers use toasts.
6. Search for `showToast` in all modified files to confirm consistent usage.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill.
