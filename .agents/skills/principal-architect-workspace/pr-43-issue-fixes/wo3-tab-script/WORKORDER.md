---
name: pr-43-wo3
description: Tab script contribution — IWebContributor + WebServerManager + app.js boot restructure
complexity: complex
---

# WO3 — Tab Script Contribution

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add `getTabScript()` to `IWebContributor`, emit it in the `/api/tabs` response, restructure the `app.js` boot sequence to load declared scripts, and migrate the sample app onto the new mechanism. Closes GitHub Issue #20.

## Strict Boundary Box

- **DO NOT** modify any files outside the checklist below.
- **DO NOT** change the tab navigation structure, pill bar logic, or tab-changed event contract.
- **DO NOT** change the authentication middleware or auth bypass list.
- **DO NOT** add new npm dependencies or build tools.
- **DO NOT** rename existing routes.

## Critical Design Constraints

These were agreed with the downstream developer and must be followed exactly:

1. **Singular `getTabScript()`** — returns one `const char*` per contributor. No arrays, no comma-separated lists.
2. **Same-origin guard** — the path MUST start with `/`. Paths not starting with `/` are silently skipped in WebServerManager.
3. **Boot-order restructure** — the naive "fetch tabs after scripts" races the first `tab-changed`. The new boot must:
   - Fetch `/api/tabs` **once** (cache the response)
   - Extract declared `script` URLs from the response
   - Load all scripts (built-in + declared) via `loadScripts()`
   - Pass the cached tabs into `loadTabs()` (no second fetch)
   - Then build nav and dispatch `tab-changed`
4. **Error isolation** — a broken declared script degrades to an empty tab, never bricks the portal. Use the existing `script.onerror → warn and continue` pattern.

## Checklist

### C++ Interface

- [ ] **`include/interfaces/IWebContributor.hpp`** — After `getTabIcon()` (line 104), add:
  ```cpp
  /**
   * @brief Return the route path for this tab's JavaScript file.
   *
   * Return nullptr if this contributor has no tab script (default).
   * When non-null, the path is included in the /api/tabs response
   * as the "script" field and loaded dynamically by the SPA.
   *
   * The path MUST start with '/' (same-origin only). Paths not
   * starting with '/' are silently ignored by WebServerManager.
   *
   * NOTE: Tab scripts are served unauthenticated (same as all static
   * assets). Do not embed sensitive data — fetch it from authenticated
   * /api/ endpoints instead.
   *
   * @return Route path string (e.g. "/weather.js"), or nullptr.
   */
  virtual const char* getTabScript() const { return nullptr; }
  ```
- [ ] **`include/interfaces/IWebContributor.hpp` header comment (lines 14-27):** Add `getTabScript()` to the exported interface documentation list.

### C++ Server

- [ ] **`modules/WebServer/WebServerManager.cpp:274-288`** — Inside the `/api/tabs` builder loop, after the `icon` emission block (after line 287), add script emission:
  ```cpp
  const char* script = contributor->getTabScript();
  if (script != nullptr && script[0] == '/') {
      tabObj["script"] = script;
  }
  ```

### JavaScript SPA

- [ ] **`web/portal/app.js` — Restructure boot sequence (lines 488-493).** Replace:
  ```js
  loadScripts(function() {
    loadTabs();
    loadAppInfo();
    checkApMode();
  });
  ```
  With a new boot flow that:
  1. Fetches `/api/tabs` once using `window.apiFetch('/api/tabs')`
  2. Extracts `script` fields from the response into an array
  3. Appends them to the built-in scripts list
  4. Calls `loadScripts()` with the combined list
  5. In the callback, calls `loadTabs()` passing the cached tabs (no re-fetch)
  6. Continues with `loadAppInfo()` and `checkApMode()` as before

- [ ] **`web/portal/app.js:165` — Modify `loadTabs()`** to accept an optional pre-fetched tabs parameter:
  ```js
  async function loadTabs(prefetchedTabs) {
    try {
      tabHierarchy = prefetchedTabs || await window.apiFetch('/api/tabs');
    } catch (err) {
      ...
    }
    // rest unchanged
  }
  ```

- [ ] **`web/portal/app.js:444-460` — Remove `'weather.js'`** from the hardcoded scripts array. It will now be loaded dynamically via the `getTabScript()` mechanism.

### Sample App Migration

- [ ] **`sample-app/src/WeatherModule.hpp`** — After the `getTabName()`/`getTabOrder()` overrides (~line 367), add:
  ```cpp
  const char* getTabScript() const override { return "/weather.js"; }
  ```

## Verification

- [ ] PlatformIO build passes (IWebContributor.hpp, WebServerManager.cpp, WeatherModule.hpp compile).
- [ ] Manual review: confirm `weather.js` is no longer in the hardcoded scripts array.
- [ ] Manual review: confirm the boot sequence fetches `/api/tabs` exactly once.
- [ ] Manual review: confirm error isolation — `script.onerror` warns and continues for declared scripts.
