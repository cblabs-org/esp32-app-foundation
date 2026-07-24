---
name: lvgl-display-wo5
description: Handoff instructions for Work Order 5 — system screens, IStatusReporter, two-stage boot splash
complexity: standard
---

# lvgl-display / WO5 — System Screens, IStatusReporter & Two-Stage Boot Splash

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo4-config`
- **Your branch:** `lvgl-display/wo5-screens` (created for you — **do not branch**)
- **Stack position:** WO5 of 11.
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add `IStatusReporter` and the framework-driven system screens (boot, error, OTA progress) as native `lv_obj_t*` screens, plus the **two-stage brandable boot splash** and the `ILVGLSystemScreens` skin-provider wiring with built-in fallbacks.

## Context You Need
- Design D2 (no `IDisplayBoard` equivalent — native `lv_obj_t*` screens), D8 (brandable screens), design §9.3 (what LVGL replaces), §10.1 (`IStatusReporter`).
- **D8 two-stage splash + asset rule (critical):**
  - **Stage 1** boot logo is rendered in **Phase-1 `bootstrapHardware()`**, *before* LittleFS is mounted → it MUST be a **compiled-in** asset (`lv_image_dsc_t`), never filesystem-loaded. Default = a bundled CB Labs logo; the app overrides by passing its own `const lv_image_dsc_t*` (constructor param or weak-symbol override).
  - **Stage 2** = optional app SquareLine boot screen after `ui_init()` (from WO3 seam).
  - All module assets are **compiled into the binary** (D9, PR-47 §9.5) — do not add any LittleFS asset path.
- The `ILVGLSystemScreens*` provider (WO1) is already a constructor param on the module (WO3). For error/OTA: if the provider returns a screen, drive it via its `onError`/`onOtaProgress`; otherwise build/show the module's built-in default screen. The actual EventBus subscriptions that trigger these are WO6 — here, expose clean `showError()/showOtaProgress()/showBoot()` methods the module can call, and cover them with tests.

## Strict Boundary Box — do NOT:
- Subscribe to EventBus or implement screensaver (WO6) — only expose the screen-show methods + provider wiring.
- Implement web routes (WO7) or MCP tools/capture (WO8).
- Store any asset in LittleFS or add a filesystem-load path.
- Design a new boot-branding framework interface beyond `ILVGLSystemScreens` + the boot-logo injection (the framework-wide U8G2 retrofit is tracked in issue #39 — out of scope).

## Tooling Constraints
`platformio` MCP tools (`build_project`, `run_tests`) via `pio-manager` — never raw `pio`. `house-style-docs`. Heed `.agents/rules/`.

## Line-Item Instructions
- [ ] `modules/LVGLDisplay/screens/BootScreen.{hpp,cpp}`, `ErrorScreen.{hpp,cpp}`, `OTAProgressScreen.{hpp,cpp}` — built-in LVGL screens (labels/arc/bar), no filesystem assets.
- [ ] `modules/LVGLDisplay/assets/boot_logo.{c,h}` — a compiled-in default `lv_image_dsc_t` (small RGB565). Document the app-override mechanism (ctor `const lv_image_dsc_t*` and/or weak symbol).
- [ ] Add `public IStatusReporter` to `LVGLDisplayModule`; implement `reportStatus(const char*, float)` to drive the boot/status screen during boot.
- [ ] Implement `showBoot()` (Stage-1 compiled-in logo in `bootstrapHardware()`), `showError(code,msg)`, `showOtaProgress(pct,phase)` — each preferring the `ILVGLSystemScreens` provider, else the built-in screen; use `lv_scr_load_anim()` for transitions and restore the prior screen after OTA/error clears.
- [ ] `test/test_unit/test_lvglDisplay/test_screens.cpp`: built-in screens created; provider path used when non-null and fallback when null; boot logo defaults + override; prior-screen restore after OTA/error.

## Verification Requirement
- [ ] `run_tests` (native) — screen tests pass, full suite green.
- [ ] `build_project` `env:esp32s3nano` green.

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results and the screen-show API you exposed (WO6 will call it). Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo5-screens/`.
4. Append a status update to this `WORKORDER.md` and terminate.
