# Architecture Decision Record: WO3 Docs & Tests

## Context
Work Order 3 finalized the Custom Branding feature (PR 39) by providing comprehensive developer documentation and validating the implementations from WO1 and WO2 via unit tests.

## Decisions & Outcomes
- **Documentation**: Developed `docs/developer/custom-branding.md`. Decided to explicitly separate the branding injection (`BootBranding` and `U8G2BootLogo` / `lv_image_dsc_t`) from the custom system screen provider interface (`IU8G2SystemScreens` / `ILVGLSystemScreens`).
- **Timing Constraints**: Explicitly documented the Phase 1 boot timing constraint, enforcing that boot splash logos must reside in flash memory as compile-time C-arrays because they are rendered prior to filesystem mounting.
- **Unit Testing**: Validated `BootBoard` rendering logic and fallback mechanisms to ensure default dimensions/titles are used if none are supplied. Validated `U8G2DisplayModule` properly delegates to custom screen providers and correctly instantiates built-in screens if the provider is null or returns null.
- **Verification**: Tests executed successfully via the GoogleTest native runner (`pio test -e native`), passing 368/368 assertions.
