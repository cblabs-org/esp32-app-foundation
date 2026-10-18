# Architecture Decision: Multi-Target Display Flags Bridge

Bridged the `HAS_LVGL_DISPLAY` and `HAS_DISPLAY` flags to `BOARD_DISPLAY_KIND` inside `template-app/src/main.cpp` to properly conditionally instantiate `LVGLDisplayModule` or `U8G2DisplayModule`. 

The hardware-config refactoring will provide `BOARD_DISPLAY_KIND` natively in the future, but until then this macro bridging ensures that both `esp32dev` (U8G2) and `crowpanel-128-rotary` (LVGL) compile cleanly in parallel via pio-manager without conflicts.
