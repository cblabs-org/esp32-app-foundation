# Project: SLS → LVGL Pro Migration

> **Branch prefix:** `sls-to-lvglpro`
> **Base branch:** `main`
> **Status:** WO1/4 — starting

## Objective

Replace SquareLine Studio with LVGL Pro Editor as the UI design tool for all
LVGL-based display environments. Introduce subjects-as-contract architecture
with `sys_*` prefix, design-folder-as-PIO-library, and eliminate the
`ILVGLSystemScreens` adapter shim.

## Work Order Ledger

- **[ ] [wo1-infrastructure](wo1-infrastructure/WORKORDER.md):** Rename design folder, rename project to "ui", add sys_ prefix to all subjects, create library.json, update platformio.ini, remove old SLS ui/ files
- **[ ] [wo2-module-integration](wo2-module-integration/WORKORDER.md):** Rewrite LVGLDisplayModule to use subjects directly, remove ILVGLSystemScreens dependency
- **[ ] [wo3-diagcarousel](wo3-diagcarousel/WORKORDER.md):** Rebind DiagCarouselController from SLS widget pointers to sys_diag_* subjects
- **[ ] [wo4-cleanup-docs](wo4-cleanup-docs/WORKORDER.md):** Delete SLS files, create subject contract docs, consumer migration guide

## Modules & Components

| Module | Status | Notes |
|---|---|---|
| `modules/LVGLDisplay/` | WO1-4 | Core module being migrated |
| `uidesign/crowpanel-128-rotary/lvglpro/` | WO1 | Design folder restructured |
| `modules/LVGLDisplay/screens/SLSSystemScreens.*` | WO4 | To be deleted |
| `modules/LVGLDisplay/DiagCarouselController.*` | WO3 | Rebind to subjects |
| `platformio.ini` | WO1 | lib_extra_dirs updated |

## Session Log

### Session 1 — 2026-09-01 — Initial decomposition
- **Approved by user:** 2026-09-01T07:37 — User approved v4 implementation plan
  and instructed PA to proceed with implementation.
- **Key decisions recorded:**
  - LVGL Pro over SLS (official tool, subjects, git-friendly XML)
  - Project name = "ui" for consistent API across form factors
  - `sys_` prefix on all 23 framework-contracted subjects
  - Design folder as PIO library (no copy script)
  - `ILVGLSystemScreens` eliminated — subjects replace adapter shim
  - Consumer overrides via `lib_extra_dirs` in platformio.ini
- **Starting WO1**
