# ADR: U8g2Display Module Skeleton Design

## Decision: Override Board Pointer for System Boards

**Context**: The WO spec (line 145) says `showBoard()` with a non-pool board should "set `_activeBoardIndex = -1` and render directly (system board override)".

**Decision**: Added an `_overrideBoard` member pointer alongside the `_activeBoardIndex = -1` sentinel. When `showBoard()` receives a board not in the pool vector, it stores the pointer in `_overrideBoard` and sets `_activeBoardIndex = -1`. The render path checks for this override first.

**Rationale**: A bare `-1` index without a stored pointer would lose the reference to the system board. The override pointer cleanly separates pool-based carousel rotation from system board display without contaminating the vector.

---

## Decision: Library.json Without Platform Restrictions

**Context**: The WO template specified `"frameworks": ["arduino", "espidf"], "platforms": ["espressif32"]` in library.json.

**Decision**: Removed these restrictions from the final library.json.

**Rationale**: PlatformIO's LDF (Library Dependency Finder) in `strict` mode uses these fields to filter libraries per environment. The `native` test environment has `platform = native`, so a library restricted to `espressif32` is invisible to LDF in native builds. Since all hardware code is already `#ifndef NATIVE_BUILD` guarded in the source files, the platform restriction in library.json is redundant and actively harmful for testability.

---

## Decision: NATIVE_BUILD Guard Strategy

**Context**: The module uses U8G2 hardware driver, `millis()`, and other Arduino functions.

**Decision**: Every hardware call is individually wrapped in `#ifndef NATIVE_BUILD` guards rather than using a conditional compilation approach at the file level.

**Rationale**: This allows the same source file to compile on both hardware and native targets. The native build compiles the business logic (pool management, carousel state machine, index tracking) while skipping hardware I/O. This enables meaningful unit testing of the module's logic without mocking the entire U8G2 driver.

---

## Decision: U8G2_DISPLAY_TYPE Macro Default

**Context**: Different applications use different display controllers (SSD1306, SSD1322, etc.).

**Decision**: Defined `U8G2_DISPLAY_TYPE` as `U8G2_SSD1306_128X64_NONAME_F_HW_I2C` (guarded by `NATIVE_BUILD`), with the expectation that applications override it via `build_flags`.

**Rationale**: The SSD1306 128x64 I2C is the most common hobbyist OLED display. Applications targeting larger displays (like the departures-board's SSD1322 256x64 SPI) will override this macro in their `platformio.ini` build_flags.
