# lib

## Invariants & contracts

### [docseed-d5798417] Make OTA verification **fail closed** (High — small, localized change in `OtaDownloadEngine` + `otaUpdateManager`).
`invariant · kind: semantic · anchors: lib/OtaDownloadEngine/OtaDownloadEngine.hpp · source_doc: docs/archive/embedded-best-practices-assessment.md · evidence: doc:docs/archive/embedded-best-practices-assessment.md`

## Decisions

### [seed-4c3e5026] Used `#ifndef UNIT_TEST` stubs for native environment, removing the need to add `DeepStackExecutor` to `lib_ignore`.
`decision · kind: episodic · anchors: lib/DeepStackExecutor, library.json, test/test_unit/test_deepStackExecutor/test_deepStackExecutor.cpp`

### [seed-73efc4ea] Defined `LV_SYMBOL_OK`, `LV_SYMBOL_CLOSE`, and `LV_SYMBOL_WARNING` in `test/stubs/platform/lvgl.h` to ensure all LVGL display tests build and pass on native runner.
`decision · kind: episodic · anchors: lib/PngWriter/PngWriter.cpp, test/test_unit/test_pngWriter/test_pngWriter.cpp, test/stubs/platform/lvgl.h, test/test_unit/test_mcpServer`

### [seed-8bfdb43a] Guarded `tdefl` functions in `PngWriter.cpp` using `#if defined(ESP_PLATFORM)` so `writeRGB565ToRGB888` falls back directly to `writeRGB565Stored` on native host builds without needing mock ROM compression headers.
`decision · kind: episodic · anchors: lib/PngWriter/PngWriter.cpp, test/test_unit/test_pngWriter/test_pngWriter.cpp, test/stubs/platform/lvgl.h, test/test_unit/test_mcpServer`
