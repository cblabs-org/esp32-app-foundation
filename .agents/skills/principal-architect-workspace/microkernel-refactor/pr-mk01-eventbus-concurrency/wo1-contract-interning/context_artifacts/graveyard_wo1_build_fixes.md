# Graveyard & Build Troubleshooting: WO1

## Issue: `esp32dev` missing `lvgl.h` header compilation failure

### Problem
When compiling for `esp32dev` environment via `build_project`, PlatformIO scanned all directories inside `modules/` including `modules/LVGLDisplay`. Because `LVGLDisplay` headers include `lvgl.h` (which is only declared in `lib_deps` for `[env:crowpanel-128-rotary]`), PlatformIO failed building `LVGLDisplay` for `esp32dev`.

### Diagnostic Log Trace
```
In file included from modules/LVGLDisplay/LVGLDisplayModule.hpp:52,
                 from modules/LVGLDisplay/LVGLDisplayModule.cpp:16:
include/interfaces/ILVGLHardwareAdapter.hpp:34:10: fatal error: lvgl.h: No such file or directory
```

### Resolution
Updated `platformio.ini` to add `lib_ignore = LVGLDisplay` under `[env:esp32dev]` and `[env:esp32s3nano]`. Hardware builds for non-LVGL display boards ignore the LVGL module, allowing `esp32dev` to build cleanly with 0 errors.
