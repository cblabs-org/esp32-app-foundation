# Graveyard: Build Issues & Diagnostic Findings (WO3)

## Issue: `src/main.cpp` Compilation Error under `WITH_DISPLAY_LVGL`

### Error Trace
```text
src/main.cpp: In function 'void setup()':
src/main.cpp:147:44: error: 'class LVGLDisplayModule' has no member named 'getDisplayInfo'; did you mean 'getDisplay'?
  147 |     mcpServer.setDisplayInfo(displayModule.getDisplayInfo());
```

### Cause
In `src/main.cpp`, `mcpServer.setDisplayInfo(displayModule.getDisplayInfo())` was positioned under `#if defined(WITH_DISPLAY_U8G2) || defined(WITH_DISPLAY_LVGL)`. `U8G2DisplayModule` exposes `getDisplayInfo()` for `DiagnosticsBoard`, whereas `LVGLDisplayModule` registers tools via `IMCPContributor` interface and does not have a `getDisplayInfo()` method.

### Resolution
Scoped `mcpServer.setDisplayInfo(displayModule.getDisplayInfo())` inside `#ifdef WITH_DISPLAY_U8G2` in `src/main.cpp`.
