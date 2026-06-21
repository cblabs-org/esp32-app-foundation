---
name: pr-35-wo3
description: Handoff instructions for Work Order 3 — UI Widgets & Application Assembly
complexity: standard
---

# Work Order 3: UI Widgets & Application Assembly

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Build the U8G2 display widgets and tie the application together in `main.cpp`.

## Strict Boundary Box
- You may ONLY modify files within: `sample-app/src/`.
- Do NOT touch `modules/` or any framework code.

## Tooling Constraints
- **CRITICAL:** You MUST use the PlatformIO MCP server tools (`build_project`, `run_tests`, `check_project`) to verify compilation and run tests. Do NOT use raw `pio` CLI commands in the terminal.
- Read the framework-expert skill at `.agents/skills/framework-expert/SKILL.md` for house style and coding conventions.

## Line-Item Instructions
- [ ] **1.** Create `sample-app/src/WeatherWidget.hpp` and `.cpp` to draw the weather icon and temperature.
- [ ] **2.** Create `sample-app/src/WeatherClockBoard.hpp` and `.cpp` (implementing `IDisplayBoard`).
- [ ] **3.** Modify `sample-app/src/main.cpp`:
  - Statically instantiate `HomeAssistantModule` and `WeatherModule`.
  - Wire DI: `weatherModule.setHomeAssistant(&haModule)`.
  - Register both modules via `app.registerModule()`.
  - Configure the `CarouselEngine` to rotate between `WeatherClockBoard` and the native `DiagnosticsBoard`.

## Verification Requirement
1. Run `build_project` via the PlatformIO MCP server for the `esp32s3nano` environment to verify compilation.
2. **CRITICAL:** Before distilling or committing, STOP and send a message back to the Principal Architect confirming results. Only after approval should you invoke the `@distillery` skill.
