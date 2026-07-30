# ADR: U8G2 Board Headers and Module Pin Migration (WO2)

## Context & Problem Statement
Prior to WO2, display hardware pins and driver types were defined as preprocessor flags in `platformio.ini` (`-DDISPLAY_CS_PIN=...`, `-DU8G2_DISPLAY_TYPE=...`). This created duplication, potential cross-file drift, and forced framework modules (`U8G2DisplayModule`) to depend directly on build flags rather than a clean hardware header interface.

## Decisions Made

1. **Dedicated Board Headers for U8G2 Targets (`hardware/esp32dev/board.h` & `hardware/esp32s3nano/board.h`):**
   - Authored single-source compile-time hardware configuration headers for `esp32dev` (ESP-WROOM-32 30-pin) and `esp32s3nano` (Waveshare ESP32-S3-Nano).
   - Implemented mandatory HC-1 criteria in both headers:
     - Physical ASCII-art pinout diagrams labeling all board pins and signals.
     - Structured Pin-Priority / Allocation Notes (Tier 1 clean GP, Tier 2 bus shared, Tier 3 strapping pins).
   - Defined display pin blocks inside `#ifdef WITH_DISPLAY_U8G2`:
     - Driver type: `#define U8G2_DISPLAY_TYPE U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI`
     - Pins (`PIN_DISPLAY_CS`, `PIN_DISPLAY_DC`, `PIN_DISPLAY_SCK`, `PIN_DISPLAY_MISO`, `PIN_DISPLAY_MOSI`, `PIN_DISPLAY_RST`) as `constexpr int8_t` values and preprocessor aliases.

2. **Module Refactoring (`U8G2DisplayModule`):**
   - Refactored `U8G2DisplayModule.hpp` and `.cpp` to `#include "board.h"`.
   - Replaced all legacy `-DDISPLAY_*_PIN` macro checks with `PIN_DISPLAY_*` `constexpr` checks.
   - Guarded module hardware instantiation with `#ifdef WITH_DISPLAY_U8G2` (replacing `#ifdef DISPLAY_CS_PIN`).
   - Maintained write-only SPI functionality (`PIN_DISPLAY_MISO` sentinel = `PIN_UNASSIGNED` / -1) and optional reset pin handling (`PIN_UNASSIGNED` mapped to `U8X8_PIN_NONE`).

3. **PlatformIO & Partition Co-location (HC-6, HC-10):**
   - Removed all `-DDISPLAY_*_PIN` and `-DU8G2_DISPLAY_TYPE` flags from `platformio.ini`.
   - Configured `-DBOARD_HEADER=\"hardware/<board_id>/board.h\"` for both targets.
   - Relocated root partition CSV files to `hardware/esp32dev/partitions.csv` and `hardware/esp32s3nano/partitions.csv`.
   - Added `-I .` to base build flags to ensure `#include BOARD_HEADER` resolves properly from root.

4. **Compile-Time Contract Validation:**
   - Ensured the contract check in `hardware/_common/board_contract.h` validates all required U8G2 symbols at compile time.

## Consequences & Verification
- `native`: 358/358 unit tests PASSED.
- `esp32dev`: Firmware compiled cleanly with `#pragma message("board: using hardware/esp32dev/board.h")`.
- `esp32s3nano`: Firmware compiled cleanly with `#pragma message("board: using hardware/esp32s3nano/board.h")`.
