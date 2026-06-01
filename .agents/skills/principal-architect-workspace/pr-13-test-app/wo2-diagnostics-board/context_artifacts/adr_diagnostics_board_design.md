# ADR: DiagnosticsBoard Design Decisions

## Decision 1: Forward-declare Application in Header
**Choice:** Forward-declare `class Application;` in the header, full `#include` only in `.cpp`.  
**Why:** Avoids pulling the massive Application dependency graph (EventBus, ConfigManager, WifiManager, etc.) into every translation unit that includes the board header. This is the same pattern used by `iBoardLayout.hpp` and keeps native testability clean.

## Decision 2: IDisplayBoard (not iBoardLayout)
**Choice:** Inherit directly from `IDisplayBoard` at `include/interfaces/IDisplayBoard.hpp`, not from `iBoardLayout`.  
**Why:** `iBoardLayout` is a layout-view layer (MVC pattern) with Application* injection in the constructor. `IDisplayBoard` is the direct carousel board interface that `U8g2DisplayModule::addBoard()` accepts. DiagnosticsBoard is a simple self-contained board, not a layout within a board-controller pattern.

## Decision 3: Separate setApplication() Instead of Constructor Injection
**Choice:** Application* is injected via `setApplication()` post-boot, not in the constructor.  
**Why:** The board is a static file-scope instance in `main.cpp`. It's constructed before `app.begin()` runs, so Application services aren't ready at construction time. The `addBoard()` call must happen before `display.begin()`, but Application data (hostname, WiFi) is only available after `app.begin()`. The two-phase pattern (`addBoard` → `begin` → `setApplication`) matches the framework's bootstrap design.

## Decision 4: Hostname Set Once in setApplication()
**Choice:** Row 1 (hostname) is set once in `setApplication()` and never refreshed in `tick()`.  
**Why:** The hostname is a static config value from `FrameworkConfig` — it doesn't change at runtime. Avoiding redundant `setText()` calls every second saves CPU cycles.

## Decision 5: Middle-dot Separator for WiFi Line
**Choice:** Used `\xB7` (·) between SSID and IP address.  
**Why:** The font is `u8g2_font_helvR08_tr` which is a "transparent" variant covering ASCII + extended Latin-1 (0x20–0xFF). `\xB7` (0xB7) is the middle-dot in ISO 8859-1, which is within the font's glyph range. It provides a clean visual separator without using multi-byte UTF-8 (which would require unicode-enabled fonts).

## Decision 6: All HW Code Guarded with NATIVE_BUILD
**Choice:** Every `WiFi.*`, `millis()`, `heap_caps_*`, and U8G2 render call is wrapped in `#ifndef NATIVE_BUILD`.  
**Why:** The native test environment has no Arduino/ESP-IDF runtime. The board must compile and link cleanly in native builds so existing unit tests aren't broken. In native builds, `tick()` and `render()` are effectively no-ops.
