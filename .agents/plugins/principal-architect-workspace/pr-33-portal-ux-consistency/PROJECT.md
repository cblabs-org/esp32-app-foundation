# PR-33: Web Portal UX Consistency — Reboot Overlays & Save Policies

> **Status:** 📋 PLANNING — awaiting user approval
> **Parent Branch:** `main`
> **Feature Branch:** `pr-33-portal-ux-consistency`

---

## 1. Problem Statement

The web portal has grown organically across 30+ PRs, and two categories of UX
inconsistency have accumulated:

### 1a. Reboot Overlay Fragmentation

Three **independent, structurally different** reboot/reconnection overlays exist:

| Overlay | File | Mechanism | Reconnection Logic |
|---------|------|-----------|---------------------|
| `#auth-reconnect-overlay` | `auth.js:161–205` | Inline JS-created DOM + injected `<style>` keyframes | **None** — shows forever, user must manually reload |
| `#wifi-restart-overlay` | `wifi.js:181–197` | Template literal HTML with inline `<style>` | **None** — shows forever, user must manually reload |
| OTA "complete — rebooting" | `ota.js:363–368` | Updates progress bar text; no overlay | **None** — status text only, no overlay |

Additionally, several reboot-triggering actions have **no overlay at all**:

| Action | File | Current Behaviour |
|--------|------|-------------------|
| System → Restart Device | `system.js:286–297` | Button text changes to "Restarting...", no overlay |
| System → Factory Reset | `system.js:320–331` | Button text changes to "Erasing...", no overlay |
| OTA → Rollback to Previous | `ota.js:639–656` | Button text changes to "Rolling back...", no overlay |
| Password set (first time, triggers TLS+restart) | `auth.js:526–537` | Toast + delayed restart call + `showReconnecting()` |
| TLS toggle | `auth.js:633–667` | Toast + delayed restart call + `showReconnecting()` |
| Cert regenerate | `auth.js:669–691` | Toast + delayed restart call + `showReconnecting()` |
| Cert upload | `auth.js:700–742` | Toast + delayed restart call + `showReconnecting()` |

### 1b. Save/Reboot Policy Inconsistency

| Tab | Setting | Save Mechanism | Reboot Required? | User Feedback |
|-----|---------|---------------|-------------------|---------------|
| **WiFi** | SSID+password | Explicit "Save & Connect" button | Yes — always | Inline overlay |
| **Display** | Brightness, flip, screensaver | Auto-save on each change | No | **Silent** (no toast on success) |
| **App** | Clock font size | Auto-save on change | No | **Silent** (no toast on success) |
| **OTA** | Channel, auto-check, check hour, updates enabled | Auto-save on each change | No | Inline status text (not toast), auto-clears after 3s |
| **Time** | Timezone, NTP servers | Explicit "Save Configuration" button | No | Toast ✅ |
| **Security/Password** | Set/change password | Explicit button | Sometimes (first set triggers TLS) | Toast + overlay OR toast only |
| **Security/TLS** | TLS toggle, AP mode toggle | Auto-save on toggle change | Yes (TLS toggle) / No (AP mode) | Mixed: overlay for TLS, toast-only for AP mode |
| **MCP** | Server enabled, read-only mode | Auto-save on toggle | No | **Silent** (no toast, no inline feedback) |
| **Keys** | Add key | Explicit button | No | Inline status text (not toast) |
| **Backup** | Restore config | Explicit button | Yes — always | Inline text, **no confirmation gate, no reboot overlay** |

---

## 2. Design Decisions

### 2a. Universal Reboot Overlay

**Decision:** Create a single, shared reboot overlay component in `utils.js` that all tabs call.

```
window.showRebootOverlay(message, subMessage)
window.hideRebootOverlay()
```

**HTML structure:** A single `#reboot-overlay` div injected by `utils.js` at startup.
It mirrors the existing `auth-reconnect-overlay` style but is **CSS-class-based** (using
existing `app.css` patterns) rather than inline styles.

**Reconnection polling:** After showing the overlay, the component polls the device's
base URL at 2-second intervals. When the device responds, it auto-reloads the page.
This eliminates the current "stuck overlay" problem.

**Context-aware messages:** Each call site passes a descriptive message:

| Trigger | Message | Sub-message |
|---------|---------|-------------|
| WiFi Save & Connect | "Connecting to WiFi…" | "The device will restart and attempt to join the selected network." |
| WiFi Forget | "Restarting device…" | "The device will restart in AP mode." |
| TLS enable/disable | "Applying TLS settings…" | "The device will restart to apply the secure connection." |
| Password set (first time) | "Setting up security…" | "The device will restart to enable HTTPS." |
| Cert regenerate | "Regenerating certificate…" | "The device will restart with the new certificate." |
| Cert upload | "Installing certificate…" | "The device will restart with the uploaded certificate." |
| System → Restart | "Restarting device…" | "Please wait for the device to come back online." |
| System → Factory Reset | "Factory reset in progress…" | "All configuration will be erased. The device will restart in AP mode." |
| OTA complete | "Applying firmware update…" | "The device will restart with the new firmware." |
| OTA rollback | "Rolling back firmware…" | "The device will restart with the previous firmware version." |

### 2b. Save & Reboot Policy

**Decision:** Formalize three tiers of settings change:

| Tier | Behaviour | UI Pattern | Examples |
|------|-----------|------------|----------|
| **Instant** | Auto-save on change, no reboot needed | Toggle/slider/dropdown saves immediately. Success → subtle toast. Failure → error toast + revert to previous value. | Display brightness, screensaver toggle, OTA channel, OTA auto-check, OTA check hour, OTA updates enabled |
| **Deferred** | Requires explicit "Save" button, no reboot | User edits multiple fields, clicks Save. Success → toast. | Time configuration (timezone + NTP servers) |
| **Disruptive** | Saves, then triggers device reboot | Save + show universal reboot overlay with context message. The overlay handles reconnection polling. | WiFi credentials, TLS toggle, first password set (triggers TLS), cert operations |

**Key clarifications:**
- **Display settings** already follow the Instant tier correctly — they auto-save via
  debounced POST. The only gap is missing success toasts (currently silent). WO1 will
  add them.
- **OTA settings** already follow the Instant tier correctly with inline status text.
  WO1 will unify them to use the same toast pattern as Display for consistency.
- **Time** correctly follows the Deferred tier with an explicit Save button + toast.
  No changes needed.
- **Password change** (when password already exists) does NOT require a reboot — only
  the first-ever password set triggers a TLS enablement restart. This distinction is
  already correct in the code.

---

## 3. Files Impacted

| File | Changes |
|------|---------|
| `web/portal/utils.js` | Add `showRebootOverlay()` / `hideRebootOverlay()` with reconnection polling |
| `web/portal/app.css` | Add `.reboot-overlay` styles, `.reboot-spinner` keyframes |
| `web/portal/auth.js` | Remove `showReconnecting()` and `#auth-reconnect-overlay` creation. Replace all 4 reboot paths with `showRebootOverlay()` |
| `web/portal/wifi.js` | Remove `#wifi-restart-overlay` HTML template + `@keyframes wifi-spin`. Replace 2 reboot paths with `showRebootOverlay()` |
| `web/portal/system.js` | Add `showRebootOverlay()` calls for Restart and Factory Reset |
| `web/portal/ota.js` | Add `showRebootOverlay()` call for OTA complete + rollback + upload. Migrate 4 settings from inline text to `showToast()` |
| `web/portal/display.js` | Add success toasts on setting changes (brightness, flip, screensaver, delays) |
| `web/portal/appconfig.js` | Add success toast to `postSetting()` |
| `web/portal/mcp.js` | Add success toast to `handleToggle()` |
| `web/portal/keys.js` | Migrate Add Key feedback from inline text to `showToast()` |
| `web/portal/backup.js` | Add armed confirmation gate to Restore button + `showRebootOverlay()` call |

---

## 4. Work Order Ledger

- **[/] [wo1-reboot-overlay](wo1-reboot-overlay/WORKORDER.md):** Universal reboot overlay component + CSS + save-policy toast alignment — creates the shared `showRebootOverlay` / `hideRebootOverlay` API in `utils.js`, adds CSS class definitions to `app.css`, and adds success toasts to `display.js`, `appconfig.js`, `mcp.js`, `ota.js`, and `keys.js`.
- **[ ] [wo2-migrate-callers](wo2-migrate-callers/WORKORDER.md):** Migrate all reboot-triggering call sites + backup restore safety — removes the 2 bespoke overlays from `auth.js` and `wifi.js`, wires all 11 reboot paths (including backup restore) across `auth.js`, `wifi.js`, `system.js`, `ota.js`, and `backup.js`, and adds a confirmation gate to the restore flow.
- **[ ] [wo3-playwright-tests](wo3-playwright-tests/WORKORDER.md):** Playwright test coverage — add tests for reboot overlay visibility, context-aware messages, and the auto-reconnection polling behaviour across WiFi, System, Security, and Backup flows.
- **[ ] [wo4-documentation](wo4-documentation/WORKORDER.md):** Documentation — adds `save-policy.md` and `shared-utilities.md` to the `framework-expert` skill, updates `extending-the-web-portal.md` developer doc with save policy section and `showToast()` examples (replacing `alert()`), updates `html-js-fragments.md` API references.

### Dependency Graph
```
WO1 (shared component + toast alignment)
  └─▶ WO2 (migrate all callers + backup gate) — depends on WO1
       └─▶ WO3 (Playwright tests) — depends on WO2
            └─▶ WO4 (documentation) — depends on WO2 (can run parallel to WO3)
```

All four WOs are serial, except WO3 and WO4 which may run in parallel (no file overlap).

---

## 5. Scope Exclusions

- **No C++ firmware changes.** All `/api/system/restart`, `/api/wifi/connect`, etc.
  endpoints already exist and work correctly. This PR is purely portal JS/CSS.
- **No new API endpoints.** The reconnection polling in the overlay will use an
  existing lightweight endpoint (`/api/system` or `/api/app`) to detect when the device
  is back online.
- **No authentication flow changes.** The login overlay (`#login-overlay`) is untouched.
- **No changes to the tab navigation system** (`app.js`).
