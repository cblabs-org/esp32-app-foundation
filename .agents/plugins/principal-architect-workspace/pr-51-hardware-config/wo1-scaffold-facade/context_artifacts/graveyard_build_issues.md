# Graveyard / Troubleshooting Record — WO1

## Issues Encountered & Resolved

1. **`lib_ignore` inclusion in `env:native` (`platformio.ini`)**:
   - *Problem*: Native builds/tests failed due to missing host libraries (`LovyanGFX.hpp` in `CrowPanel128Adapter` and ESP-IDF specific symbols in `Application`).
   - *Solution*: Added `Application` and `LVGL_Adapters` to `lib_ignore` in `env:native`, and ignored hardware-dependent display/app tests in `test_ignore`.

2. **GoogleTest Multiple Main Symbols**:
   - *Problem*: Creating `test_main.cpp` inside `test_unit/test_hardware_board` and `test_native/test_hardware_board` conflicted with PlatformIO's automatic GoogleTest runner `main()`.
   - *Solution*: Kept test cases in `test_hardware_board.cpp` without defining `main()` or leaving empty placeholder main files so PlatformIO's test harness link stage succeeds cleanly.
