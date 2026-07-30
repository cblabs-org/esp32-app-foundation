# Graveyard: BOARD_HAS_PSRAM Preprocessor Macro Collision

## Problem Statement
During initial build verification of the `esp32s3nano` environment, the compiler failed with a preprocessor syntax error in `hardware/esp32s3nano/board.h`:

```
<command-line>: error: expected unqualified-id before numeric constant
./hardware/esp32s3nano/board.h:72:16: note: in expansion of macro 'BOARD_HAS_PSRAM'
   72 | constexpr bool BOARD_HAS_PSRAM = true;
      |                ^~~~~~~~~~~~~~~
```

## Root Cause Analysis
In `platformio.ini`, `[env:esp32s3nano]` specifies `-DBOARD_HAS_PSRAM` in `build_flags`. The GCC compiler defines `BOARD_HAS_PSRAM` as a preprocessor macro expanding to `1`.

When `hardware/esp32s3nano/board.h` contained:
```cpp
constexpr bool BOARD_HAS_PSRAM = true;
```
The preprocessor expanded this line to:
```cpp
constexpr bool 1 = true;
```
which is invalid C++ syntax.

## Resolution
To avoid macro collision while preserving the preprocessor flag required by `modules/Application/Application.cpp` (`#if defined(BOARD_HAS_PSRAM)`):

1. The C++ `constexpr` capability boolean was renamed to `HAS_PSRAM` (`constexpr bool HAS_PSRAM = true;`).
2. `board.h` explicitly defines `#define BOARD_HAS_PSRAM 1` if it is not already defined when PSRAM is available.
3. Updated `esp32dev`, `native`, and `_TEMPLATE` board headers to use `constexpr bool HAS_PSRAM = false;` for consistency across all targets.
