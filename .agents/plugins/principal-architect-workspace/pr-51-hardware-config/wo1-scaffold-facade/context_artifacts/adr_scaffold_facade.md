# Architecture Decision Record — WO1 Scaffold, Facade, Contract, Null Board

## Context & Rationale
WO1 stood up the core `board.h` pin-map infrastructure for PR-51 without migrating any real board's pin definitions.

## Key Decisions Made
1. **Shared Primitives (`hardware/_common/board_pins.h`)**:
   - Defined `constexpr int8_t PIN_UNASSIGNED = -1;` sentinel.
   - Introduced `PIN_IS_VALID(pin)` macro helper for clean checking.

2. **Hardware Facade (`hardware/board.h`)**:
   - Resolves `-DBOARD_HEADER` selector flag.
   - Includes `board_contract.h` and outputs `#pragma message("board: using " BOARD_HEADER)` build-time echo to prevent silent fallbacks (HC-5).

3. **Contract Header (`hardware/_common/board_contract.h`)**:
   - Enforces display symbol definitions conditionally (`#ifdef WITH_DISPLAY_U8G2`, `#ifdef WITH_DISPLAY_LVGL`).
   - Emits legible `#error` messages if mandatory pins are missing, reinforced by `static_assert`.

4. **Null Board (`hardware/native/board.h`)**:
   - Provides a host native board mapping where all display pins are set to `PIN_UNASSIGNED`.
   - Headless by construction (neither `WITH_DISPLAY_*` flag defined by default).
   - Fully satisfies mandatory HC-1 documentation requirements (ASCII-art pinout diagram + 3-tier pin-allocation notes).

5. **Authoring Template (`hardware/_TEMPLATE.board.h`)**:
   - Documented reference skeleton containing mandatory ASCII pinout diagram and 3-tier pin allocation strategy notes to guide WO2/WO3.
