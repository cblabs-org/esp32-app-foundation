---
name: pr39-wo3
description: Handoff instructions for Work Order 3 — House style headers & embedded idioms
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Add file-level header comment blocks to all sample app source files (house style compliance) and replace `std::string` parser scratch fields with fixed `char[]` buffers for embedded idiom compliance. Covers fix items F13–F20.

# Strict Boundary Box

- **ONLY** touch files inside `sample-app/src/`.
- Do **NOT** modify any framework kernel code.
- Do **NOT** create new branches or merge anything.

# Context — House Style Header Format

Every file must begin with this comment block format (adapted to the actual content):

```cpp
/*
 * ESP32 App Foundation — Sample Application
 *
 * Module: sample-app/src/<filename>
 * Description: <1-2 sentence description of what this file does>
 *
 * Exported Classes/Functions:
 * - <ClassName or function>: <brief description>
 */
```

For reference, see how `sample-app/src/main.cpp` is already formatted.

# Tooling Constraints

- Use the `platformio` MCP server tools for builds — not raw `pio` CLI commands.
- The sample-app's PlatformIO project root is `/Users/mcneillm/Documents/Projects/esp32-app-foundation/sample-app`.

# Checklist

- [ ] **F17:** Add file-level header comment block to `WeatherModule.hpp`. List all implemented interfaces and key public methods in the Exported section.

- [ ] **F18:** Add file-level header comment block to `WeatherModule.cpp`. Reference the implementation of all interfaces.

- [ ] **F13:** Add file-level header comment block to `WeatherClockBoard.hpp`. Note the IDisplayBoard interface and the HOM relationship to WeatherModule.

- [ ] **F14:** Add file-level header comment block to `WeatherClockBoard.cpp`. Reference the render() implementation and thread-safety via atomic/mutex getters.

- [ ] **F15:** Add file-level header comment block to `WeatherWidget.hpp`. Note the pure static utility class role.

- [ ] **F16:** Add file-level header comment block to `WeatherWidget.cpp`. Note the U8G2 rendering implementation.

- [ ] **F19: Replace std::string parser scratch fields** — In `WeatherModule.hpp`, replace the 15 `std::string` parser scratch fields (_parsedTemp through _parsedIcon) with fixed `char[]` buffers:
  ```cpp
  char _parsedTemp[16] = "";
  char _parsedWind[16] = "";
  char _parsedCond[32] = "";
  // ... etc. Size each buffer to match the corresponding output buffer or 16 for numbers
  ```
  In `WeatherModule.cpp`:
  - Update `startDocument()` to zero-fill with `_parsedTemp[0] = '\0';` etc.
  - Update `value()` callback to use `strlcpy(_parsedTemp, value.c_str(), sizeof(_parsedTemp));`
  - Update `endDocument()` emptiness checks from `.empty()` to `_parsedTemp[0] != '\0'`
  - Update `std::strtof(_parsedTemp.c_str(), ...)` to `std::strtof(_parsedTemp, ...)`
  Also update `_currentKey` from `std::string` to `char[32]` — it only holds JSON key names which are short.

- [ ] **F20:** In `WeatherWidget.cpp`, add a comment above the hardcoded pixel offsets: `// Layout assumes 256×64 SSD1322 OLED — adjust offsets for different displays`

# Verification

After making all changes:
1. Run `pio run -e esp32dev` via MCP tools to verify hardware build.
2. Run `pio run -e native` via MCP tools to verify native build.
3. Report results.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
