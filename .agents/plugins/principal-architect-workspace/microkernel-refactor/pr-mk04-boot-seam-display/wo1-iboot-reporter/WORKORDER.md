---
name: mk04-wo1
description: Handoff instructions for Work order 1
complexity: architect
---

# mk04 / WO1 - IBootReporter seam

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

**Objective:** Add `IBootReporter : public IStatusReporter` with one `beginEarly()` hook. Modify `Application` so `begin()` step 0 scans the registry for an `IBootReporter`, calls `beginEarly()`, and adopts it as the status reporter. Implement a `NullStatusReporter` fallback.

**Strict Boundary Box:**
- Touch ONLY `modules/Application/Application.hpp`, `modules/Application/Application.cpp`, and create `modules/Application/interfaces/IBootReporter.hpp`.
- Do NOT touch the display modules yet (U8G2 or LVGL) — that is WO2.
- Do NOT extract MCP, OTA, or Time.

**Tooling Constraints:**
- Use the `platformio` MCP tools (e.g., `build_project`, `run_tests`) to compile and test. Do NOT run `pio` CLI commands directly in the terminal.

**Line-Item Instructions:**
- [ ] Create `modules/Application/interfaces/IBootReporter.hpp` inheriting from `IStatusReporter.hpp`. Add `virtual bool beginEarly() = 0;`.
- [ ] Modify `Application.hpp`:
  - Include `"interfaces/IBootReporter.hpp"`.
  - Add `bool registerBootReporter(IBootReporter* reporter);`.
  - Add `IBootReporter* _bootReporter = nullptr;` member.
  - Update `registerModule<T>` to also check `std::is_base_of_v<IBootReporter, T>` and call `registerBootReporter`.
- [ ] Modify `Application.cpp`:
  - Implement `registerBootReporter()`.
  - In `Application::begin()`, *before* Phase 1 (or replacing Phase 1), if `_bootReporter` is set, call `_bootReporter->beginEarly()` and set `_statusReporter = _bootReporter`.
  - If `_bootReporter` is null, `_statusReporter` falls back to `&_nullReporter`.
  - Note: In WO1, since U8G2 is not yet a registered module (it's hardcoded), `_bootReporter` will be null for now. Ensure that the existing hardcoded display bootstrap in `Application::begin()` (`_display.bootstrapHardware(); _statusReporter = &_display;`) is preserved as a temporary fallback if `HAS_DISPLAY` is defined and no `_bootReporter` is registered. This prevents breaking compilation and behavior until WO2 extracts the display.
- [x] Verify `native` and `esp32dev` envs compile via MCP `build_project` and `run_tests`.
- [x] Once verified, STOP and send a message back to me. Do not wrap up or distill until I explicitly approve the work. Once I approve, invoke the `@distillery` skill to package your output, passing `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk04-boot-seam-display/wo1-iboot-reporter/` as the target directory. Finally, append a status update to the `WORKORDER.md` file and terminate your session.

## Status Update (2026-07-29)
**Completed**. The `IBootReporter` interface was successfully implemented and wired into `Application.hpp` and `Application.cpp`. The fallback for the hardcoded display bootstrap is preserved. Commits have been pushed to `feat(mk04-wo1): add IBootReporter seam`. Both `esp32dev` and `native` compilation/tests have passed successfully. `distillery` has packaged the session contexts.
