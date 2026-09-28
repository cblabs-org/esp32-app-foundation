---
name: pr-225-ble-manager-wo7
description: Handoff instructions for Work Order 7 — BLE portal tab redesign
complexity: standard
---

# WO-7 — BLE Portal Tab Redesign

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Redesign the BLE portal tab to be a lightweight, framework-appropriate overview with 3 cards + a diagnostics drawer, and fix the bottom nav ordering so Bluetooth appears immediately after WiFi.

## Strict Boundary Box

**IN scope:** Only the 5 files listed in the Line-Item Instructions below.

**OUT of scope — do NOT touch:**
- Any C++ module files beyond the two listed below (`BLEManager.cpp`, `BLESerialization.cpp`)
- Any other portal tabs (`wifi.js`, `ota.js`, `security.js`, `system.js`)
- Any framework core files (`Application.*`, `AppFoundation.h`, `include/interfaces/`)
- PlatformIO configuration files
- Any documentation or skill files
- Do not create new files — all changes are modifications to existing files
- Do not change any REST API endpoints or MCP tools

## Tooling Constraints

- Use the `platformio` MCP tools (`build_project`, `check_task_status`) for build verification — never raw `pio` CLI commands.
- Build env: `esp32s3nano_ble`
- After editing `ble.js`, the portal builder (`scripts/portalBuilder.py`) runs automatically during build and regenerates the gzipped C++ header.

## Design System

**You MUST read** `.agents/skills/framework-portal-design/SKILL.md` and its `readme.md` before writing any HTML/CSS/JS. Key constraints:

- 480px mobile column, card-based layout
- Material Blue `#2196F3` accent, dark/light theme via `[data-theme]`
- Feather icons (24×24 SVG, stroke-width: 2, `currentColor`)
- System fonts only (no webfonts)
- Destructive actions: armed confirmation pattern (3.5s timeout)
- API calls: `window.apiFetch(url, options)` — handles auth, error toasts
- User feedback: `window.showToast(message, type)`
- Tab lifecycle: `window.addEventListener('tab-changed', ...)`, scaffold on first activation, 1 Hz polling when visible
- **Reference implementations** — read these for patterns: `web/portal/wifi.js`, `modules/OTAUpdateManager/web/ota.js`

## Backend API Context

These APIs already exist and are unchanged. Use them as-is:

| Endpoint | Method | Notes |
|----------|--------|-------|
| `/api/ble/status` | GET | Returns state, isScanning, isAdvertising, advertisedName, advertising{}, connections[], devices[], counters{}, discovery{} — **plus `bleAddress` after your BLESerialization change** |
| `/api/ble/scan` | POST | Body: `{ providerId, durationMs }` |
| `/api/ble/scan/results` | GET | Returns scan results array |
| `/api/ble/pair` | POST | Body: `{ providerId, address, addressType }` |
| `/api/ble/forget` | POST | Body: `{ providerId }` |

## Line-Item Instructions

### Part A — Bottom Nav Ordering

- [ ] **`web/portal/index.html`** (~lines 122–158) — Add `data-order` attributes to the hardcoded nav links (these are `<a class="tab-link">` elements with `data-target` attributes):
  - WiFi link (`data-target="tab-wifi"`): add `data-order="10"`
  - Security link (`data-target="tab-security"`): add `data-order="50"`
  - System link (`data-target="tab-system"`): add `data-order="900"`
  - Keep all existing attributes, classes, SVG icons, and label text unchanged.

- [ ] **`web/portal/app.js`** (~lines 306–366, `buildBottomNav()`) — Change the dynamic tab insertion logic to be order-aware:
  - Each dynamic tab has an `order` value (from `dynamicTabsConfig`).
  - Instead of simply appending dynamic tab links into `#dynamic-nav-links`, walk ALL nav links (both hardcoded and dynamic) by their `data-order` attribute and insert the new link before the first link with a higher order value.
  - If no link has a higher order, append at the end.
  - Set `data-order` on each dynamically-created link too (from the tab config's order value).
  - The insertion target is the parent `<nav>` element, not just the `#dynamic-nav-links` div. Dynamic links should be inserted into the correct position among ALL nav links.

- [ ] **`modules/BLEManager/BLEManager.cpp`** (~lines 1294–1302, `registerTabs()`) — Change the order parameter from `60` to `15` in the `addTab()` call:
  ```cpp
  registry.addTab(
      "Bluetooth",
      nullptr,
      15,   // was 60; WiFi=10, Security=50, System=900
      "<polyline points=\"6.5 6.5 17.5 17.5 12 23 12 1 17.5 6.5 6.5 17.5\"></polyline>",
      "/ble.js"
  );
  ```

### Part B — Backend: Add BLE Address to Status API

- [ ] **`modules/BLEManager/BLESerialization.cpp`** (in `serializeStatus()`) — Add a `bleAddress` field:
  - Add `#include <NimBLEDevice.h>` at the top of the file (if not already present) — guard with `#if defined(ESP_PLATFORM) && defined(WITH_BLE)`.
  - Inside `serializeStatus()`, add: `doc["bleAddress"] = NimBLEDevice::getAddress().toString();`
  - Place it near the other top-level status fields (state, isScanning, isAdvertising, advertisedName).

### Part C — Portal Page Rewrite (`ble.js`)

- [ ] **`modules/BLEManager/web/ble.js`** — Full rewrite. Replace the entire file with a new implementation following the 3-card + drawer design:

  **Card 1: Status Overview** — "Bluetooth" header with status dot
  - BLE Address (monospace font) — from `status.bleAddress`
  - Advertised Name — from `status.advertisedName`
  - Advertising indicator (Active/Stopped badge) — from `status.isAdvertising`

  **Card 2: Paired Devices** (conditional — only render if `status.devices.length > 0`)
  - Header: "Paired Devices"
  - Per provider: label + status badge (Connected/Disconnected/Reconnecting) + stored device address
  - Scan button per provider (triggers `POST /api/ble/scan` with that provider's ID)
  - Forget button per provider (destructive — use armed confirmation pattern)
  - Inline scan results table with Pair buttons (after a scan completes)

  **Card 3: Connections** — "Connections" header with count badge
  - List of active peripheral connections: redacted address (show last 4 chars only, e.g. `••:••:••:8E:92`), handle number, encrypted/plaintext badge
  - Empty state: "No active connections" centered text

  **Drawer: Diagnostics** (collapsed by default, use `<details>/<summary>`)
  - Dropped Events count
  - Scan Inhibit count
  - Advertising budget bars (used/max bytes for advertising data and scan response)
  - Faults list (if any)

  **Implementation requirements:**
  - Follow the tab lifecycle pattern from `wifi.js`: scaffold HTML on first `tab-changed` event, start 1 Hz polling on activation, stop on deactivation.
  - Use `window.apiFetch` for all API calls.
  - Use `window.showToast` for success/error feedback on actions.
  - All CSS must be inline in the JS (injected via `<style>` tag on first scaffold, scoped with a `#ble-tab` prefix).
  - Dark/light theme support via `[data-theme="dark"]` selectors.
  - Keep the file well-structured: init function, polling functions, render functions (one per card), action handlers.
  - Feather SVG icons where appropriate (bluetooth, wifi-off for disconnected, shield for encrypted, etc.).

## Verification Requirement

1. Build the project using `build_project` MCP tool with env `esp32s3nano_ble`. Verify clean compilation with no errors.
2. The portal builder will automatically regenerate `include/portal_assets/ble_js.hpp` during build — verify this file is updated.
3. **CRITICAL:** Before committing, you MUST ask the user if they are happy with the changes. Present a summary of all modifications made.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report

### Status
complete

### Changes Made
- `web/portal/index.html` — Added `data-order` attributes (`10` on WiFi, `50` on Security, `900` on System) to hardcoded nav links.
- `web/portal/app.js` — Updated `buildBottomNav()` to be order-aware, inserting dynamic tab links before the first link with higher `data-order` attribute or appending at the end.
- `modules/BLEManager/BLEManager.cpp` — Changed tab order parameter from 60 to 15 in `registerTabs()`.
- `modules/BLEManager/BLESerialization.cpp` — Added `bleAddress` field to `serializeStatus()`, populated from `NimBLEDevice::getAddress().toString()` guarded with `#if defined(ESP_PLATFORM) && defined(WITH_BLE)` (and empty string for native).
- `modules/BLEManager/web/ble.js` — Complete rewrite of BLE portal tab into 3 cards + diagnostics drawer following `framework-portal-design`:
  - Card 1: Status Overview (Bluetooth header + dot, BLE Address in monospace, Advertised Name, Advertising indicator badge)
  - Card 2: Paired Devices (conditional — displayed only when `devices.length > 0`, showing provider label, status badge, stored device, armed Forget button with 3.5s timeout, Scan button with timer, inline scan results table with Pair buttons)
  - Card 3: Connections (Connections header + count badge, active peripheral connections list with redacted address `••:••:••:8E:92`, handle number, interval, encrypted/plaintext badge, empty state)
  - Drawer: Diagnostics (`<details>/<summary>Diagnostics</summary>` with Dropped Events, Scan Inhibit count, Advertising budget bars with progress fill and warning color on overflow, Advertising faults list)
  - Scoped styles with `#ble-tab` prefix, dark/light theme support via `[data-theme="dark"]`
  - Lifecycle hooks: `tab-changed` listener for `tab-bluetooth`, scaffold on first activation, 1 Hz polling when visible, stops when hidden/deactivated
- `test/test_unit/test_bleManager/test_bleSerialization.cpp` — Updated `RegisterTabsMetadata` test assertion to expect order `15` instead of `60`.

### Decisions Made
- Guarded `<NimBLEDevice.h>` include and `NimBLEDevice::getAddress()` in `BLESerialization.cpp` with `#if defined(ESP_PLATFORM) && defined(WITH_BLE)` to preserve native unit test compatibility per D6.
- Used `#ble-tab` CSS namespace and CSS custom properties (`var(--primary)`, `var(--border)`, `var(--card-bg)`, etc.) to seamlessly support portal theme switching (`[data-theme="dark"]`).

### Dead Ends & Warnings
- (none)

### Out-of-Scope Findings
- `test_unit/test_lvgl_stubs` currently fails in native test suite due to missing include dependencies (`WifiManager.hpp` / `ConfigManager.hpp`) in `MCPServer`. Unrelated to BLEManager.

### Blockers
(none)
