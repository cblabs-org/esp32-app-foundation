# ADR: CrowPanel 1.28" Board Header Migration & Pin Mapping (WO3)

## Context
As part of PR-51 (Hardware Configuration), Work Order 3 migrates the Elecrow CrowPanel 1.28" HMI Rotary Display target (`crowpanel-128-rotary`) from scattered `platformio.ini` `-D` build flags to the single-source `board.h` header model.

## Decisions

1. **Single Source of Truth (`hardware/crowpanel-128-rotary/board.h`)**:
   - Authored board header containing mandatory ASCII-art pinout diagram and structured pin-priority / bus-grouping notes (enforced HC-1 criteria).
   - Identity: `BOARD_NAME` ("Elecrow CrowPanel 1.28 HMI Rotary"), `BOARD_ID_STRING` ("crowpanel-128-rotary"), `HAS_PSRAM = true`.

2. **RFC #50 Power-Enable Pins**:
   - Added `PIN_LCD_PWR_EN1 = 1` and `PIN_LCD_PWR_EN2 = 2` with prominent comment: `"must be driven HIGH at init — display will not light otherwise"`.
   - Per Strict Boundary Box rules, no functional initialization code was added to `CrowPanel128Adapter.cpp` to avoid blind runtime changes during a pin-migration WO.

3. **Touch Controller Hardware Verification Notes**:
   - Retained `PIN_TOUCH_INT = 0` (current wired value) while documenting MakerGuides verified GPIO `5` in comments.
   - Retained `PIN_TOUCH_RST = PIN_UNASSIGNED` (`-1`) while documenting MakerGuides GPIO `13` reset pin.

4. **Auxiliary I2C Bus Outside Display Guard (HC-12)**:
   - Placed `PIN_AUX_I2C_SDA = 38` and `PIN_AUX_I2C_SCL = 39` outside `#ifdef WITH_DISPLAY_LVGL` because onboard sensor I2C expansion is a board capability, not a display property.

5. **Adapter Refactoring**:
   - Refactored `adapters/lvgl/CrowPanel128Adapter.cpp` to `#include "board.h"` and use `PIN_LCD_*`, `PIN_TOUCH_*`, and `PIN_ENCODER_*` `constexpr` definitions.

6. **Dedicated Partitions CSV (HC-10)**:
   - Created `hardware/crowpanel-128-rotary/partitions.csv` and updated `board_build.partitions` in `platformio.ini`.

7. **Flag Elimination**:
   - Deleted all 13 `-DLCD/TOUCH/ENCODER_*` and `-DAUX_I2C_*` flags from `platformio.ini`. Added `-DBOARD_HEADER=\"hardware/crowpanel-128-rotary/board.h\"`.
