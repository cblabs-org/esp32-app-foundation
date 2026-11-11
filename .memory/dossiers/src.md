# src

## Invariants & contracts

### [docseed-74613b41] T2 has **no root `src/main.cpp`**, meaning the framework cannot be compiled standalone from root. The entry point exists only in `template/src/main.cpp`. This fundamentally changes the project topology from a buildable library+app to a library-only layout. The spec explicitly requires a root entry point per the Hollywood Principle.
`invariant · kind: semantic · anchors: src/main.cpp · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

### [docseed-e927bbc4] **Create root `src/main.cpp`** — framework must be standalone-buildable
`invariant · kind: semantic · anchors: src/main.cpp · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

## Decisions

### [seed-533c2c54] Guarded `ui.h` and `ui_gen.h` with `LV_LVGL_H_INCLUDE_SIMPLE` and configured it in `library.json` so LVGL Pro headers resolve `lvgl.h` properly within the PlatformIO build system.
`decision · kind: episodic · anchors: src/main.cpp, modules/LVGLDisplay/library.json`

### [seed-7f266318] Defined weak event callback hooks (`ui_boot_fade_overlay_loaded`, `ui_system_dismiss_clicked`) directly in `LVGLDisplayModule.cpp` to resolve linker dependencies when compiling without `LV_EDITOR_PREVIEW`.
`decision · kind: episodic · anchors: src/main.cpp, modules/LVGLDisplay/README.md, test/stubs/platform/lvgl.h, test/stubs/platform/ui.h, test/stubs/platform/ui_gen.h, test/test_unit/test_lvglDisplay/ui_stubs.cpp, test/test_unit/test_lvglDisplay/test_screens.cpp, test/test_unit/test_lvgl_stubs/test_lvgl_stubs.cpp`

### [seed-a2dbc586] Created platform stubs `ui.h` and `ui_gen.h` in `test/stubs/platform/` to allow full native unit testing of `LVGLDisplayModule` and `DiagCarouselController` without requiring hardware-specific `UIDesign` libraries.
`decision · kind: episodic · anchors: src/main.cpp, modules/LVGLDisplay/README.md, test/stubs/platform/lvgl.h, test/stubs/platform/ui.h, test/stubs/platform/ui_gen.h, test/test_unit/test_lvglDisplay/ui_stubs.cpp, test/test_unit/test_lvglDisplay/test_screens.cpp, test/test_unit/test_lvgl_stubs/test_lvgl_stubs.cpp`

### [seed-cc6f2fa9] Provided a legacy delegating constructor in `LVGLDisplayModule.hpp` so that existing caller sites (`src/main.cpp`) remain ABI/API compatible without modifying files outside the WO2 boundary box, until WO4 removes `SLSSystemScreens`.
`decision · kind: episodic · anchors: src/main.cpp, modules/LVGLDisplay/library.json`

## Open issues

### [seed-5e22fdee] `DiagCarouselController.cpp:15` compilation error (`ui/ui.h: No such file or directory`) was observed during verification and is scoped for resolution in WO3.
`open-issue · kind: semantic · anchors: src/main.cpp, modules/LVGLDisplay/library.json`

### [seed-878f4adf] `uidesign/crowpanel-128-rotary/lvglpro/` generated C files need `-DLV_LVGL_H_INCLUDE_SIMPLE` defined during standalone compilation of the `UIDesign` library.
`open-issue · kind: semantic · anchors: src/main.cpp, modules/LVGLDisplay/library.json`
