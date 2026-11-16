---
schema_version: 1
wo_id: WO-144.3
pr_id: PR-144
title: Add missing native portal asset stubs and pin LVGL 9.5.0 / LovyanGFX 1.2.26
complexity: standard
status: PENDING
allowed_files:
- test/stubs/platform/portal_assets/
- platformio.ini
- modules/LVGLDisplay/LVGLDisplay.hpp
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills: []
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: b91f65af86ef75068617389a21cc8a78be1919669ce493db4b678fa926356574
completion: null
---

# WO-144.3 — Add missing native portal asset stubs and pin LVGL 9.5.0 / LovyanGFX 1.2.26

## 1. Objective & Scope

Resolve the two CI failures surfaced in live GitHub Actions run `43836294622` on branch `launch/pr-e-ci-workflows`:
1. **`Native Tests & Doc C++ Samples` (`test_native/test_homeassistant`, `test_unit/test_homeassistant`):** Clean CI checkouts do not have the gitignored `include/portal_assets/` directory generated prior to firmware builds, and `test/stubs/platform/portal_assets/` lacked `homeassistant_js.hpp` (plus `ble_js.hpp` and `carousel_js.hpp`).
2. **`Firmware Build (root-crowpanel)`:** Unpinned `lvgl/lvgl@^9.5.0` at `platformio.ini:182` pulled newly released `lvgl @ 9.6.0`, whose `lvgl_private.h` -> `ft81x/lv_ft81x_defines.h` `#define CMD_ROTATE 0xFFFFFF29` macro collided with `LovyanGFX` `Panel_M5UnitLCD.hpp:79` `CMD_ROTATE`.

Scope is strictly bounded to `test/stubs/platform/portal_assets/`, `platformio.ini`, and `modules/LVGLDisplay/LVGLDisplay.hpp`.

## 2. Detailed Specification

### 2.1 `test/stubs/platform/portal_assets/` — Missing Portal Asset Stubs

Create three stub headers matching `test/stubs/platform/portal_assets/hardware_js.hpp` using `#pragma once`, `<cstddef>`, `<cstdint>`, and `inline constexpr`:

1. **`test/stubs/platform/portal_assets/homeassistant_js.hpp`:**
   ```cpp
   #pragma once
   #include <cstddef>
   #include <cstdint>
   inline constexpr uint8_t homeassistant_js_gz[] = {0};
   inline constexpr size_t homeassistant_js_gz_len = 0;
   inline constexpr char homeassistant_js_gz_etag[] = "";
   ```

2. **`test/stubs/platform/portal_assets/ble_js.hpp`:**
   ```cpp
   #pragma once
   #include <cstddef>
   #include <cstdint>
   inline constexpr uint8_t ble_js_gz[] = {0};
   inline constexpr size_t ble_js_gz_len = 0;
   inline constexpr char ble_js_gz_etag[] = "";
   ```

3. **`test/stubs/platform/portal_assets/carousel_js.hpp`:**
   ```cpp
   #pragma once
   #include <cstddef>
   #include <cstdint>
   inline constexpr uint8_t carousel_js_gz[] = {0};
   inline constexpr size_t carousel_js_gz_len = 0;
   inline constexpr char carousel_js_gz_etag[] = "";
   ```

### 2.2 `platformio.ini` (`[env:crowpanel-128-rotary]`, lines 182–183) — Exact Library Pins

- Replace unpinned Caret dependencies in `[env:crowpanel-128-rotary]` `lib_deps`:
  - `lvgl/lvgl@^9.5.0` -> `lvgl/lvgl@9.5.0`
  - `lovyan03/LovyanGFX@^1.1.16` -> `lovyan03/LovyanGFX@1.2.26`

### 2.3 `modules/LVGLDisplay/LVGLDisplay.hpp` — `CMD_ROTATE` Macro Guard

- Directly after `#include "ui_gen.h"` (line 80, before `// Forward declarations`), add:
  ```cpp
  // Prevent LVGL private ft81x driver macro (LVGL 9.6+) from colliding with LovyanGFX Panel_M5UnitLCD::CMD_ROTATE.
  #ifdef CMD_ROTATE
  #undef CMD_ROTATE
  #endif
  ```

## 3. Verification Requirements

1. **Mandatory Gate Command:**
   - `pytest scripts/tests/ && python3 scripts/doc_lint.py` (must exit `0` with zero failures).
2. **Additional Relevant Checks (run via PlatformIO MCP / workspace tools per `.agents/rules/04-mcp-tooling.md`):**
   - Run `python3 scripts/check_doc_cpp_samples.py` and PlatformIO MCP `run_tests` on `native` and `native_ha`, confirming they pass even without gitignored `include/portal_assets/` present (do not rely on generated `include/portal_assets/` headers; verify clean-checkout stub resolution).
   - Run PlatformIO MCP `build_project` on `crowpanel-128-rotary` to verify `lvgl/lvgl@9.5.0`, `lovyan03/LovyanGFX@1.2.26`, and the `#undef CMD_ROTATE` guard build cleanly.
3. **Cryptographic Receipt (`pa wo verify`):**
   - None of `WO-144.3`'s `allowed_files` are dotfiles, so run `python3 .agents/plugins/principal-architect/scripts/pa_cli.py wo verify WO-144.3 --pr-id PR-144` after edits and all checks pass (only escalate to PA if an unexpected tooling/dotfile blocker occurs).
   - Keep `completion.verification_summary` strictly `<= 280` characters.

## Completion Report
