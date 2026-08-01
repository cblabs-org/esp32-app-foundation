# ADR: U8G2 Boot Branding & System Screen Customization Interface

## Context
Issue #39 standardizes boot screen branding and system screen customization across display modules (U8G2 and LVGL).
`BootBoard` and `U8G2DisplayModule` previously relied on hardcoded `APP_TITLE` macros and `cblabs_logo_*` definitions, making downstream custom branding impossible without modifying framework source code.

## Decisions

1. **Constructor Parameter Injection for Boot Logo & Title**:
   - `BootBoard` constructor updated to accept `const U8G2BootLogo* logo = nullptr` and `const char* appTitle = nullptr`.
   - `BootBoard` stores `_appTitle` and falls back to `APP_TITLE` macro when `_appTitle` is `nullptr`.
   - `BootBoard` checks `logo` and uses `logo->width`, `logo->height`, `logo->bits` when non-null, falling back to `cblabs_logo_width`, `cblabs_logo_height`, `cblabs_logo_bits` when `nullptr`.
   - `U8G2DisplayModule` constructor updated to accept `const BootBranding* branding = nullptr`, `const U8G2BootLogo* bootLogo = nullptr`, and `IU8G2SystemScreens* screens = nullptr`.
   - `U8G2DisplayModule` passes `bootLogo` and `branding ? branding->appTitle : nullptr` to `_bootBoard`.

2. **System Screens Override Precedence**:
   - `U8G2DisplayModule` stores `_screens` (pointer to `IU8G2SystemScreens`).
   - In `reportError()`, `_onOtaProgress()`, and `_onAppConfiguring()`, `_screens` is queried first (`_screens->errorBoard()`, `_screens->otaProgressBoard()`, `_screens->configuringBoard()`).
   - If `_screens` returns a non-null board pointer, that custom board is shown instead of the module's built-in system board.

## Consequences
- Downstream applications can pass custom branding descriptors (`BootBranding` and `U8G2BootLogo`) and custom system screen implementations (`IU8G2SystemScreens`) at module construction.
- Existing code and unit tests with default constructor parameters retain 100% backwards compatibility.
