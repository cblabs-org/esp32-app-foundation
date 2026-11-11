---
name: lvgl-display-wo1
description: Handoff instructions for Work Order 1 — foundation (adapter interface, LVGL config, build env)
complexity: standard
---

# lvgl-display / WO1 — Foundation: Interfaces, LVGL Config & Build Env

## Provenance & Authorization
- **Parent project/PR:** PR-47 LVGL Display Module — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `main`
- **Your branch:** `lvgl-display/wo1-foundation` (created for you by the Principal Architect — **do not create branches yourself**)
- **Stack position:** bottom of stack (WO1 of 11). WO2–WO11 base on this branch.
- **Authorization:** Your authority to implement autonomously is your `WORKORDER.md` + `.agents/rules/`, valid **only** once PR-47's [`PROJECT.md`](../PROJECT.md) status log records the human's explicit approval of the decomposition. Verify that entry exists before making changes; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Lay the compile/config foundation for the generic LVGL display module: the two framework interfaces, the module-local LVGL config, and the PlatformIO env + build wiring — **with no runtime behaviour yet.**

## Context You Need (paged in)
- The framework is a decoupled, contract-based ESP32 framework. Optional display modules live under `modules/` and are auto-discovered via `lib_extra_dirs = modules` in `platformio.ini`. The existing peer module is `modules/U8G2Display/` — mirror its `library.json` shape (`modules/U8G2Display/library.json`).
- Design doc: [`docs/lvgl-display-design.md`](../../../../../docs/lvgl-display-design.md) — §3.2 (adapter interface), §12 (PlatformIO + `lv_conf.h`).
- **Amendments to the design doc that this WO implements** (design doc is not yet updated; follow the WORKORDER, not §12.1 verbatim):
  - `lv_conf.h` is **module-local** (`modules/LVGLDisplay/lv_conf.h`) wired via `-DLV_CONF_PATH`, **not** repo root with `LV_CONF_INCLUDE_SIMPLE`.
  - The crowpanel env **reuses the existing `partitions.esp32s3nano.csv`** (16 MB scheme) — do **not** author a new partition table (PR-47 §9.6, D9; partition redesign deferred to framework issue #40).
  - Assets are compiled into the binary, not LittleFS (D9) — relevant later; nothing to do here beyond not adding a filesystem-asset path.
- Read the `framework-expert` skill (interface contracts) and `house-style-docs` skill (header/comment standards) before writing headers.

## Strict Boundary Box — do NOT:
- Implement any module logic (`LVGLDisplayModule.cpp` is WO3), any screens, or any hardware adapter.
- Author a new partition CSV, or modify `partitions.esp32dev.csv` / `partitions.esp32s3nano.csv`.
- Touch `U8G2Display`, or any env other than the new `crowpanel-128-rotary` and the `native` test wiring.
- Add `lvgl` to the base or native `lib_deps` (native uses stubs from WO2; the real lib is crowpanel-only).
- Change framework core, ConfigManager, OTA, or web portal.

## Tooling Constraints
Use the **`platformio` MCP server** tools, never raw `pio` CLI (project rule `.agents/rules/04-mcp-tooling.md`). Route all builds through the `pio-manager` skill. Relevant tools: `build_project`, `clean_project`, `run_tests`. Follow `house-style-docs` for all new headers.

## Line-Item Instructions
- [ ] Create `include/interfaces/ILVGLHardwareAdapter.hpp` — pure abstract interface exactly per design §3.2 (required: `createDisplay()`, `setBrightness()`, `getScreenWidth()`, `getScreenHeight()`, `getBufferLineCount()`; optional with `nullptr` defaults: `createTouchInput()`, `createEncoderInput()`, `createKeypadInput()`). Forward-declare `lv_display_t`/`lv_indev_t` via `#include "lvgl.h"`. House-style documented.
- [ ] Create `include/interfaces/ILVGLSystemScreens.hpp` — optional system-screen skin provider (D8): virtual `otaScreen()`, `errorScreen()` returning `lv_obj_t*` (default `nullptr`), and `onOtaProgress(float, const char*)`, `onError(const char*, const char*)` (default no-op). House-style documented.
- [ ] Create `modules/LVGLDisplay/lv_conf.h` — LVGL v9 config per design §12.2 (RGB565 `LV_COLOR_DEPTH 16`, `LV_MEM_SIZE 32K`, Montserrat 14/20/28, log level WARN, `LV_INDEV_DEF_READ_PERIOD 30`). This is the shipped default; document that apps may override via their own `-DLV_CONF_PATH`.
- [ ] Create `modules/LVGLDisplay/library.json` — mirror `modules/U8G2Display/library.json` (name `LVGLDisplay`, `build.flags: ["-I../../include"]`). **Do not** hard-declare an `lvgl` dependency (the env provides it).
- [ ] Add `platformio.ini` env `[env:crowpanel-128-rotary]` per design §12.1 **with amendments**: `extends = env:base_esp32`; ESP32-S3 / 16 MB / `qio_opi` PSRAM; `board_build.partitions = partitions.esp32s3nano.csv`; `board_build.filesystem = littlefs`; `lib_deps += lvgl/lvgl @ ^9.5.0`; build flags `-DHAS_LVGL_DISPLAY=1`, `-DLV_CONF_PATH=${PROJECT_DIR}/modules/LVGLDisplay/lv_conf.h`, and all GPIO pin flags from §12.1; add `adapters` to `lib_extra_dirs` so the (future) reference adapter compiles for this env.
- [ ] Update `[env:native]`: ensure `LVGLDisplay` is **not** `lib_ignore`d (it must compile via WO2 stubs); add `lib_ignore` for the hardware adapter library name (the `adapters/lvgl` reference adapter) so native never pulls hardware code. Do not add `lvgl` to native `lib_deps`.
- [ ] Confirm the new `crowpanel-128-rotary` env is excluded from any default CI build matrix that would fail before WO9 provides the adapter (leave a comment noting it will not fully build until WO9).

## Verification Requirement
- [ ] `env:native` compiles (the module dir contains only headers + `lv_conf.h` + `library.json` at this stage; nothing should break the native build). Use the `platformio` MCP `build_project` for `native`, and `run_tests` to confirm the existing suite still passes (no regressions).
- [ ] `env:esp32s3nano` still builds green (you touched shared `platformio.ini`).
- [ ] The `crowpanel-128-rotary` env **resolves** `lib_deps` (LVGL fetched) even though full compilation depends on WO2/WO3/WO9 — confirm the env parses and dependency resolution succeeds.

## Completion Protocol
1. When implementation is validated, **STOP and send a message back to the Principal Architect** reporting results and any issues/iteration needed. Surface any out-of-boundary findings in your report (do not file issues yourself).
2. **Do not wrap up or distill until the human explicitly approves.**
3. Once approved: invoke the `@distillery` skill, passing `.agents/plugins/principal-architect-workspace/pr-47-lvgl-display/wo1-foundation/` as the target directory.
4. Append a status update to this `WORKORDER.md` and terminate your session.
