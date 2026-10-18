# PR-39 Branding — Context Bridge

## Strategic Summary
The primary objective of this project was to establish a unified system branding standard across both the U8G2 and LVGL display modules, eliminating hardcoded macros (`APP_TITLE`) and enabling downstream consumers to inject custom boot logos and application titles dynamically without modifying the framework source.

## Architectural Decisions (ADRs)

1. **Unified `BootBranding` Struct**: A new display-agnostic `BootBranding` struct was introduced to hold the application title. This is shared between both display modules, standardising the title injection API while allowing each module to use its native boot logo image format (`U8G2BootLogo` vs `lv_image_dsc_t`).
2. **Standardised System Screens Provider**: The U8G2 module was brought up to parity with LVGL's system screens provider pattern. We introduced `IU8G2SystemScreens` to mirror `ILVGLSystemScreens`, allowing consumers to override built-in framework status boards (Error, OTA, Configuring, WiFi) while maintaining consistent opt-in fallback logic.
3. **Constructor Injection**: Branding is injected via the display module's constructor. This enforces that the branding is known before `IBootReporter::beginEarly()` is called during Phase 1 boot, which is necessary since the boot splash renders before the LittleFS filesystem mounts (requiring compile-time C-arrays for logos).
4. **Macro Eradication**: The legacy `-DAPP_TITLE` build flag was removed from all `platformio.ini` configurations. The macro is only retained as a fallback inside the framework headers for backwards compatibility.

## Execution Trace
The project was executed in a 3-tier stacked workflow:
- **WO1**: Implemented `BootBranding` and `IU8G2SystemScreens`, updating the U8G2 module and `BootBoard`.
- **WO2**: Aligned the LVGL module to accept the shared `BootBranding` and removed the build flags.
- **WO3**: Wrote the `custom-branding.md` developer guide and added the native unit tests for the branding injection.

All tests passed successfully across all targets.
