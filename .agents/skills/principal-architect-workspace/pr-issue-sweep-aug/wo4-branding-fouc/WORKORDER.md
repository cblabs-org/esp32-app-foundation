---
name: issue-sweep-aug-wo4
description: Handoff instructions for Work Order 4 — LVGL boot title + portal branding FOUC
complexity: standard
---

# WO4 — LVGL Boot Title + Portal Branding FOUC

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix two branding inconsistencies: (1) the LVGL boot screen hardcodes "ESP32 App Foundation" ignoring `APP_TITLE` / `setAppName()`, and (2) the web portal flashes the framework's blue theme before the app's custom CSS loads. Closes **#129** and **#80**.

## Context

**Current branding surface status:**
| Surface | Resolves title? | Where |
|---|---|---|
| Web portal | ✅ `_appName ? _appName : APP_TITLE` | `WebServerManager.cpp:401` |
| U8G2 boot board | ✅ `_appTitle ? _appTitle : APP_TITLE` | `BootBoard.cpp:86` |
| **LVGL boot screen** | ❌ Hardcoded SLS literal | `ui_uiBootScreen.c:52` |

**Portal FOUC sequence:** `loadAppInfo()` (app.js:531) is called at line 626 — *after* `/api/tabs` fetch and script loading. The title and CSS flash framework defaults before the app's branding applies.

## Strict Boundary Box
- **ONLY** modify the files listed below.
- Do NOT modify the SquareLine Studio `.spj` file or regenerate UI C files (the fix is runtime, not SLS).
- Do NOT change the tab loading mechanism or `/api/tabs` endpoint.
- Do NOT touch U8G2Display (it already works correctly).

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (rule `04-mcp-tooling.md`).
- Build verification: `build_project` targeting `crowpanel-128-rotary`.
- Native tests: `run_tests` with environment `native`.

## Line-Item Instructions

### #129 — LVGL Boot Screen Title

- [ ] **`modules/LVGLDisplay/ILVGLSystemScreens.hpp`** (around line 56)
  Add a new virtual method to the interface:
  ```cpp
  /**
   * Set the application title displayed on the boot screen.
   * Called by the framework after Application::getAppName() is available.
   * @param title The application title string (lifetime managed by caller).
   */
  virtual void setAppTitle(const char* title) {}  // Default no-op for backward compat
  ```

- [ ] **`modules/LVGLDisplay/screens/SLSSystemScreens.hpp`**
  Override `setAppTitle()`:
  ```cpp
  void setAppTitle(const char* title) override;
  ```

- [ ] **`modules/LVGLDisplay/screens/SLSSystemScreens.cpp`** (after line 34)
  Implement `setAppTitle()` — mirror the U8G2 `BootBoard` pattern:
  ```cpp
  void SLSSystemScreens::setAppTitle(const char* title) {
      if (ui_uiBootTitle != nullptr && title != nullptr) {
          lv_label_set_text(ui_uiBootTitle, title);
      }
  }
  ```

- [ ] **`modules/LVGLDisplay/LVGLDisplayModule.cpp`** (in `begin(Application&)` or `reportStatus`)
  After the system screens are initialised, call `setAppTitle()` with the app name:
  ```cpp
  _systemScreens->setAppTitle(app.getAppName());
  ```
  Find the right insertion point — it should be after `_systemScreens` is assigned but before/during the boot screen display. Look at `reportStatus()` (line 436–465) for context.

### #80 — Portal Branding FOUC

- [ ] **`modules/Application/Application.hpp`** (near line 153)
  Add a new method for app stylesheet:
  ```cpp
  void setAppStylesheet(const char* path) { _appStylesheet = path; }
  const char* getAppStylesheet() const { return _appStylesheet; }
  ```
  Add the member variable near the other app metadata fields:
  ```cpp
  const char* _appStylesheet = nullptr;  ///< Optional CSS path for portal branding
  ```

- [ ] **`modules/WebServer/WebServerManager.cpp`** (lines 401–409, `/api/app` route)
  Extend the `/api/app` JSON response to include the optional stylesheet:
  ```cpp
  // After line 405 (doc["version"] = BUILD_VERSION;):
  if (_appStylesheet) {
      doc["stylesheet"] = _appStylesheet;
  }
  ```
  The stylesheet value needs to come from `Application`. Check how `_appName` is passed through — `WebServerManager` likely stores a copy set via a setter or passed during `begin()`. Follow the same pattern for `_appStylesheet`.

- [ ] **`web/portal/app.js`** (lines 531–543 and 616–636)
  Restructure the boot sequence so `/api/app` is fetched **before** `/api/tabs`:

  ```javascript
  // NEW boot sequence:
  // 1. Fetch app branding FIRST
  loadAppInfo().then(function() {
      // 2. THEN fetch tabs and load scripts
      return window.apiFetch('/api/tabs');
  }).then(function(tabs) {
      // ... existing tab loading logic from line 616+ ...
  });
  ```

  Extend `loadAppInfo()` to also inject the stylesheet:
  ```javascript
  async function loadAppInfo() {
      try {
          var data = await window.apiFetch('/api/app');
          if (data && data.name) {
              portalTitle.textContent = data.name;
              document.title = data.name;
              var loginTitle = document.getElementById('login-title');
              if (loginTitle) loginTitle.textContent = data.name;
          }
          // Inject app stylesheet if provided
          if (data && data.stylesheet) {
              var link = document.createElement('link');
              link.rel = 'stylesheet';
              link.href = data.stylesheet;
              document.head.appendChild(link);
          }
      } catch (err) {
          console.warn('Failed to fetch /api/app');
      }
  }
  ```

## Verification Requirement
1. `pio run -e crowpanel-128-rotary` — must compile cleanly.
2. Native test suite must pass.
3. Verify that `loadAppInfo()` now runs before tab scripts in the JS boot sequence.
4. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
_(pending)_

### Changes Made
_(pending)_

### Decisions Made
_(pending)_

### Dead Ends & Warnings
_(pending)_

### Out-of-Scope Findings
_(pending)_

### Blockers
_(pending)_
