# WO2 — Files Changed

## Modified Files

### `modules/U8G2Display/U8G2DisplayModule.hpp`
- Added `#include "interfaces/IMCPContributor.hpp"`
- Added forward declaration `class MCPToolRegistry;`
- Added `IMCPContributor` to class inheritance list
- Added `registerMCPTools(MCPToolRegistry& registry) override;` declaration
- Added IMCPContributor section in module header documentation

### `modules/U8G2Display/U8G2DisplayModule.cpp`
- Added `#include "../MCPServer/MCPToolRegistry.hpp"`
- Added full `registerMCPTools()` implementation (~145 lines):
  - `display_status`: read-safe tool returning brightness, flip, activeBoard, screensaver{enabled, state, dimTimeout, blankTimeout}, carousel{boardCount, rotationInterval, suspended}
  - `display_control`: mutating tool accepting action(wake/sleep), brightness(0-255), screensaverEnabled(bool), dimTimeout(ms), blankTimeout(ms) — persists via ConfigManager
- Updated module header comment to document new function and IMCPContributor role

### `modules/U8G2Display/CarouselEngine.hpp`
- Added `bool isSuspended() const { return _suspended; }` inline accessor (line ~152)

## New Files

### `test/test_unit/test_mcpServer/test_displayMcpTools.cpp`
- 32 GoogleTest cases in `DisplayMcpToolsTest` fixture
- Coverage: registration (5 tests), status response structure (12 tests), default values (6 tests), carousel state reflection (2 tests), control operations (7 tests)
- Uses standalone `MCPToolRegistry` + live `U8G2DisplayModule` — no MCPServer or Application wiring needed

## Untouched (per boundary box)
- `Application.cpp` — needs `registerMcpContributor(&_display)` one-liner (architect-owned)
- `ScreensaverController` internals — only used existing public API
- Board files — no changes
- Web portal files — no changes
- MCPServer files — no changes needed (existing API was sufficient)
