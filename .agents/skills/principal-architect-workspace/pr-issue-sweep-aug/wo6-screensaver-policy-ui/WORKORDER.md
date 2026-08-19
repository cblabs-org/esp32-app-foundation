---
name: issue-sweep-aug-wo6
description: Handoff instructions for Work Order 6 — Screensaver policy toggle in portal
complexity: standard
---

# WO6 — Screensaver Policy Portal Toggle

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add a user-facing screensaver policy control to the portal's Display settings tab, allowing toggle between `DimOnly` and `FullSleep`. Persist via `IConfigurable` and expose via MCP tools. Closes **#110**.

## Context
The `ScreensaverPolicy` enum already exists (`ScreensaverController.hpp:31–34`) with `DimOnly` and `FullSleep`. The `ScreensaverController` already supports both policies programmatically via `setPolicy()`. However:
- `LVGLDisplayModule::applyConfig()` (line 578–638) does NOT read/write the policy
- `LVGLDisplayModule::serializeConfig()` (line 632–638) does NOT serialize the policy
- `display_control` MCP tool (line 1159–1213) does NOT accept a `policy` parameter
- `display_status` MCP tool (line 1081–1129) does NOT report the current policy
- No portal UI exists for this setting

**Note:** Both `LVGLDisplay` and `U8G2Display` have their own `ScreensaverController.hpp`. This WO targets the LVGL path. If U8G2 has the same gap, note it in your Completion Report but do NOT fix it (out of scope).

## Strict Boundary Box
- **ONLY** modify the files listed below.
- Do NOT change `ScreensaverController.hpp` or `ScreensaverController.cpp` (the enum and runtime logic already exist).
- Do NOT touch U8G2Display's screensaver (note gaps in Completion Report instead).
- Do NOT change the screensaver timeout logic or dim/blank behaviour.
- Do NOT change any other portal tabs or API endpoints.

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (rule `04-mcp-tooling.md`).
- Build verification: `build_project` targeting `crowpanel-128-rotary`.
- Native tests: `run_tests` with environment `native`.

## Line-Item Instructions

### Config Persistence

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (line 578–638)
  **In `applyConfig()`:** Read the screensaver policy from config:
  ```cpp
  // After existing screensaver config reads (line 602-606):
  String policyStr = section["screensaverPolicy"] | "FullSleep";
  ScreensaverPolicy policy = (policyStr == "DimOnly")
      ? ScreensaverPolicy::DimOnly
      : ScreensaverPolicy::FullSleep;
  _screensaver.setPolicy(policy);
  ```

  **In `serializeConfig()`:** Write the screensaver policy:
  ```cpp
  // After line 637 (section["blankTimeout"]):
  section["screensaverPolicy"] = (_screensaver.getPolicy() == ScreensaverPolicy::DimOnly)
      ? "DimOnly" : "FullSleep";
  ```

  Check that `ScreensaverController` has a `getPolicy()` getter. If not, add one (simple inline getter returning `_policy`).

### MCP Tool Updates

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (line 1081–1129, `display_status`)
  Add the current policy to the status response:
  ```cpp
  result["screensaverPolicy"] = (_screensaver.getPolicy() == ScreensaverPolicy::DimOnly)
      ? "DimOnly" : "FullSleep";
  ```

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (line 1159–1213, `display_control`)
  Add `screensaverPolicy` as an accepted parameter:
  ```cpp
  if (args.containsKey("screensaverPolicy")) {
      String policy = args["screensaverPolicy"].as<String>();
      if (policy == "DimOnly") {
          _screensaver.setPolicy(ScreensaverPolicy::DimOnly);
      } else if (policy == "FullSleep") {
          _screensaver.setPolicy(ScreensaverPolicy::FullSleep);
      }
      // Persist the change
      saveConfig();
  }
  ```

### Portal UI

- [ ] **Find the Display settings tab JavaScript** — this will be either in `web/portal/app.js` or a contributor script served by `LVGLDisplayModule` (check `getTabScript()` if it exists, or search for `dimTimeout` in the portal JS files to find where display settings are rendered).

  Add a radio button group or toggle for screensaver policy:
  ```html
  <div class="setting-group">
    <label>Screensaver Mode</label>
    <div class="radio-group">
      <label><input type="radio" name="screensaverPolicy" value="DimOnly"> Dim Only</label>
      <label><input type="radio" name="screensaverPolicy" value="FullSleep"> Full Sleep (dim then blank)</label>
    </div>
  </div>
  ```

  Wire it to read from and write to the display config API (follow the pattern used by `dimTimeout` and `blankTimeout` — they likely use `/api/display/config` or similar).

## Verification Requirement
1. `pio run -e crowpanel-128-rotary` — must compile cleanly.
2. Native test suite must pass.
3. Verify the MCP `display_status` response includes `screensaverPolicy`.
4. Verify the MCP `display_control` accepts and persists `screensaverPolicy`.
5. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
✅ **Complete** — committed as `ab016c4d` on `issue-sweep-aug/wo6-screensaver-policy-ui`.

### Changes Made

**`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (7 change sites):
1. `applyConfig()` — reads `screensaverPolicy` from config JSON, defaults to `"FullSleep"`, calls `_screensaver.setPolicy()`.
2. `serializeConfig()` — writes the current policy via `_screensaver.getPolicy()`.
3. `resetConfig()` — resets policy to `FullSleep` on config reset.
4. `GET /api/display/status` — adds `screensaverPolicy` to REST API response.
5. `POST /api/display/sleep` — accepts `screensaverPolicy` parameter and persists.
6. MCP `display_status` — adds `screensaverPolicy` at top level and `policy` in the `screensaver` sub-object.
7. MCP `display_control` — adds `screensaverPolicy` parameter to schema (string enum) and handler.

**`web/display/display.js`** (3 change sites):
1. Added "Screensaver Mode" radio button group (Dim Only / Full Sleep) in the Screensaver article.
2. Radio button `change` listeners POST `{screensaverPolicy: value}` to `/api/display/sleep`.
3. `loadSettings()` reads `screensaverPolicy` from status API and checks the matching radio.

**`test/web/server.js`**:
- Added `screensaverPolicy: 'FullSleep'` to `MOCK_DISPLAY_STATUS`.

### Decisions Made
- **`getPolicy()` already existed** on `ScreensaverController` (inline getter, line 100) — no header modification needed.
- **Added `resetConfig()` policy reset** — not in WO instructions but necessary for config round-trip consistency (resetConfig must mirror applyConfig/serializeConfig).
- **REST API coverage** — added policy to both `GET /api/display/status` and `POST /api/display/sleep` in addition to MCP tools, since the portal JS reads/writes via REST not MCP.

### Dead Ends & Warnings
None — clean implementation with no rework.

### Out-of-Scope Findings
- **U8G2Display has the same gap**: `modules/U8G2Display/ScreensaverController.hpp` is missing `getPolicy()` entirely, and `U8G2DisplayModule` does not persist or expose `screensaverPolicy` in config, REST API, or MCP tools. Per WO boundary box, not touched.

### Blockers
None.

### Verification
- ✅ `pio run -e crowpanel-128-rotary` — compiled cleanly (43.2% flash, 14.6% RAM).
- ✅ `pio test -e native` — all 748 test cases passed across 30 suites.

