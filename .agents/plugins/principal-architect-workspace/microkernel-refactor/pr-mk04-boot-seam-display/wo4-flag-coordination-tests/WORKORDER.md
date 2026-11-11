---
name: mk04-wo4
description: Handoff instructions for Work order 4
complexity: standard
---

# mk04 / WO4 - Flag Coordination & Smoke Tests

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

**Objective:** Ensure that `template-app/src/main.cpp` properly orchestrates the display modules based on compilation flags, and physically verify that the display boot splash is rendering on a hardware target using the `pio-manager` and `device-mcp-client` skills.

**Strict Boundary Box:**
- Modify `template-app/src/main.cpp`.
- You MUST use the `pio-manager` skill for flashing/building.
- You MUST use the `device-mcp-client` skill for verifying the display output.

**Line-Item Instructions:**
- [ ] Review and update `template-app/src/main.cpp` so it cleanly handles display instantiation:
  - If `HAS_DISPLAY` is defined, check `BOARD_DISPLAY_KIND`. If it is `BOARD_DISPLAY_KIND_LVGL`, instantiate and register `LVGLDisplayModule`.
  - Otherwise, fallback to instantiating and registering `U8G2DisplayModule` (along with its `DiagnosticsBoard`).
  - Make sure the appropriate includes (`U8G2DisplayModule.hpp` / `LVGLDisplayModule.hpp`) are guarded correctly.
- [ ] Compile both `esp32dev` (U8G2) and `crowpanel-128-rotary` (LVGL) via `pio-manager`. Ensure both build successfully.
- [ ] Flash the firmware to a connected hardware target (e.g. `esp32dev` or `crowpanel-128-rotary`) using `pio-manager`.
- [ ] Use the `device-mcp-client` skill to connect to the device and verify that it booted successfully. If the device has a display tool (like `capture_display`), use it to capture the screen and confirm the boot splash (or the status log) is correctly rendered by the `IBootReporter` implementation. If `capture_display` is not available, verify the system status and logs using device tools to confirm `IBootReporter` executed without panic.
- [x] Once verified, STOP and send a message back to me. Do not wrap up or distill until I explicitly approve the work. Once I approve, invoke the `@distillery` skill to package your output, passing `.agents/plugins/principal-architect-workspace/microkernel-refactor/pr-mk04-boot-seam-display/wo4-flag-coordination-tests/` as the target directory. Finally, append a status update to the `WORKORDER.md` file and terminate your session.

### Status Update (2026-07-29)
- `main.cpp` was updated to bridge compilation flags (`HAS_LVGL_DISPLAY` / `HAS_DISPLAY` -> `BOARD_DISPLAY_KIND`).
- Both `esp32dev` and `crowpanel-128-rotary` compiled successfully.
- **Note:** The hardware flash and boot-splash smoke test was EXPLICITLY SKIPPED as directed by the Chief Architect due to the human being away from the physical device. This test is deferred to a later session.
- Distillery payloads generated.
