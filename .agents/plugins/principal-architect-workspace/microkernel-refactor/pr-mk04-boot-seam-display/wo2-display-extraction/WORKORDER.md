---
name: mk04-wo2
description: Handoff instructions for Work order 2
complexity: architect
---

# mk04 / WO2 - Display Extraction

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

**Objective:** Extract the display from being an owned member of the `Application` kernel. Register it dynamically as a module. Make both `U8G2DisplayModule` and `LvglDisplayModule` implement `IBootReporter`.

**Strict Boundary Box:**
- You will modify `Application.hpp`, `Application.cpp`, `U8G2DisplayModule.hpp/.cpp`, `LvglDisplayModule.hpp/.cpp`, and `template-app/src/main.cpp`.
- Do NOT touch MCP, OTA, or Time.

**Tooling Constraints:**
- Use the `platformio` MCP tools (e.g., `build_project`, `run_tests`) to compile and test. Do NOT run `pio` CLI commands directly in the terminal.

**Line-Item Instructions:**
- [ ] In `modules/U8G2DisplayModule/U8G2DisplayModule.hpp` and `.cpp`:
  - Inherit from `IBootReporter`.
  - Implement `bool beginEarly() override;`. Move the logic from `bootstrapHardware()` into `beginEarly()` and return `true`. You may keep `bootstrapHardware` and just call it from `beginEarly()`.
  - In `reportStatus`, `reportError`, `clearStatus`, ensure they still function as status reporters.
- [ ] In `modules/LvglDisplayModule/LvglDisplayModule.hpp` and `.cpp`:
  - Inherit from `IBootReporter`.
  - Implement `bool beginEarly() override;`. Add the equivalent early hardware bootstrap here (if applicable, else just return true). 
- [ ] In `modules/Application/Application.hpp`:
  - Remove the `#ifdef HAS_DISPLAY` block that includes `U8G2DisplayModule.hpp` and `DiagnosticsBoard.hpp`.
  - Remove the `#ifdef HAS_DISPLAY` block containing `_display`, `_diagnosticsBoard`, `_wifiWizardShown`, and `getDisplay()`.
- [ ] In `modules/Application/Application.cpp`:
  - Remove the `#ifdef HAS_DISPLAY` block in `begin()` that calls `_display.bootstrapHardware()` and sets `_statusReporter`. (WO1 already handles calling `_bootReporter->beginEarly()`).
  - Remove the `#ifdef HAS_DISPLAY` block in `begin()` that calls `_display.begin()`, `setConfigManager()`, configures `_diagnosticsBoard`, etc. This registration logic will be moved to `main.cpp`.
  - In `tick()`, remove the `#ifdef HAS_DISPLAY` block for the WiFi wizard modal.
  - In `_onEnterConfiguring()` and `_onEnterRunning()`, comment out the `#ifdef HAS_DISPLAY` contents (e.g., `// _display.carousel.suspend();`). We will fix this in WO3 by moving the behaviour to EventBus handlers. Just make sure the code compiles.
- [ ] In `template-app/src/main.cpp`:
  - Include the appropriate display header based on the `BOARD_DISPLAY_KIND` macro or whatever logic is currently there.
  - Instantiate the display (U8G2 or LVGL) as a global or static variable.
  - Instantiate `DiagnosticsBoard` if using U8G2.
  - Before `app.begin()`, call `app.registerModule(&display)`.
  - Also ensure that if the display requires `begin()` or `carousel.add()`, that it is handled appropriately after `app.begin()` or within the display's own `begin()` overriding `ITickable::begin()` or similar. Wait, the framework's `registerModule` handles registering it as an `IWebContributor`, `ITickable`, etc. For now, just make it compile.
- [ ] Verify `native` and `esp32dev` envs compile via MCP `build_project` and `run_tests`.
- [x] Once verified, STOP and send a message back to me. Do not wrap up or distill until I explicitly approve the work. Once I approve, invoke the `@distillery` skill to package your output, passing `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk04-boot-seam-display/wo2-display-extraction/` as the target directory. Finally, append a status update to the `WORKORDER.md` file and terminate your session.

## Status Updates
- **2026-07-29**: COMPLETED - Display extracted to IBootReporter and HAS_DISPLAY references removed from Application. Tests passed and work approved.
