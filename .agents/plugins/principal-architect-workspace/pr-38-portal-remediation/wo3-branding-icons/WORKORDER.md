---
name: pr38-wo3
description: Portal branding and custom tab icons — appName in FrameworkConfig, portal title override, getTabIcon() for HA (house) and Display (monitor)
complexity: standard
---

# WO3 — Branding & Tab Icons

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Make the portal title application-overridable and add recognisable custom icons to the Home Assistant and Display bottom-nav tabs. Covers findings P10, P11, P12 from the PR-35 deep review.

## Strict Boundary Box

- **DO** modify: `include/Application.h`, `include/Application.cpp` (or wherever Application is defined), `modules/WebServer/WebServerManager.cpp` (system API response), `web/portal/app.js` (title injection), `modules/HomeAssistant/HomeAssistantModule.h` and `.cpp` (add `getTabIcon()`), `modules/U8G2Display/U8G2DisplayModule.h` and `.cpp` (add `getTabIcon()`)
- **DO NOT** modify: `app.css`, `index.html` (the `id="portal-title"` placeholder already exists), any other portal JS files, any entity classes, any MQTT/weather logic
- **DO NOT** add new CSS classes or modify the portal build pipeline
- **DO NOT** modify `FrameworkConfig` — use `Application` instead (FrameworkConfig is for OTA secrets only)

## Tooling Constraints

- Use PlatformIO MCP tools (`build_project`, `upload_firmware`, `start_monitor`, `query_logs`) for all build/flash/monitor operations — never raw `pio` CLI
- Read the `pio-manager` skill at `.agents/skills/pio-manager/SKILL.md` before any hardware operation
- Use `acquire_lock` before and `release_lock` after any PlatformIO operation
- The sample-app `platformio.ini` is at `sample-app/platformio.ini` — build environment is `esp32dev`

## Reference Material

Before making changes, read these files:

1. `include/interfaces/IWebContributor.hpp` — the `getTabIcon()` contract (lines 93-104): returns inner SVG content, outer `<svg>` wrapper added by client
2. `modules/AuthManager/AuthManager.cpp` — search for `getTabIcon()` to see the ONLY existing override (padlock icon) — use this as the reference pattern
3. `modules/WebServer/WebServerManager.cpp` lines 280-330 — how tabs and `/api/system` are built
4. `web/portal/app.js` — search for `portal-title` or `tabs` to understand how the client renders the navigation
5. `include/Application.h` — the Application class definition
6. `sample-app/src/main.cpp` — how the sample app creates Application to see where `appName` would be set

## Line-Item Instructions

### 1. Add `appName` to Application

#### a) Modify `include/Application.h`
Add a new method and member:
```cpp
/** @brief Set the application name displayed in the portal title bar. */
void setAppName(const char* name) { _appName = name; }

/** @brief Get the application name. Returns "Device Portal" if not set. */
const char* getAppName() const { return _appName ? _appName : "Device Portal"; }
```
Add the member variable:
```cpp
const char* _appName = nullptr;
```

#### b) Set in Sample App
In `sample-app/src/main.cpp`, after the `Application app;` declaration (or in `setup()`), add:
```cpp
app.setAppName("Weather Station");
```

### 2. Wire Portal Title from `/api/system`

#### a) Modify WebServerManager `/api/system` response
In `modules/WebServer/WebServerManager.cpp`, find the `GET /api/system` handler. Add `appName` to the JSON response. The WebServerManager needs access to Application's `getAppName()`. Check how other data (hostname, version) is accessed — likely via the Application reference or a static accessor. Add:
```cpp
doc["appName"] = app.getAppName();  // or however Application is accessible
```

#### b) Modify `app.js` to set portal title
In `web/portal/app.js`, find where the system API is fetched (search for `/api/system` or `fetchSystem` or `init`). After the response is received, add:
```js
if (data.appName) {
  document.getElementById('portal-title').textContent = data.appName;
  document.title = data.appName;
}
```

### 3. Add `getTabIcon()` to HomeAssistantModule

#### a) In the `.h` file
Add the override declaration:
```cpp
const char* getTabIcon() const override;
```

#### b) In the `.cpp` file
Add the implementation using a Home Assistant-style house icon (Feather `home` icon, 24x24 viewBox):
```cpp
const char* HomeAssistantModule::getTabIcon() const {
    return "<path d=\"M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z\"/>"
           "<polyline points=\"9 22 9 12 15 12 15 22\"/>";
}
```
Note: Only the INNER SVG content is returned. The outer `<svg viewBox="0 0 24 24" ...>` wrapper is added by `app.js`.

### 4. Add `getTabIcon()` to U8G2DisplayModule

#### a) In the `.h` file
Add the override declaration:
```cpp
const char* getTabIcon() const override;
```

#### b) In the `.cpp` file
Add the implementation using a monitor/display icon (Feather `monitor` icon):
```cpp
const char* U8G2DisplayModule::getTabIcon() const {
    return "<rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"2\" ry=\"2\"/>"
           "<line x1=\"8\" y1=\"21\" x2=\"16\" y2=\"21\"/>"
           "<line x1=\"12\" y1=\"17\" x2=\"12\" y2=\"21\"/>";
}
```

### 5. Rebuild Portal Assets

After modifying `app.js`:
```bash
python3 scripts/portalBuilder.py
```
Commit the regenerated `.hpp` files.

### 6. Build & Flash

- Build via PlatformIO MCP `build_project` (env: `esp32dev`, project dir: `sample-app`)
- Flash via PlatformIO MCP `upload_firmware` (do NOT upload filesystem)

### 7. Hardware Verification

1. **Serial monitor**: Clean boot, all modules register
2. **Web portal — Title**: Portal shows "Weather Station" instead of "Device Portal"
3. **Web portal — Browser tab**: HTML `<title>` also shows "Weather Station"
4. **Web portal — Bottom nav**: 
   - Home Assistant tab has a house icon (not the generic grid)
   - Display tab has a monitor icon (not the generic grid)
   - WiFi (arcs), Security (padlock), System (gear) remain unchanged
5. **Dark mode**: Icons and title render correctly in dark mode
6. **MCP server**: `system_info` response includes `appName: "Weather Station"`

## Verification Requirement

- [ ] `Application` has `setAppName()` / `getAppName()` methods
- [ ] Sample app sets `app.setAppName("Weather Station")`
- [ ] `/api/system` response includes `appName` field
- [ ] Portal title dynamically set from API response
- [ ] HA tab has house icon in bottom nav
- [ ] Display tab has monitor icon in bottom nav
- [ ] All existing tab icons (WiFi, Security, System) unchanged
- [ ] Build succeeds, firmware flashed, device boots cleanly

**CRITICAL:** Once verified, STOP and send a message back to the Principal Architect. Do not distill or wrap up until the PA explicitly approves.
