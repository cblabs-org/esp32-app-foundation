---
name: pr-33-wo4
description: Handoff instructions for Work Order 4 — Document save policy and shared utilities in developer docs + framework-expert skill
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Document the save/reboot policy and the shared portal utility functions (`showToast`, `showRebootOverlay`, `armButton`) in both the developer documentation and the `framework-expert` AI skill, so that future developers and AI agents building on the framework follow the established UX patterns.

# Strict Boundary Box

- **DO** modify/create files in:
  - `docs/developer/extending-the-web-portal.md`
  - `.agents/skills/framework-expert/systems/web-portal/`
  - `.agents/skills/framework-expert/skill_manifest.json`
- **DO NOT** modify any portal JS/CSS files
- **DO NOT** modify any C++ firmware files
- **DO NOT** modify files outside the two directories listed above (except `skill_manifest.json`)
- **DO NOT** create branches or make architectural decisions

# Tooling Constraints

- This WO is documentation only — no firmware build needed.

# Pre-Requisite

WO1 and WO2 must be merged first. The shared utilities (`showRebootOverlay`, `hideRebootOverlay`) must exist in `utils.js` and all callers must be migrated.

# Line-Item Instructions

## Task 1: Create `save-policy.md` in the framework-expert skill

- [ ] Create a new file at `.agents/skills/framework-expert/systems/web-portal/save-policy.md` with the following content.

This document should cover:

### 1a. The Four-Tier Save Policy

| Tier | Name | Trigger | UI Pattern | User Feedback | When to Use |
|------|------|---------|------------|---------------|-------------|
| **1** | **Instant** | Auto-save on each change | Toggle / slider / dropdown / radio | `window.showToast('Setting saved.', 'success')` on success, `window.showToast('Failed to save.', 'error')` + revert on failure | Single-field setting, no reboot, trivially reversible |
| **2** | **Batched** | Explicit "Save" button | Multi-field form with a Save CTA | `window.showToast(...)` | Multiple related fields that should be saved atomically |
| **3** | **Disruptive** | Auto-save + device reboot | Button or toggle that triggers restart | `window.showRebootOverlay(msg, subMsg)` | Change requires a device reboot |
| **4** | **Destructive** | Armed confirm or typed confirm | Red-zone action, data loss possible | `window.armButton(btn, action)` → `window.showRebootOverlay(...)` | Irreversible action (factory reset, rollback, forget, restore) |

### 1b. The Decision Flowchart (as text)

```
Is it a single field?
  ├─ Yes → Does it require reboot?
  │         ├─ Yes → Tier 3 (Disruptive)
  │         └─ No → Is it destructive/irreversible?
  │                   ├─ Yes → Tier 4 (Destructive)
  │                   └─ No → Tier 1 (Instant)
  └─ No → Tier 2 (Batched)
```

### 1c. Code Examples

For each tier, provide a concise code example showing the correct JS pattern:

**Tier 1 (Instant):**
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

**Tier 2 (Batched):**
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

**Tier 3 (Disruptive):**
```javascript
await window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function() {});
window.showRebootOverlay('Applying settings…', 'The device will restart.');
```

**Tier 4 (Destructive):**
```javascript
dangerBtn.addEventListener('click', function() {
  window.armButton(dangerBtn, async function() {
    await window.apiFetch('/api/danger', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Resetting device…', 'All data will be erased.');
  });
});
```

### 1d. Key Principles

- Auto-save is the default. An explicit Save button is only justified when the user is editing multiple related fields.
- **Every** save must produce user feedback — a silent save is a policy violation.
- Use `window.apiFetch()` (not raw `fetch()`) for all API calls — it serialises requests through the `ApiQueue` to protect ESP32 socket pools.
- Use `window.showToast()` (not `alert()` or inline status text) for transient feedback.
- Use `window.showRebootOverlay()` (not custom DOM injection) for any action that reboots the device.

## Task 2: Create `shared-utilities.md` in the framework-expert skill

- [ ] Create a new file at `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md` documenting the globally available utility functions in `utils.js`.

Cover each function with its signature, parameters, and a usage example:

| Function | Purpose |
|----------|---------|
| `window.showToast(message, type, duration)` | Slide-in toast notification. Types: `'success'`, `'error'`, `'warning'`, `'info'` (default). Duration default: 3000ms. |
| `window.armButton(button, action, timeout)` | Armed confirmation. First click arms (shows "Confirm?" in red), second click within timeout (default 3000ms) executes `action`. Auto-disarms after timeout. |
| `window.showRebootOverlay(message, subMessage)` | Full-screen reboot overlay with spinner. Shows a context-aware message and starts reconnection polling (2s interval after 3s delay). Auto-reloads the page when the device responds. |
| `window.hideRebootOverlay()` | Hides the reboot overlay and stops polling. Normally not needed (auto-reload handles it). |
| `window.apiFetch(url, options)` | Serialised `fetch()` wrapper. Queues requests through `ApiQueue` to prevent ESP32 socket pool exhaustion. All portal API calls MUST use this instead of raw `fetch()`. |
| `window.showToast(...)` | See above. |
| `window.escapeHtml(str)` | XSS-safe HTML escaping for innerHTML usage. |
| `window.debounce(fn, ms)` | Standard debounce wrapper. |
| `window.formatUptime(seconds)` | Formats seconds to "Xd Xh Xm" or "Xm Xs". |
| `window.formatBytes(bytes)` | Formats bytes to "X.X KB" or "X.X MB". |
| `window.rssiToBars(rssi)` | Converts RSSI dBm to 0–4 bar level. |
| `window.renderProgressBar(label, value, max, unit)` | RAG-coloured progress bar HTML string. |

## Task 3: Update `skill_manifest.json`

- [ ] Add the two new files to the `references` array in `.agents/skills/framework-expert/skill_manifest.json`:

```json
"systems/web-portal/save-policy.md",
"systems/web-portal/shared-utilities.md"
```

Insert them after the existing `"systems/web-portal/html-js-fragments.md"` entry.

- [ ] Update `"sourceDocsVersion"` to `"PR-33"`.

## Task 4: Update `extending-the-web-portal.md` developer doc

- [ ] In `docs/developer/extending-the-web-portal.md`, make the following updates:

### 4a. Replace `alert()` with `window.showToast()`
Find the JavaScript example code that uses `alert('Configuration saved successfully')` (line 210) and `alert('Save failed: ' + err.message)` (line 213). Replace with:
```javascript
window.showToast('Configuration saved.', 'success');
```
and:
```javascript
window.showToast('Save failed: ' + err.message, 'error');
```

### 4b. Add a Save Policy section
After the existing "API Route Conventions" section (section 5), add a new section:

```markdown
---

## 6. Save & Reboot Policy

When adding settings to your tab, follow the framework's standard save policy to ensure UX consistency across all portal tabs.

### The Four Tiers

| Tier | Name | When to Use | UI Pattern |
|------|------|-------------|------------|
| **Instant** | Single-field settings that take effect immediately (no reboot) | Auto-save on each change + `showToast()` feedback |
| **Batched** | Multiple related fields that should be committed atomically | Explicit "Save" button + `showToast()` feedback |
| **Disruptive** | Settings that require a device reboot to take effect | Auto-save or button + `showRebootOverlay()` |
| **Destructive** | Irreversible actions (reset, delete, erase) | `armButton()` or typed confirmation + `showRebootOverlay()` |

> [!IMPORTANT]
> **Every save must produce user feedback.** A silent save (no toast, no overlay) is a policy violation. Use `window.showToast()` for transient feedback and `window.showRebootOverlay()` for reboot paths.

### Shared Utility Functions

The framework provides these global functions in `utils.js` (loaded before all tab scripts):

- **`window.showToast(message, type, duration)`** — Slide-in toast notification. Types: `'success'`, `'error'`, `'warning'`, `'info'`.
- **`window.armButton(button, action, timeout)`** — Armed confirmation pattern. First click arms the button, second click within timeout executes the action.
- **`window.showRebootOverlay(message, subMessage)`** — Full-screen overlay with spinner and reconnection polling. Auto-reloads the page when the device comes back online.
- **`window.apiFetch(url, options)`** — Serialised fetch wrapper. **Always use this instead of raw `fetch()`** — it queues requests to protect ESP32 socket pools.

### Decision Rule

> Auto-save is the default. Only use an explicit "Save" button when the user is editing **multiple related fields** that should be committed atomically.
```

### 4c. Renumber subsequent sections
If there are sections numbered after section 5, renumber them accordingly (6 → 7, etc.).

## Task 5: Update the framework-expert skill `html-js-fragments.md`

- [ ] In `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md`, the example code on line 60 uses `App.showToast('Saved!', 'success')`. This should be updated to `window.showToast('Saved!', 'success')` to match the actual API (the global is `window.showToast`, not `App.showToast`).

- [ ] Also add a brief note at the bottom of the file:

```markdown
## Save Policy

All settings must follow the framework's four-tier save policy. See [Save Policy](save-policy.md) for the full specification and code examples.
```

# Verification Requirement

1. Verify the new `save-policy.md` covers all four tiers with code examples.
2. Verify `skill_manifest.json` includes both new files and `sourceDocsVersion` is `"PR-33"`.
3. Verify `extending-the-web-portal.md` no longer uses `alert()` — search for `alert(` and confirm zero occurrences.
4. Verify `html-js-fragments.md` uses `window.showToast` not `App.showToast`.
5. Verify all cross-references between documents use correct relative paths.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill.
