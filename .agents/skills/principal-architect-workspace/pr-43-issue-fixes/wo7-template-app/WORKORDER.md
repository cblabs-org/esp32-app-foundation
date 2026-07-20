---
name: pr-43-wo7
description: Update template-app for WO3 and WO5 interface additions
complexity: standard
---

# WO7 — Update Template App

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Update `template-app/src/MyApp.hpp` and `MyApp.cpp` to demonstrate the new `IWebContributor` methods added in WO3 (`getTabScript()`) and WO5 (`getApiPrefix()`, `getMaxPostBody()`). The template app is the starting point for new downstream projects, so it must document all available interface methods.

## Strict Boundary Box

- **DO NOT** modify any framework source files — they were updated in WO1-WO6.
- **DO NOT** modify the sample-app — that's WO8.
- **DO NOT** modify any test files.
- **DO NOT** change any core logic in MyApp — only add the new interface overrides.

## Critical Design Constraints

1. **The template app does NOT serve a tab script.** It uses inline HTML/JS. So `getTabScript()` should be listed in the "Optional overrides" comment block but NOT implemented (i.e., keep the default `nullptr` return).
2. **`getApiPrefix()` and `getMaxPostBody()` SHOULD be overridden** to demonstrate the pattern — even if the template uses the default 4096 limit. Showing the override teaches downstream developers the mechanism exists.
3. **Follow existing house style** — match the Doxygen docstring pattern used by the other overrides in the file.

## Checklist

### `template-app/src/MyApp.hpp`

- [ ] **Lines 119-121** — Expand the "Optional overrides" comment block to include the WO3 and WO5 methods:
  ```cpp
  // Optional overrides — uncomment to customize:
  // const char* getParentTab() const override { return "System"; }
  // const char* getTabIcon() const override { return "⚙️"; }
  // const char* getHelpText() const override { return "Help text for this tab"; }
  // const char* getTabScript() const override { return "/myapp.js"; }

  /** @brief Returns the API path prefix for body-size middleware matching.
   *  @return API prefix string.
   */
  const char* getApiPrefix() const override { return "/api/myapp"; }

  /** @brief Returns the maximum POST/PUT body size for this module's routes.
   *  @return Maximum body size in bytes (ADR-15 default: 4096).
   */
  size_t getMaxPostBody() const override { return 4096; }
  ```

- [ ] **IWebContributor docstring block (lines 28-33)** — Add the new methods to the interface overview comment:
  ```
   *   IWebContributor:
   *     registerRoutes()  — REST endpoints for configuration
   *     getTabName()      — Portal tab display name
   *     getTabOrder()     — Tab sort position
   *     getApiPrefix()    — API path prefix for body-size middleware [NEW]
   *     getMaxPostBody()  — Max POST/PUT body size (default: 4096) [NEW]
   ```

### `template-app/src/MyApp.cpp`

- [ ] **IWebContributor section header comment (~line 129)** — Update to mention the new methods are available.

## Verification

- [ ] PlatformIO build passes for `template-app` (native environment).
- [ ] Code review: confirm `getTabScript()` is commented-out (optional), `getApiPrefix()` and `getMaxPostBody()` are active overrides.
- [ ] Code review: confirm house style compliance (Doxygen `@brief`/`@return` format).
