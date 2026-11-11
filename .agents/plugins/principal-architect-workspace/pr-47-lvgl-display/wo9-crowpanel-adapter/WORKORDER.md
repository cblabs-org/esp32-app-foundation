---
name: lvgl-display-wo9
description: Handoff instructions for Work Order 9 — CrowPanel 1.28" reference hardware adapter
complexity: architect
---

# lvgl-display / WO9 — Reference Adapter: CrowPanel128Adapter

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo8-mcp-tools` (default linear stack — PR-47 §7 Option A). *(If the human elected Option B, this bases on `lvgl-display/wo3-module-core` instead — confirm from `PROJECT.md` before starting.)*
- **Your branch:** `lvgl-display/wo9-crowpanel-adapter` (created for you — **do not branch**)
- **Stack position:** WO9 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the reference hardware adapter `CrowPanel128Adapter` (implements `ILVGLHardwareAdapter`) for the Elecrow CrowPanel 1.28" — GC9A01 display, CST816D touch, rotary encoder, backlight — and the adapter authoring guide. **esp32 build only** (excluded from `env:native`).

## Context You Need
- Design §3.2 (interface), §4 (hardware/GPIO map), §7 (input), §8 (driver stack), §12.1 (pins as build flags).
- **Decisions to honour:**
  - **OQ-2 (driver backend):** attempt `esp_lcd_new_panel_gc9a01()` + `esp_lvgl_port` **first** (D6). Time-box this as a spike at the start; if it will not resolve cleanly in the Arduino/ESP-IDF hybrid build, fall back to **LovyanGFX** (design §8.2) and record the decision in your report + the adapter README.
  - **OQ-4 (encoder):** use **`ESP32Encoder` (hardware PCNT)** for rotation (the ESP32-S3 has PCNT — quadrature decoded in silicon, no ISR in the hot path); keep the encoder push-button as a debounced `digitalRead` poll in the `read_cb`. Retain the design §7.5 software-`IRAM_ATTR`-ISR approach **as the documented fallback in the adapter README** (for MCUs without PCNT), not as the primary path.
  - **OQ-7:** the generic module allocates buffers by default; `createDisplay()` returns an `lv_display_t*` without buffers unless this panel needs a bespoke strategy (it does not — SPI GC9A01 uses the module's partial SRAM buffers).
- Pins come from `platformio.ini` build flags (WO1), never hardcode.

## Strict Boundary Box — do NOT:
- Modify the generic `LVGLDisplayModule` or its interfaces (consume them only). If the interface is insufficient, **stop and report** — do not widen it yourself.
- Make this adapter compile on `env:native` (it is `lib_ignore`d there per WO1). No native tests for hardware code.
- Flash hardware — that is WO10. This WO is code + compile verification only.
- Add other catalogue adapters (CheapYellowDisplay etc. are out of scope / 16 MB-gated per D9).

## Tooling Constraints
`platformio` MCP tools — **`build_project` for `env:crowpanel-128-rotary`** to verify compilation; `clean_project` as needed. Via `pio-manager` — never raw `pio`. `house-style-docs`. Heed `.agents/rules/` embedded-safety rules (IRAM only for the *fallback* ISR path; no heap/`Serial` in any ISR; task-stack/DMA rules).

## Line-Item Instructions
- [ ] **Backend spike (first):** stand up GC9A01 via `esp_lcd`/`esp_lvgl_port`; if blocked in the hybrid build, switch to LovyanGFX. Record which was used.
- [ ] `adapters/lvgl/CrowPanel128Adapter.{hpp,cpp}` implementing `ILVGLHardwareAdapter`:
  - `createDisplay()` → init SPI bus + GC9A01 (pins from flags, `invert_color=true` per §4.3), wire `flush_cb` + DMA, return `lv_display_t*`; `getScreenWidth/Height()` = 240; `getBufferLineCount()` = 24 (§5.3).
  - `setBrightness(uint8_t)` → backlight PWM on `LCD_BL` (GPIO 46).
  - `createTouchInput()` → CST816D over I2C (SDA 6/SCL 7, addr 0x15), `read_cb` fills point/state; return `LV_INDEV_TYPE_POINTER`.
  - `createEncoderInput()` → `ESP32Encoder` on A/B (45/42) for rotation + debounced button poll on SW (41); `read_cb` fills `enc_diff` + state; return `LV_INDEV_TYPE_ENCODER`.
  - `createKeypadInput()` → `nullptr` (not present).
- [ ] Add `ESP32Encoder` to the `crowpanel-128-rotary` `lib_deps` (WO1's env). Confirm it remains `lib_ignore`d on native.
- [ ] `adapters/lvgl/README.md` — adapter authoring guide: how to implement `ILVGLHardwareAdapter`, the ≥16 MB flash prerequisite (D9), the encoder PCNT-vs-software-ISR fallback note (design §7.5), and the app wiring example (design §3.4).

## Verification Requirement
- [ ] `build_project` for `env:crowpanel-128-rotary` compiles green (full module + adapter link).
- [ ] `run_tests` (native) still green (adapter excluded there).
- [ ] **Measure and report** the crowpanel firmware size (PR-47 §9.3 requires the real LVGL figure).
- [ ] **No hardware flashing** — defer runtime verification to WO10.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results, the OQ-2 backend actually used, and the measured firmware size. Surface out-of-boundary findings (e.g. interface gaps) in your report — do not file issues or widen interfaces.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/plugins/principal-architect-workspace/pr-47-lvgl-display/wo9-crowpanel-adapter/`.
4. Append a status update to this `WORKORDER.md` and terminate.
