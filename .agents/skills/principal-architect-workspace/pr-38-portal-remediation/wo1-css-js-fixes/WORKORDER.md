---
name: pr38-wo1
description: Critical CSS/JS fixes for PR-35 portal tabs — undefined classes, duplicate event listeners, lifecycle alignment, hardcoded colours
complexity: standard
---

# WO1 — Critical CSS/JS Fixes

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the 6 critical/medium portal bugs identified in the PR-35 deep review: undefined CSS classes (P1), duplicate JS event listeners (P2), wrong CSS variable name (P3), bare `fetch()` usage (P6), hardcoded colours (P7), and non-standard tab lifecycle (P9).

## Strict Boundary Box

- **DO** modify: `web/portal/homeassistant.js`, `sample-app/src/WeatherModule.cpp` (inline JS only)
- **DO NOT** modify: any C++ backend logic, any other portal JS files, `app.css`, `index.html`, the portal build pipeline, any test files
- **DO NOT** add new files or new REST endpoints
- **DO NOT** add instruction boxes, status badges, or other information-depth features (that's WO2)
- **DO NOT** touch tab icons or branding (that's WO3)

## Tooling Constraints

- Use PlatformIO MCP tools (`build_project`, `upload_firmware`, `start_monitor`, `query_logs`) for all build/flash/monitor operations — never raw `pio` CLI
- Read the `pio-manager` skill at `.agents/skills/pio-manager/SKILL.md` before any hardware operation
- Use `acquire_lock` before and `release_lock` after any PlatformIO operation
- The sample-app `platformio.ini` is at `sample-app/platformio.ini` — build environment is `esp32dev`

## Reference Material

Before making changes, read these files to understand the design system patterns:

1. `web/portal/app.css` — the CSS design system (card = `<article>`, header = `<header>`, status colours = `var(--success)`, `var(--error)`)
2. `web/portal/wifi.js` lines 1-200 — the canonical tab lifecycle pattern
3. `web/portal/system.js` lines 1-100 — another reference for lifecycle + helpers

## Line-Item Instructions

### 1. Fix `homeassistant.js` — Full Rewrite

Replace the entire contents of `web/portal/homeassistant.js` with a clean implementation that:

- [x] **Removes duplicate event listeners** (P2) — The current file has event handlers at lines 131-190 that are OUTSIDE the `tab-changed` scope and reference undefined variables. Delete all code outside the IIFE's `tab-changed` handler.
- [x] **Converts to design system HTML** (P1):
  - `<div class="card">` → `<article>`
  - `<div class="card-body">` → remove (content goes directly in `<article>`)
  - `<div class="form-group">` → `<label>` wrapping input (framework pattern: `<label>Label Text<input .../></label>`)
  - Keep `<header>` as-is (it's correct)
- [x] **Fixes `--danger` → `--error`** (P3) — All error colour references must use `var(--error)` not `var(--danger)`
- [x] **Aligns to standard lifecycle** (P9):
  - Render HTML scaffold immediately at IIFE execution (don't defer to `tab-changed`)
  - Use `tab-changed` event ONLY for loading data via API
  - Add the "already-active" check: `if (container.classList.contains('active')) loadData();`
- [x] **Uses `window.showToast()` for save feedback** — Replace the inline `<div id="ha-save-result">` pattern with `window.showToast('Settings saved.', 'success')` and `window.showToast('Error: ' + err.message, 'error')`
- [x] **Uses `window.apiFetch()`** for all API calls (not bare `fetch()`)

**Target structure:**
```js
// web/portal/homeassistant.js
(function() {
  const container = document.getElementById('tab-home-assistant');
  if (!container) return;

  // 1. Render scaffold immediately
  container.innerHTML = `
    <article>
      <header>Home Assistant MQTT</header>
      <label>MQTT Broker URI
        <input type="text" id="ha-uri" placeholder="mqtts://broker.hivemq.com:8883" />
      </label>
      <label>Username (Optional)
        <input type="text" id="ha-user" placeholder="username" />
      </label>
      <label>Password (Optional)
        <input type="password" id="ha-pass" placeholder="Leave blank to keep existing" />
      </label>
      <button type="button" id="ha-save-btn">Save Settings</button>
    </article>

    <article>
      <header>TLS Certificate</header>
      <p style="font-size:0.85rem; color:var(--text-muted); margin-bottom:1rem;">
        Upload the Root CA certificate for the MQTT broker (PEM format) if using mqtts://.
      </p>
      <label>Certificate (PEM)
        <textarea id="ha-cert" rows="5" placeholder="-----BEGIN CERTIFICATE-----\n..."></textarea>
      </label>
      <button type="button" id="ha-cert-btn">Upload Certificate</button>
    </article>
  `;

  // 2. Wire event listeners
  const uriInput  = document.getElementById('ha-uri');
  const userInput = document.getElementById('ha-user');
  const passInput = document.getElementById('ha-pass');
  const saveBtn   = document.getElementById('ha-save-btn');
  const certInput = document.getElementById('ha-cert');
  const certBtn   = document.getElementById('ha-cert-btn');

  saveBtn.addEventListener('click', async () => { /* save logic using apiFetch + showToast */ });
  certBtn.addEventListener('click', async () => { /* cert logic using apiFetch + showToast */ });

  // 3. Data loading function
  async function loadData() {
    try {
      const data = await window.apiFetch('/api/homeassistant');
      if (data.uri) uriInput.value = data.uri;
      if (data.user) userInput.value = data.user;
      if (data.passExists) passInput.placeholder = '•••••••• (Saved)';
    } catch (err) {
      console.error('Failed to load HA config', err);
    }
  }

  // 4. Load on activation + already-active check
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-home-assistant') loadData();
  });
  if (container.classList.contains('active')) loadData();
})();
```

### 2. Fix WeatherModule.cpp inline JS

In `sample-app/src/WeatherModule.cpp`, fix ONLY the inline JavaScript (the `R"rawliteral(...)rawliteral"` block, approximately lines 114-192):

- [x] **Convert `<div class="card">` → `<article>`** (P1)
- [x] **Convert `<h2>` → `<header>`** (P1)
- [x] **Remove `class="form-group"` divs** — use `<label>` wrapping input instead (P1)
- [x] **Remove `class="btn btn-primary"`** — bare `<button>` is already primary-styled (P1)
- [x] **Fix hardcoded colours** (P7): `color: "green"` → `color: "var(--success)"`, `color: "red"` → `color: "var(--error)"`
- [x] **Use `window.apiFetch`** instead of bare `fetch()` for the config GET and POST (P6)
- [x] **Use `window.showToast()`** — already partially done, verify both success and error paths use it
- [x] **Align lifecycle** (P9): Render scaffold at load time, use `tab-changed` for data loading only, add already-active check

### 3. Rebuild Portal Assets

After modifying `homeassistant.js`, the portal build pipeline must be re-run:

```bash
python3 scripts/portalBuilder.py
```

This regenerates `include/portal_assets/homeassistant_js.hpp` (and others). Commit the regenerated `.hpp` files.

### 4. Build & Flash

- Build the project using PlatformIO MCP `build_project` with environment `esp32dev` and project dir `sample-app`
- Flash using PlatformIO MCP `upload_firmware` (do NOT upload filesystem — preserves existing config)

### 5. Hardware Verification

After flashing, verify via:

1. **Serial monitor** (`start_monitor` / `query_logs`): Clean boot, no crash, modules register
2. **Web portal** (open in browser): 
   - Navigate to Home Assistant tab — verify `<article>` cards render with background, border, border-radius
   - Navigate to Weather tab — verify same card styling
   - Toggle dark mode — verify no hardcoded green/red colours bleed through
   - Save settings on HA tab — verify toast notification appears (not inline div)
3. **MCP server**: Call `system_info` to confirm firmware version incremented

## Verification Requirement

Before reporting back to the PA, you must confirm:
- [ ] `homeassistant.js` has zero duplicate code, uses only design system classes, uses `apiFetch`/`showToast`
- [ ] WeatherModule inline JS uses `<article>`/`<header>`, CSS variables, `apiFetch`/`showToast`
- [ ] Portal assets rebuilt (`portalBuilder.py` run, `.hpp` files committed)
- [ ] Build succeeds on `esp32dev`
- [ ] Firmware flashed and device boots cleanly
- [ ] Both tabs render with correct card styling in light AND dark mode
- [ ] Toast notifications work for save/error actions

**CRITICAL:** Once verified, STOP and send a message back to the Principal Architect. Do not distill or wrap up until the PA explicitly approves.
