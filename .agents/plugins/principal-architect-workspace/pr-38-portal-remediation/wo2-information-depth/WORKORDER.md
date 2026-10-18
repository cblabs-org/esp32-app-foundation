---
name: pr38-wo2
description: Information depth enhancement for PR-35 portal tabs — instruction boxes, MQTT status badge, API key indicator, eye-btn, toast consistency
complexity: standard
---

# WO2 — Information Depth Enhancement

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add the missing information hierarchy elements to the Home Assistant and Weather portal tabs, bringing them to parity with the departures-board reference design and existing framework tabs (WiFi, System, Security). Covers findings P4 and P5 from the PR-35 deep review.

## Strict Boundary Box

- **DO** modify: `web/portal/homeassistant.js`, `sample-app/src/WeatherModule.cpp` (inline JS only), `modules/HomeAssistant/HomeAssistantModule.cpp` (REST endpoint enhancement only)
- **DO NOT** modify: `app.css`, `index.html`, `app.js`, or any other portal JS files
- **DO NOT** touch tab icons, branding, or the portal title (that's WO3)
- **DO NOT** modify C++ entity classes, MQTT connection logic, or weather data fetching logic
- **DO NOT** add new CSS classes — use only existing classes from `app.css`

## Tooling Constraints

- Use PlatformIO MCP tools (`build_project`, `upload_firmware`, `start_monitor`, `query_logs`) for all build/flash/monitor operations — never raw `pio` CLI
- Read the `pio-manager` skill at `.agents/skills/pio-manager/SKILL.md` before any hardware operation
- Use `acquire_lock` before and `release_lock` after any PlatformIO operation
- The sample-app `platformio.ini` is at `sample-app/platformio.ini` — build environment is `esp32dev`

## Reference Material

Before making changes, read these files to understand the information hierarchy patterns:

1. `web/portal/app.css` sections 9c-9d — `.info-drawer` and `.advanced-drawer` patterns
2. `web/portal/app.css` section 10 — `.status-badge` and `.dot` patterns
3. `web/portal/wifi.js` lines 73-200 — WiFi tab scaffold with status header, network list, password wrapper
4. `web/portal/app.css` lines 188-228 — `.password-input-wrapper` and `.eye-btn` component

## Line-Item Instructions

### 1. Home Assistant Tab — Add Information Depth

Modify `web/portal/homeassistant.js` (which was already cleaned up in WO1) to add:

#### a) Instruction Box (before the MQTT form)
Add an instruction box at the top of the first `<article>`, BEFORE the form fields:
```html
<div style="background:var(--primary-alpha); border-left:4px solid var(--primary); padding:1rem; border-radius:8px; font-size:0.85rem; color:var(--text); margin-bottom:1.2rem; line-height:1.4;">
  Configure the <strong>MQTT broker</strong> connection for Home Assistant auto-discovery. 
  Entities registered by your application will appear automatically in Home Assistant.
</div>
```

#### b) MQTT Connection Status Badge (top of first article)
Add a status badge showing the current MQTT connection state. Place it right after the `<header>`:
```html
<div class="status-badge" id="ha-mqtt-status" style="margin-bottom:1rem;">
  <div class="dot grey" id="ha-mqtt-dot"></div>
  <span id="ha-mqtt-text" style="text-transform:uppercase; font-size:0.75rem; letter-spacing:1px;">Checking...</span>
</div>
```

Update the `loadData()` function to also set the MQTT status:
```js
// Inside loadData(), after loading config:
const mqttDot = document.getElementById('ha-mqtt-dot');
const mqttText = document.getElementById('ha-mqtt-text');
if (data.mqttConnected) {
  mqttDot.className = 'dot green';
  mqttText.textContent = 'Connected';
} else {
  mqttDot.className = 'dot red';
  mqttText.textContent = 'Disconnected';
}
```

#### c) Backend Enhancement — Expose MQTT Status
In `modules/HomeAssistant/HomeAssistantModule.cpp`, find the `GET /api/homeassistant` handler and add `mqttConnected` to the JSON response. The module already has a `_mqttConnected` member or equivalent — just expose it:
```cpp
doc["mqttConnected"] = _mqttClient.connected();  // or however the connection state is tracked
```

Read the `HomeAssistantModule.cpp` file to find the exact variable/method name. Do NOT modify the MQTT connection logic itself.

#### d) Password Eye Toggle for MQTT Password
Replace the bare `<input type="password" id="ha-pass" ...>` with the framework's `.password-input-wrapper` + `.eye-btn` pattern. Reference `web/portal/wifi.js` for the exact implementation (search for `password-input-wrapper` and `eye-btn`). The SVG icons for eye-on and eye-off are:
```html
<button type="button" class="eye-btn" id="ha-pass-eye" title="Toggle Visibility">
  <svg viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>
</button>
```
Wire the toggle logic: clicking toggles `ha-pass` between `type="password"` and `type="text"`.

### 2. Weather Tab — Add Information Depth

Modify the inline JS in `sample-app/src/WeatherModule.cpp` to add:

#### a) Instruction Box
Add an instruction box at the top of the `<article>`:
```html
<div style="background:var(--primary-alpha); border-left:4px solid var(--primary); padding:1rem; border-radius:8px; font-size:0.85rem; color:var(--text); margin-bottom:1.2rem; line-height:1.4;">
  Configure the weather data source. You need a free API key from 
  <a href="https://openweathermap.org/appid" target="_blank" style="color:var(--primary); font-weight:700; text-decoration:none;">OpenWeatherMap</a>.
</div>
```

#### b) API Key Status Indicator
Replace the `<small id="weather-key-status"></small>` with a proper status badge:
```html
<div class="status-badge" id="weather-key-status" style="margin-top:0.5rem;">
  <div class="dot grey" id="weather-key-dot"></div>
  <span id="weather-key-text" style="font-size:0.75rem; text-transform:uppercase; letter-spacing:0.5px;">Checking...</span>
</div>
```
Update the fetch callback to set:
```js
if (data.hasApiKey) {
  document.getElementById('weather-key-dot').className = 'dot green';
  document.getElementById('weather-key-text').textContent = 'Key Configured';
} else {
  document.getElementById('weather-key-dot').className = 'dot red';
  document.getElementById('weather-key-text').textContent = 'No Key Set';
}
```

### 3. Rebuild Portal Assets

After modifying `homeassistant.js`:
```bash
python3 scripts/portalBuilder.py
```
Commit the regenerated `.hpp` files.

### 4. Build & Flash

- Build via PlatformIO MCP `build_project` (env: `esp32dev`, project dir: `sample-app`)
- Flash via PlatformIO MCP `upload_firmware` (do NOT upload filesystem)

### 5. Hardware Verification

1. **Serial monitor**: Clean boot, HA module registers, MQTT attempts connection
2. **Web portal — HA tab**:
   - Instruction box visible with blue left border
   - MQTT status badge shows Connected (green dot) or Disconnected (red dot)
   - Password field has eye-toggle button
   - Saving settings shows toast notification
3. **Web portal — Weather tab**:
   - Instruction box visible with OWM link
   - API key status shows green/red dot indicator
4. **Dark mode**: All new elements respect theme (no hardcoded colours)
5. **MCP server**: `system_info` confirms new firmware version

## Verification Requirement

- [ ] HA tab has instruction box, MQTT status badge, eye-btn password toggle
- [ ] Weather tab has instruction box, API key status badge
- [ ] `GET /api/homeassistant` response includes `mqttConnected` field
- [ ] All new UI elements use existing CSS classes from `app.css` (no new classes added)
- [ ] Build succeeds, firmware flashed, device boots cleanly
- [ ] All visual elements work in both light and dark mode

**CRITICAL:** Once verified, STOP and send a message back to the Principal Architect. Do not distill or wrap up until the PA explicitly approves.
