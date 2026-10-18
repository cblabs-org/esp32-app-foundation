# ADR — WO4 Consumer App Migration, Custom-Board Guide, and Multi-Target Verification

## Context & Rationale

Prior to PR-51, hardware configuration settings (such as display pin assignments `-DDISPLAY_*_PIN` and display drivers `-DU8G2_DISPLAY_TYPE`) were scattered across `platformio.ini` environment definitions in both core framework targets and downstream consumer apps (`sample-app`, `template-app`).

Work Order 4 completed the hardware configuration migration program by:
1. Migrating consumer app environments (`sample-app` and `template-app`) to the `board.h` selector model (`-DBOARD_HEADER=\"../hardware/<id>/board.h\"`).
2. Pointing partition configurations at co-located partition CSV files (`../hardware/<id>/partitions.csv`).
3. Updating developer documentation to reflect the `board.h` model.
4. Authoring a comprehensive Custom Board Authoring Guide (`docs/developer/custom-boards.md`) for zero-edit downstream custom board integration (HC-11).
5. Performing end-to-end verification across all 8 PlatformIO build/test environments.

## Decisions Made

- **Single Source of Truth**: All display pins, bus routing, and driver types are sourced from `hardware/<id>/board.h`. Downstream `platformio.ini` files act purely as environment selectors.
- **Include Path Propagation**: Added `../hardware` and `../hardware/_common` to application include flags in consumer `platformio.ini` files to ensure seamless resolution of `board.h` and helper macros.
- **HC-11 Zero-Edit Custom Boards**: Downstream repos create `hardware/<myboard>/board.h` with mandatory ASCII pinout diagrams and allocation priority notes. No framework code modifications are required.
- **PlatformIO Board JSON vs. `board.h` Boundary**: Sourced toolchain/chip metadata (ESP32 vs ESP32-S3, PSRAM, Flash size) from PlatformIO board JSON (`board = ...`), while physical pinout and component availability are owned by `board.h`.

## Verification Results

Full-stack build and test validation succeeded across all targets:
- `native`: 358/358 GoogleTest test cases passed (including host null-board tests).
- `esp32dev` (framework): Build succeeded.
- `esp32s3nano` (framework): Build succeeded.
- `crowpanel-128-rotary` (framework): Build succeeded.
- `sample-app` `esp32dev`: Build succeeded.
- `sample-app` `esp32s3nano`: Build succeeded.
- `template-app` `esp32dev`: Build succeeded.
- `template-app` `esp32s3nano`: Build succeeded.
