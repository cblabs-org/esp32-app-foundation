---
name: pr-18-wo3
description: Handoff instructions for Work Order 3 — Create web portal Display tab with screensaver controls
---

# PR-18 / WO3 — Web Portal Display Tab

You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

---

## Objective

Create a new "Display" tab in the web portal with controls for brightness, flip,
screensaver enable/disable, dim timeout, and blank timeout. Follow the existing tab
pattern established by `wifi.js`, `time.js`, and `appconfig.js`. Merge the existing
App tab clock font control into this new Display tab (removing the standalone App tab
if it only had the clock font).

---

## Git Context

You are working on branch **`pr-18-wo3-display-tab`**. Check it out before making any changes:

```bash
git checkout pr-18-wo3-display-tab
```

Do NOT commit to any other branch. Do NOT merge.

---

## Strict Boundary Box

| ✅ You MAY touch | ❌ You MUST NOT touch |
|---|---|
| `web/portal/display.js` (NEW) | `modules/U8G2Display/ScreensaverController.*` |
| `web/portal/index.html` (add Display tab) | `modules/Application/*` |
| `web/portal/appconfig.js` (move clock font or remove) | `modules/MCPServer/*` |
| `test/web/tests/display.spec.js` (NEW Playwright tests) | C++ REST route implementations |
| `test/web/server.js` (add mock endpoints if needed) | |

---

## Pre-Work Reading (Mandatory)

1. **CRITICAL**: Read `.agents/workflows/web-testing-strategy.md` — the mandatory 3-phase testing workflow
2. `web/portal/index.html` — portal shell, tab registration pattern
3. `web/portal/appconfig.js` — existing App tab (clock font) as a pattern reference
4. `web/portal/wifi.js` — full-featured tab with form controls, save, load
5. `web/portal/time.js` — tab with dropdowns and API calls
6. `web/portal/utils.js` — shared utilities (`apiFetch`, toast, etc.)
7. `modules/U8G2Display/U8G2DisplayModule.cpp` lines 660–775 — REST API routes to call from JS
8. `test/web/tests/` — existing Playwright test files for patterns

---

## Implementation Checklist

### Step 1 — Create `display.js`

- [ ] Create `web/portal/display.js`
- [ ] Follow the existing tab module pattern (IIFE, `init()` function, event listeners)
- [ ] Register as a tab in `index.html`

**UI Controls** (all should load current state from `GET /api/display/status` on tab activate):

| Control | Type | API Endpoint | Notes |
|---|---|---|---|
| Brightness | Range slider 0-255 | `POST /api/display/brightness` `{"value": N}` | Show current value label |
| Flip Screen | Toggle switch | `POST /api/display/flip` `{"value": bool}` | On/Off |
| Screensaver Enabled | Toggle switch | `POST /api/display/sleep` `{"enabled": bool}` | On/Off — disables all timeout logic |
| Dim Delay | Dropdown or number input | `POST /api/display/sleep` `{"dimTimeout": ms}` | Presets: 1min, 2.5min, 5min, 10min, Custom |
| Blank Delay | Dropdown or number input | `POST /api/display/sleep` `{"blankTimeout": ms}` | Presets: 5min, 10min, 30min, Never |
| Clock Font Size | Radio group (S/M/L) | `POST /api/display/clockFontSize` `{"value": N}` | Migrated from App tab |
| Wake Display | Button | `POST /api/display/sleep` `{"action": "wake"}` | Immediate wake |
| Sleep Display | Button | `POST /api/display/sleep` `{"action": "sleep"}` | Immediate sleep |

- [ ] Group controls logically:
  - **Brightness & Orientation** — brightness slider, flip toggle
  - **Screensaver** — enable toggle, dim delay, blank delay, wake/sleep buttons
  - **Clock** — font size selector (migrated from App tab)

- [ ] Show current screensaver state indicator: "Active" / "Dimmed" / "Sleeping"

### Step 2 — Update `index.html`

- [ ] Add Display tab button to the tab bar
- [ ] Add Display tab content container
- [ ] Include `display.js` script
- [ ] If `appconfig.js` only contained clock font, remove the App tab and `appconfig.js` entirely

### Step 3 — Update mock server

- [ ] In `test/web/server.js`, add mock endpoints for any new API calls:
  - Ensure `GET /api/display/status` returns the expanded fields (`screensaverEnabled`, `dimmed`, `dimTimeout`, `blankTimeout`)
  - Ensure `POST /api/display/sleep` handles the new `enabled`, `dimTimeout`, `blankTimeout` fields

### Step 4 — Playwright Tests

- [ ] Create `test/web/tests/display.spec.js`
- [ ] Test cases (minimum):
  - Display tab loads and shows current brightness value
  - Brightness slider change sends POST to `/api/display/brightness`
  - Flip toggle sends POST to `/api/display/flip`
  - Screensaver enable/disable toggle sends POST to `/api/display/sleep` with `enabled` field
  - Dim delay change sends POST with `dimTimeout` field
  - Blank delay change sends POST with `blankTimeout` field
  - Wake button sends `{"action": "wake"}`
  - Sleep button sends `{"action": "sleep"}`
  - Clock font selector sends POST to `/api/display/clockFontSize`
  - State indicator shows correct screensaver status

---

## Design Notes

- Use the same CSS variables and card styling as the existing tabs
- The portal is a single-page app with inline CSS — no external CSS framework
- All API calls use `window.apiFetch()` from `utils.js`
- Show toast notifications on save success/failure (use existing `showToast()`)
- Dim/Blank delay dropdowns should show human-readable labels ("2.5 minutes") but send millisecond values

---

## Verification Requirement

```bash
# Run web tests
npm --prefix test/web test

# Hardware build (to verify portal changes compile into LittleFS)
pio run -e esp32s3nano
```

---

## Completion Protocol

1. Verify web tests pass and hardware build succeeds
2. **STOP. Send a message back to the Principal Architect** with:
   - Test results (web tests + build)
   - Screenshot of the Display tab in the portal
   - Any deviations from the checklist
   - A request for approval before wrapping up

> Do NOT distill, do NOT commit, do NOT merge until the Principal Architect explicitly approves.

---

## Status

_(To be filled in by worker agent on completion)_
