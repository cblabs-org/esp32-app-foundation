---
name: pr-43-wo8
description: Update sample-app (WeatherModule) for WO3 and WO5 interface additions
complexity: standard
---

# WO8 — Update Sample App

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Update `sample-app/src/WeatherModule.hpp` to add the WO3 (`getTabScript()`) and WO5 (`getApiPrefix()`, `getMaxPostBody()`) overrides. The sample app is a real, functional reference implementation — it must demonstrate the correct usage of all new interface methods.

## Strict Boundary Box

- **DO NOT** modify any framework source files — they were updated in WO1-WO6.
- **DO NOT** modify the template-app — that's WO7.
- **DO NOT** modify any test files.
- **DO NOT** change WeatherModule's core logic, config, data-source, or display code.

## Critical Design Constraints

1. **The sample app DOES serve a tab script** (`/weather.js`). In WO3, `weather.js` was removed from the hardcoded scripts array in `app.js` and is now loaded dynamically via `getTabScript()`. This WO must add the override to WeatherModule so the script continues to be loaded.
2. **`getApiPrefix()` should return `"/api/weather"`** — matching the routes registered in `registerRoutes()`.
3. **`getMaxPostBody()` should return `4096`** — the weather config endpoint only receives small JSON payloads. The default is sufficient, but showing the override explicitly documents the choice.
4. **Follow existing house style** — match the Doxygen docstring pattern used by the adjacent overrides (see `getTabName()`, `getTabIcon()` at lines 366-376).

## Checklist

### `sample-app/src/WeatherModule.hpp`

- [ ] **After `getTabIcon()` override (~line 376):** Add the three new overrides:
  ```cpp
  /** @brief Returns the route path for the Weather tab's JavaScript file. */
  const char* getTabScript() const override { return "/weather.js"; }

  /** @brief Returns the API path prefix for body-size middleware matching. */
  const char* getApiPrefix() const override { return "/api/weather"; }

  /** @brief Returns the maximum POST/PUT body size for weather routes (ADR-15 default). */
  size_t getMaxPostBody() const override { return 4096; }
  ```

- [ ] **IWebContributor docstring block (~lines 55-62)** — Add the new methods to the interface overview comment:
  ```
   *   IWebContributor:
   *     registerRoutes()  — REST API + /weather.js serving
   *     getTabName()      — "Weather" portal tab
   *     getParentTab()    — nullptr (top-level)
   *     getTabOrder()     — 50 (mid-priority)
   *     getTabIcon()      — Cloud SVG icon
   *     getTabScript()    — "/weather.js" dynamic script loading [NEW]
   *     getApiPrefix()    — "/api/weather" for body-size middleware [NEW]
   *     getMaxPostBody()  — 4096 (ADR-15 default) [NEW]
   ```

### `sample-app/src/WeatherModule.cpp`

- [ ] **No changes required** — the overrides are inline in the header.

## Verification

- [ ] PlatformIO build passes for `sample-app` (native environment).
- [ ] Code review: confirm `getTabScript()` returns `"/weather.js"` — this is CRITICAL for the tab to work post-WO3.
- [ ] Code review: confirm `getApiPrefix()` matches the actual routes registered in `registerRoutes()`.
- [ ] Code review: confirm house style compliance (Doxygen `@brief` format, consistent placement).
