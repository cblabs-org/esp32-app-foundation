# test/test-unit/test-u8g2Display

## Ruled out, failure signatures and gotchas

### [seed-493a6540] **Direct U8G2 Render in Native Tests**: Initial attempt to invoke `board.render(u8g2)` in native tests with an uninitialized `U8G2` mock driver caused memory faults due to null internal font pointers; updated tests to validate state calculation, snapshot mapping, and cause properties directly.
`ruled-out · kind: episodic · anchors: modules/U8G2Display/library.json, src/main.cpp, test/test_unit/test_u8g2Display/u8g2_module_native_stubs.cpp, test/test_unit/test_u8g2Display/test_boards.cpp, test/test_unit/test_u8g2Display/test_eventbus_integration.cpp`

### [seed-85c1845c] None. All file moves and include updates compiled and passed tests cleanly without side effects.
`ruled-out · kind: episodic · anchors: modules/U8G2Display/U8G2BootLogo.h, modules/U8G2Display/IU8G2SystemScreens.hpp, modules/U8G2Display/IDisplayBoard.hpp, modules/U8G2Display, include/AppFoundation.h, test/test_unit/test_u8g2Display/test_module.cpp, test/test_unit/test_u8g2Display/test_eventbus_integration.cpp`

## Open issues

### [seed-76ba1c89] Documentation file `docs/developer/custom-branding.md` (lines 77, 108, 157, 181) contains references to old header paths (`include/types/U8G2BootLogo.h` and `include/interfaces/IU8G2SystemScreens.hpp`). Per Strict Boundary Box rules, external docs were not modified and are surfaced for PA doc updates.
`open-issue · kind: semantic · anchors: modules/U8G2Display/U8G2BootLogo.h, modules/U8G2Display/IU8G2SystemScreens.hpp, modules/U8G2Display/IDisplayBoard.hpp, modules/U8G2Display, include/AppFoundation.h, test/test_unit/test_u8g2Display/test_module.cpp, test/test_unit/test_u8g2Display/test_eventbus_integration.cpp`
