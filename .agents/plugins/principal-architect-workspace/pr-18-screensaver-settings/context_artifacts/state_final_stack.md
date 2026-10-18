# PR-18 Final Commit Stack

## Branch: `pr-18-screensaver-settings` (based on `pr-17-wo4-module-root`)

| # | Hash | Description | Author |
|---|---|---|---|
| 1 | `e47db28` | feat(display): PR-18 WO1 — screensaver enable/disable, getDimTimeout accessor, 14 new tests | WO1 Worker |
| 2 | `d8e8746` | docs(distillery): PR-18 WO1 config expansion — context bridge, ADRs, session registry | WO1 Worker |
| 3 | `87208b8` | feat(display): PR-18 WO2 — display_status and display_control MCP tools | WO2 Worker |
| 4 | `805911b` | docs(distillery): PR-18 WO2 MCP tools — context bridge, ADRs, session registry | WO2 Worker |
| 5 | `f25057c` | fix(app): wire U8G2DisplayModule as MCP contributor | PA |
| 6 | `909df5b` | feat(portal): PR-18 WO3 — Display Settings tab | WO3 Worker |
| 7 | `38b549f` | docs(distillery): PR-18 WO3 display tab — context bridge, ADRs, session registry | WO3 Worker |
| 8 | `2ffa1ec` | docs(pa): PR-18 project closed | PA |
| 9 | `bab104d` | fix(test): replace MAX_CONFIGURABLES tests with dynamic vector + lock tests | PA |

## Key Files Changed

### Firmware (C++)
- `modules/U8G2Display/ScreensaverController.hpp` — `setEnabled()`, `isEnabled()`, `getDimTimeout()`, `_enabled` member
- `modules/U8G2Display/ScreensaverController.cpp` — implementations + tick() wake ordering
- `modules/U8G2Display/U8G2DisplayModule.hpp` — `IMCPContributor` inheritance, `registerMCPTools()` declaration
- `modules/U8G2Display/U8G2DisplayModule.cpp` — `registerMCPTools()` (~145 lines: display_status + display_control)
- `modules/U8G2Display/CarouselEngine.hpp` — `isSuspended()` accessor
- `modules/Application/Application.cpp` — `_mcpContributors.push_back(&_display)` wiring

### Web Portal
- `web/portal/appconfig.js` — Full Display Settings tab UI (brightness, flip, screensaver, clock)
- `include/portal_assets/appconfig_js.hpp` — PROGMEM-embedded portal JS

### Tests
- `test/test_unit/test_u8g2Display/test_screensaver.cpp` — 13 screensaver enable/disable tests
- `test/test_unit/test_u8g2Display/test_config.cpp` — schema v3 migration test
- `test/test_unit/test_mcpServer/test_displayMcpTools.cpp` — 32 MCP tool tests (NEW)
- `test/test_unit/test_application/test_application.cpp` — MAX_CONFIGURABLES → dynamic vector tests
- `test/web/tests/display.spec.ts` — 14 Playwright display tab tests (NEW)
- `test/web/server.js` — Mock display API endpoints

## Worker Sessions
- WO1: `e4941b47-2ef8-4ad6-b023-004ca8ab37f7`
- WO2: `1083988a-31c8-4e4e-960d-5fd304c09e5d`
- WO3: `b5c6718f-cf86-4eea-8e13-efdd13f6764d`
