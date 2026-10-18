---
name: pr39-wo2
description: Handoff instructions for Work Order 2 — Include discipline & code quality fixes
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Fix include discipline violations (app-layer code must only use `#include "AppFoundation.h"`) and code quality issues in the sample app. This covers fix items F05–F12 from the review.

# Strict Boundary Box

- **ONLY** touch files inside `sample-app/src/`.
- Do **NOT** modify any framework kernel code (`modules/`, `lib/`, `include/`, `src/`).
- Do **NOT** create new branches or merge anything.

# Context

- `AppFoundation.h` is the single facade include for all app-layer code. Check what it re-exports by reading `/Users/mcneillm/Documents/Projects/esp32-app-foundation/include/AppFoundation.h`.
- `HomeAssistantModule.hpp` and `entities/HaSensor.hpp` are kernel-internal headers. They may or may not be re-exported by `AppFoundation.h`. If they are NOT, you may include them directly in `.cpp` files (not `.hpp`) and document the exception with a comment.
- The ESP32 App Foundation uses `Application.hpp`, `ConfigManager.hpp`, `DataManager.hpp` etc. internally. Check if `AppFoundation.h` provides these transitively.

# Tooling Constraints

- Use the `platformio` MCP server tools for builds — not raw `pio` CLI commands.
- The sample-app's PlatformIO project root is `/Users/mcneillm/Documents/Projects/esp32-app-foundation/sample-app`.

# Checklist

- [ ] **F05:** In `WeatherClockBoard.hpp`, replace `#include "interfaces/IDisplayBoard.hpp"` with `#include "AppFoundation.h"`. Remove the redundant `#include "WeatherModule.hpp"` if it's already pulled in, but keep it if needed for the `WeatherModule*` member type.

- [ ] **F06:** In `WeatherModule.hpp`, check if `HomeAssistantModule.hpp` and `entities/HaSensor.hpp` are re-exported by `AppFoundation.h`. If not, use forward declarations in the header and move the includes to `WeatherModule.cpp`.

- [ ] **F07:** In `WeatherModule.cpp`, check if `Application.hpp`, `ConfigManager.hpp`, `DataManager.hpp`, `MCPToolRegistry.hpp` are re-exported by `AppFoundation.h`. If yes, replace all four includes with a single `#include "AppFoundation.h"` (it's already included via the header). If they provide additional symbols not in the facade, keep them and add a comment explaining why.

- [ ] **F08:** In `main.cpp`, add a comment above the `weatherModule.begin(app)` call explaining the ordering rationale: why `begin()` is called before `registerModule()` (the module needs its dependencies wired before the framework calls lifecycle methods during `app.begin()`).

- [ ] **F09:** In `WeatherModule.hpp`, add a public getter: `const std::string& getUnits() const { return _units; }`. In `WeatherModule.cpp` MCP handler (~L746), replace `self->_units` with `self->getUnits()`.

- [ ] **F10 + F11:** In `WeatherWidget.cpp`, replace `std::string text = tempStr + " C"` with:
  ```cpp
  char text[24];
  snprintf(text, sizeof(text), "%s °C", tempStr.empty() ? "--" : tempStr.c_str());
  ```
  Remove the separate empty check below since `snprintf` handles it. Update the `display.print()` call to use `text` directly.

- [ ] **F12:** In `WeatherModule.cpp`, replace all occurrences of `String output; serializeJson(doc, output); return response->send(200, "application/json", output.c_str());` with a stack buffer pattern:
  ```cpp
  char buf[512];
  size_t len = serializeJson(doc, buf, sizeof(buf));
  return response->send(200, "application/json", buf);
  ```
  There are 3 occurrences (GET config, POST config response, GET status). The status endpoint may need a larger buffer (~1024 bytes) since it serialises the full weather payload.

# Verification

After making all changes:
1. Run `pio run -e esp32dev` via MCP tools to verify hardware build compiles.
2. Run `pio run -e native` via MCP tools to verify native build compiles.
3. Report results.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
