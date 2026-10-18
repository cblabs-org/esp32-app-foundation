# Hardware Test Plan — CrowPanel 1.28" SLS Screen Redesign + CONFIGURING

**Target:** Elecrow CrowPanel 1.28" round display (ESP32-S3, GC9A01 / CST816D touch / rotary encoder)
**Under test:** `sls-screen-redesign` stack (WO1–WO6) — redesigned SLS screens (Setup / Busy / Message groups, simplified Diag) **and** the newly-implemented LVGL CONFIGURING orchestration (#82). This is the on-device smoke test the stack has **not** yet had (all verification so far is compile + host-unit only).
**Owner:** _(hardware operator)_  **Date:** _____  **Firmware tag:** _____  **Result:** ☐ Pass ☐ Fail

> Priority focus: the **CONFIGURING screens (T3–T6)** are brand-new on LVGL — they have never rendered on hardware. Boot / Diag / Error / OTA are lower risk (mostly re-skinned) but still verify.

---

## 0. Flash gate (MANDATORY — do first, per `.agents/rules/03-versioning.md`)

The firmware contains new functionality, so it MUST be tagged before flashing:

1. `git describe --tags --abbrev=0` → baseline is `v0.0.12-dev.35`.
2. `git tag v0.0.12-dev.36 && git push origin v0.0.12-dev.36` (dev tags are autonomous — no approval needed).
3. **Rebuild** `crowpanel-128-rotary` so `inject_version.py` bakes in the new tag.
4. Flash + monitor: via the `pio-manager` MCP (`upload_firmware` env `crowpanel-128-rotary`, `start_monitor: true`).
5. After boot, `curl /api/system` (or MCP `system_info`) → confirm `firmware_version == v0.0.12-dev.36`. **Do not test an untagged build.**

> ⚠️ Flashing is a hardware action — confirm the CrowPanel is the connected board and on the expected serial port (`list_devices`) before upload to avoid bricking another target.

---

## 1. Setup & preconditions

- [ ] CrowPanel 1.28" connected; correct port confirmed (`pio-manager` → `list_devices`).
- [ ] Firmware `v0.0.12-dev.36` flashed and booted (§0).
- [ ] Serial monitor running (watch for `LVGLDisplayModule` logs, `CONFIGURING gate: …`, state transitions).
- [ ] Device MCP reachable — note device IP; verify `system_info` responds.
- [ ] A second device (phone/laptop) available to join the SoftAP and hit the config portal.

## 2. Verification method

For each case: **capture a screenshot** and eyeball it against the SLS design intent, and cross-check serial/MCP state.

- **Screenshot:** MCP `capture_display` (or `.../device-mcp/capture_display.py`). Save each as `T<id>-<name>.png` and attach to the results.
- **State:** MCP `display_status`, `wifi_get_status`, `ota_get_status`, `system_info`.
- **Round-display note:** all content must sit inside the **170px safe zone** — check nothing critical is clipped by the circular bezel.

---

## 3. Test matrix

Trigger → Expect → Verify. Mark ☐ Pass / ☐ Fail and note the screenshot filename.

### Boot & Running

| ID | Screen | Trigger | Expected | Verify | Result |
|----|--------|---------|----------|--------|--------|
| T1 | **Boot** | Power-on / reset | CB Labs logo (`cblabs_180x58`), title, progress bar advancing, status label; smooth fade-in overlay | `capture_display` during boot; logo not clipped | ☐ |
| T2 | **Running / Diag** | Complete configuring (see T3–T5) → app enters RUNNING | Diag screen loads (fade): edge **arc**, **category** label, large **value**, **status** line, **counter**. No leftover "Wizard/roller/touch-button" widgets (removed in redesign) | `capture_display`; serial `App transition → RUNNING` | ☐ |

### CONFIGURING — the new #82 work (highest priority)

| ID | Group | Trigger | Expected | Verify | Result |
|----|-------|---------|----------|--------|--------|
| T3 | **Setup** (WiFi wizard) | Factory-reset Wi‑Fi (MCP `config_factory_reset`, or first boot with no creds) → device enters SoftAP, `WiFi Station` gate = NeedsUserAction | System screen shows **Setup** group only: **AP SSID** (device hostname) + **config‑portal URL** (`http://<AP IP>`); WiFi icon; other groups hidden | `capture_display`; serial `CONFIGURING gate: WiFi Station (status=2)`; SSID/URL match `wifi_get_status` | ☐ |
| T4 | **Busy** (please‑wait) | While the device is actively connecting to a chosen network (`WiFi Station` = InProgress) | **Busy** group only: spinner animating + status/please‑wait message; Setup/Message hidden | `capture_display` mid-connect; serial `status=1` | ☐ |
| T5 | **Message** (generic gate helper) | A **non‑WiFi** gate reports NeedsUserAction (e.g. an app gate / Timezone if registered) | **Message** group only: icon + **title = gate name** + **body = help text**; not the Error screen | `capture_display`; serial `CONFIGURING gate: <name> (status=2)` | ☐ |
| T6 | **Configuring → group switching** | Move through gate states (NeedsUserAction ↔ InProgress) without leaving CONFIGURING | Correct group swaps **in place** on the single System screen; **no full-screen re-fade / flicker** on each re-publish (load-guard) | Watch transitions live; screenshots before/after | ☐ |

> If no non-WiFi gate is registered in the current app build, T5 can't be triggered as-is — note it as **N/A (no non-WiFi gate)** and, if desired, temporarily register a dummy `IConfigGate` to exercise the Message group.

### Error & OTA (re-skinned)

| ID | Screen | Trigger | Expected | Verify | Result |
|----|--------|---------|----------|--------|--------|
| T7 | **Error** | Force a `showError` path (e.g. induce a boot-stage failure, or an app error event) | **Error** group: icon, title, message, dismiss button; distinct from the Message group | `capture_display` | ☐ |
| T8 | **OTA** | Kick an OTA update (`/api/ota` / MCP `ota_get_status` flow) | **OTA** group: title, status text, **progress bar advancing**, **percentage** updating; render pauses appropriately during flash write | `capture_display` at ~30/60/90%; `ota_get_status` | ☐ |

### Input, screensaver, stability

| ID | Feature | Trigger | Expected | Verify | Result |
|----|---------|---------|----------|--------|--------|
| T9 | **Rotary encoder** | Rotate CW/CCW on an interactive screen | Focus/value responds; direction correct; detents feel right | Visual + serial `encoder.rotate` | ☐ |
| T10 | **Touch** | Tap the screen | Touch registers (wake from dim/blank; any touch handlers fire). *Note: the redesign removed the Diag touch-test button — verify touch still wakes/activates* | Visual; `display_status` | ☐ |
| T11 | **Screensaver — dim** | Idle to dim timeout (or MCP `display` action `sleep`/force-timeout) | Backlight dims; content retained | `display_status.dimmed == true` | ☐ |
| T12 | **Screensaver — blank & wake** | Idle to blank timeout; then touch/encoder | Screen blanks; input wakes it, restores prior screen | `display_status.sleeping`; wake restores | ☐ |
| T13 | **Stability / memory** | Run through T1–T12, then leave running ≥10 min | No watchdog (TWDT) resets, no crash loop; heap stays healthy | Serial clean; `system_info` heap headroom (design budget ~65 KB free at peak) | ☐ |

---

## 4. Exit criteria

- [ ] T1–T4, T6, T13 **pass** (boot, the core WiFi-setup CONFIGURING path, group-switching, and stability are the must-pass set).
- [ ] T5, T7, T8 pass or are recorded **N/A** with reason.
- [ ] All screenshots captured and attached; any deviation from SLS design intent logged as a defect (surface to the architect → framework issue).
- [ ] Firmware version on device matches the flashed dev tag.

## 5. Results log

| Date | Tag | Operator | Pass/Fail | Screenshots | Notes / defects raised |
|------|-----|----------|-----------|-------------|------------------------|
|      |     |          |           |             |                        |

## 6. Notes

- Nothing here has been executed — no CrowPanel was attached during the `sls-screen-redesign` implementation. This plan is the gate before trusting the CONFIGURING flow.
- Screenshot verification is the primary signal for the SLS visuals; serial + MCP status confirm the *logic* already covered by host unit tests (native 482/482).
- Defects found on hardware: surface to the PA/architect → filed as framework issues (do not fix ad hoc mid-test).
