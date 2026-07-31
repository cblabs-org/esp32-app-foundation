---
name: input-module-wo4
description: Handoff instructions for Work Order 4 — mechanical rename Input→Button + disablePull fix
complexity: mechanical
---

# Work Order 4: Rename Input→Button + disablePull

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Perform a full mechanical rename of all `Input*` identifiers to `Button*` across
the codebase, rename EventBus topics from `input.*` to `button.*`, add a
`disablePull` field to the GPIO config struct, and add a BOOT button doc note to
the template. This addresses PR #53 review findings.

## Strict Boundary Box

- **DO** create/modify/delete ONLY the files listed in the checklist below.
- **DO NOT** change any architectural decisions or logic — this is a pure rename.
- **DO NOT** file issues or make architectural decisions — surface findings to the PA.

## Tooling Constraints

- Use PlatformIO MCP server for build verification.
- Verify: `pio run -e native -t test` (all tests pass)
- Verify: `pio run -e esp32dev` and `pio run -e esp32s3nano` compile
- Do NOT flash hardware.

## Checklist

### 1. Rename `include/InputConfig.h` → `include/ButtonConfig.h`

- Create `include/ButtonConfig.h` with the SAME content as `InputConfig.h` but with ALL type names renamed:
  - `InputType` → `ButtonType`
  - `InputGesture` → `ButtonGesture`
  - `InputGpioConfig` → `ButtonGpioConfig`
  - `InputAdcConfig` → `ButtonAdcConfig`
  - `InputMatrixConfig` → `ButtonMatrixConfig`
  - `BoardInput` → `BoardButton`
  - `InputEvent` → `ButtonEvent`
- **Add `disablePull` field** to `ButtonGpioConfig`:
  ```cpp
  struct ButtonGpioConfig {
      int8_t   pin;
      bool     activeLow;
      bool     disablePull;   // true for externally-driven inputs (e.g. TTP223 touch)
      bool     powerSave;
  };
  ```
- Update the file header comment (module path, description, exported symbols).
- Delete `include/InputConfig.h`.

### 2. Rename `modules/InputModule/` → `modules/ButtonModule/`

Create `modules/ButtonModule/ButtonModule.hpp`:
- Copy from `InputModule.hpp`, rename:
  - Class: `InputModule` → `ButtonModule`
  - Include: `"InputConfig.h"` → `"ButtonConfig.h"`
  - All type references: `BoardInput` → `BoardButton`, `InputEvent` → `ButtonEvent`, `InputGesture` → `ButtonGesture`, etc.
  - Header guard / pragma once is fine
  - Update file header comment

Create `modules/ButtonModule/ButtonModule.cpp`:
- Copy from `InputModule.cpp`, rename:
  - Class: `InputModule` → `ButtonModule`
  - Include: `"InputModule.hpp"` → `"ButtonModule.hpp"`
  - All type references renamed
  - **EventBus topics renamed:**
    - `"input.click"` → `"button.click"`
    - `"input.double_click"` → `"button.double_click"`
    - `"input.long_press"` → `"button.long_press"`
    - `"input.press_down"` → `"button.press_down"`
    - `"input.unknown"` → `"button.unknown"`
  - **MCP tool renamed:** `"input_status"` → `"button_status"`
  - **Plumb `disablePull`** in `createGpioButton()`:
    ```cpp
    .disable_pull = input.gpio.disablePull
    ```
    (replaces the hardcoded `false`)

Create `modules/ButtonModule/library.json`:
- Copy from `InputModule/library.json`, rename the library name from `InputModule` to `ButtonModule`.

Delete the entire `modules/InputModule/` directory.

### 3. Update `hardware/_common/board_contract.h`

- Change `#ifdef WITH_INPUT` → `#ifdef WITH_BUTTONS`
- Change `BOARD_INPUT_COUNT` → `BOARD_BUTTON_COUNT` in the static_assert
- Update the assert message text

### 4. Update `hardware/esp32dev/board.h`

- `#ifdef WITH_INPUT` → `#ifdef WITH_BUTTONS`
- `#include "InputConfig.h"` → `#include "ButtonConfig.h"`
- `BoardInput` → `BoardButton`
- `InputType::GPIO` → `ButtonType::GPIO`
- `BOARD_INPUTS[]` → `BOARD_BUTTONS[]`
- `BOARD_INPUT_COUNT` → `BOARD_BUTTON_COUNT`

### 5. Update `hardware/esp32s3nano/board.h`

Same changes as esp32dev.

### 6. Update `hardware/native/board.h`

- `#ifdef WITH_INPUT` → `#ifdef WITH_BUTTONS`
- `#include "InputConfig.h"` → `#include "ButtonConfig.h"`
- `BoardInput` → `BoardButton`
- `BOARD_INPUTS[]` → `BOARD_BUTTONS[]`
- `BOARD_INPUT_COUNT` → `BOARD_BUTTON_COUNT`

### 7. Update `hardware/_TEMPLATE.board.h`

- Rename all `Input*` references to `Button*` in the WITH_INPUT example section
- Change `WITH_INPUT` → `WITH_BUTTONS` throughout
- Add BOOT button reuse note:
  ```cpp
  // BOOT button reuse (pin-constrained boards):
  //   The BOOT button (typically GPIO 0) is safe to use as a user input
  //   post-boot. Strapping state only matters at reset. If the user holds
  //   the button during a software reboot, the chip enters download mode
  //   — this is a useful escape hatch, not a defect.
  ```

### 8. Update `sample-app/src/main.cpp`

- `#include "InputModule.hpp"` → `#include "ButtonModule.hpp"`
- `#ifdef WITH_INPUT` → `#ifdef WITH_BUTTONS`
- `InputModule inputModule` → `ButtonModule buttonModule`
- `BOARD_INPUT_COUNT` → `BOARD_BUTTON_COUNT`
- `BOARD_INPUTS` → `BOARD_BUTTONS`
- `inputModule.addInput(...)` → `buttonModule.addInput(...)` (method name stays `addInput` — wait, check the header. The method should stay the same or also be renamed. Check `ButtonModule.hpp` for the actual method name. If it's still `addInput`, rename it to `addButton`.)
- EventBus topic: `"input.click"` → `"button.click"`
- All other `inputModule` references → `buttonModule`

### 9. Update `sample-app/platformio.ini`

- `-D WITH_INPUT` → `-D WITH_BUTTONS` in all hardware environments
- `InputModule` → `ButtonModule` in any `lib_deps` references

### 10. Rename test file

Create `test/test_unit/test_buttonModule/test_ButtonModule.cpp`:
- Copy from `test_inputModule/test_InputModule.cpp`
- Rename all `Input*` → `Button*` type references
- Rename `InputModule` → `ButtonModule`
- Rename `"input.click"` etc → `"button.click"` etc in topic assertions
- Rename `"input_status"` → `"button_status"` in MCP tool tests
- Update file header

Delete `test/test_unit/test_inputModule/` directory.

### 11. Also rename `addInput()` → `addButton()` method

In `ButtonModule.hpp` and `ButtonModule.cpp`, rename:
- `void addInput(const BoardButton& input)` → `void addButton(const BoardButton& button)`
- `_pendingInputs` → `_pendingButtons`
- Update all references in the .cpp and tests

## Verification

1. `pio run -e native -t test` — all 368 tests pass
2. `pio run -e esp32dev` — compiles with `-D WITH_BUTTONS`
3. `pio run -e esp32s3nano` — compiles with `-D WITH_BUTTONS`
4. Grep for any remaining `InputModule`, `InputConfig`, `WITH_INPUT`, `BOARD_INPUTS`, `input.click` — should find zero hits outside of `.agents/` and stubs.

## When Finished

Send a message back to the Principal Architect reporting:
- Which files were created/modified/deleted
- Whether all builds and tests pass
- Grep results confirming no stale references remain
- Any issues or findings that need PA attention
