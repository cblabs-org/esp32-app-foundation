# ADR: LVGL Branding Alignment & APP_TITLE Removal

## Status
Accepted

## Context
Prior to Issue #39, the framework used a hardcoded compile-time build flag `-DAPP_TITLE` in `platformio.ini` files and hardcoded CB Labs boot logos. Downstream applications could not easily brand their boot screens without modifying framework source or build flags. In WO1, the shared `BootBranding` struct and U8G2 `BootBoard` were updated. In WO2, `LVGLDisplayModule` and LVGL `BootScreen` are brought to parity with `BootBranding` and the legacy `-DAPP_TITLE` build flag is removed repository-wide.

## Decisions
1. **BootBranding Injection**:
   - `LVGLDisplayModule` accepts an optional `const BootBranding*` in its constructor (signature: `(adapter, branding, bootLogo, screens)`).
   - In `showBoot()`, `LVGLDisplayModule` passes `_branding ? _branding->appTitle : nullptr` to `_builtInBootScreen`.

2. **BootScreen Title Widget**:
   - Added `appTitle` property/constructor parameter/`init` argument/`setAppTitle` setter to `BootScreen`.
   - Created a persistent `_titleLabel` widget positioned below the logo (`LV_ALIGN_CENTER, 0, -10`) formatted with `lv_font_montserrat_14` in white, defaulting to `"ESP32 App Foundation"`.

3. **Build Flags Clean-up**:
   - Removed `-DAPP_TITLE=...` from all `platformio.ini` files (`env:base_esp32`, `env:native`, `sample-app`, `template-app`).
   - Added `#ifndef APP_TITLE` default fallback macro in `WebServerManager.hpp` to prevent breaking `/api/app` endpoint response when `appName` is unconfigured.

4. **Application Wiring**:
   - `sample-app`, `template-app`, and framework `src/main.cpp` instantiate a `static constexpr BootBranding` struct and pass `&branding` into display module constructors.

## Consequences
- Single unified contract for display branding across both U8G2 and LVGL modules.
- Downstream applications brand their boot screens simply by passing `&branding` at static instantiation.
- Removal of compile-time `-DAPP_TITLE` flags decouples framework binaries from hardcoded title macros.
