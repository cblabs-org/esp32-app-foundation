# Save & Reboot Policy

The framework enforces a four-tier save policy across all portal tabs to ensure consistent UX. Every setting change MUST produce user feedback — a silent save is a policy violation.

## The Four Tiers

| Tier | Name | Trigger | UI Pattern | User Feedback | When to Use |
|------|------|---------|------------|---------------|-------------|
| **1** | **Instant** | Auto-save on each change | Toggle / slider / dropdown / radio | `window.showToast('Setting saved.', 'success')` on success, `window.showToast('Failed to save.', 'error')` + revert on failure | Single-field setting, no reboot, trivially reversible |
| **2** | **Batched** | Explicit "Save" button | Multi-field form with a Save CTA | `window.showToast(...)` | Multiple related fields that should be saved atomically |
| **3** | **Disruptive** | Auto-save + device reboot | Button or toggle that triggers restart | `window.showRebootOverlay(msg, subMsg)` | Change requires a device reboot |
| **4** | **Destructive** | Armed confirm or typed confirm | Red-zone action, data loss possible | `window.armButton(btn, action)` → `window.showRebootOverlay(...)` | Irreversible action (factory reset, rollback, forget, restore) |

## Decision Flowchart

```
Is it a single field?
  ├─ Yes → Does it require reboot?
  │         ├─ Yes → Tier 3 (Disruptive)
  │         └─ No → Is it destructive/irreversible?
  │                   ├─ Yes → Tier 4 (Destructive)
  │                   └─ No → Tier 1 (Instant)
  └─ No → Tier 2 (Batched)
```

## Code Examples

### Tier 1 — Instant (Auto-Save)

```javascript
toggleEl.addEventListener('change', async function() {
  try {
    await window.apiFetch('/api/mymodule/setting', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ enabled: toggleEl.checked })
    });
    window.showToast('Setting saved.', 'success');
  } catch (err) {
    window.showToast('Failed to save setting.', 'error');
    toggleEl.checked = !toggleEl.checked; // Revert on failure
  }
});
```

### Tier 2 — Batched (Explicit Save Button)

```javascript
saveBtn.addEventListener('click', async function() {
  try {
    await window.apiFetch('/api/mymodule/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ field1: val1, field2: val2 })
    });
    window.showToast('Configuration saved.', 'success');
  } catch (err) {
    window.showToast('Failed to save configuration.', 'error');
  }
});
```

### Tier 3 — Disruptive (Reboot Required)

```javascript
await window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function() {});
window.showRebootOverlay('Applying settings…', 'The device will restart.');
```

### Tier 4 — Destructive (Armed Confirmation)

```javascript
dangerBtn.addEventListener('click', function() {
  window.armButton(dangerBtn, async function() {
    await window.apiFetch('/api/danger', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Resetting device…', 'All data will be erased.');
  });
});
```

## Key Principles

- **Auto-save is the default.** An explicit Save button is only justified when the user is editing multiple related fields.
- **Every save must produce user feedback** — a silent save is a policy violation.
- Use `window.apiFetch()` (not raw `fetch()`) for all API calls — it serialises requests through the `ApiQueue` to protect ESP32 socket pools.
- Use `window.showToast()` (not `alert()` or inline status text) for transient feedback.
- Use `window.showRebootOverlay()` (not custom DOM injection) for any action that reboots the device.
