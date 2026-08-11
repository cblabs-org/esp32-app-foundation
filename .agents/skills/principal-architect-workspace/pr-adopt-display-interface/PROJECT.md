# PR — Adopt Display-Interface Framework (forward-port SLS stack onto main)

> Bring the unmerged **`sls-screen-redesign` stack (WO1–WO7)** — the SquareLine
> redesign, the LVGL CONFIGURING orchestration (#82), the boot-screen rework, and the
> single `FaultScreen` — onto a `main` that has since gained the unified display
> interface. **No merge / rebase / squash** (git history hygiene, human directive):
> we *forward-port* — new branch off `main`, file-level copy of the clean SLS files,
> hand-reconcile the small overlap.

**Base branch:** `origin/main` @ `6fffa41`
**Source of the SLS work:** branch `sls-screen-redesign/wo7-hardware-test-debug` (tip `4c57f72`)
**Human approval:** "proceed please" (2026-08-11) — approved the forward-port strategy below and PA-led execution.

---

## 1. Context — what changed on `main` since the SLS stack branched

Merge-base is `#81` (`8ff3791`). Since then `main` merged:

- **#86** Unified `IDisplayModule` interface (RFC #85) — `LVGLDisplayModule`/`U8G2DisplayModule`
  now implement `IDisplayModule`; adds `capturePNG(Print&)`, `showSystemState()`, geometry/
  brightness/screensaver accessors; new `lib/PngWriter/`; **deleted** builtin `capture_display`
  + `DisplayInfo`; each display self-registers its `capture_display` MCP tool.
- **#88** `DisplayServiceHelper` — shared contract-driven web + MCP display routes (`lib/DisplayServiceHelper/`).
- **#89** Portal display sub-pills + diagnostics drawer — touches `LVGLDisplayModule.*`, `U8G2DisplayModule.*`,
  `WebServerManager.cpp`, all three `main.cpp`, portal JS.
- **#87** Portal Design System remediation (portal JS/CSS + `framework_design_system` skill submodule).
- **#90** docs cross-reference.

Crucially, `main` still has the **old** `reportStatus`/built-in `BootScreen`/`ErrorScreen`/
`OTAProgressScreen`, and still has **`_onAppConfiguring` (EventBus)** — so #86's `showSystemState`
did NOT replace the rich configuring path; they coexist. Our boot rework + CONFIGURING work
therefore *re-apply* rather than fight the new interface.

## 2. Merge surface (why forward-port is cheap here)

Of ~54 files the SLS stack touched, **~50 are clean** (`main` never touched them since base) and
copy over verbatim. Collisions are concentrated in **4 files** + a few doc/submodule merges:

| File | Resolution |
|------|------------|
| `modules/LVGLDisplay/LVGLDisplayModule.cpp` / `.hpp` | **Start from `main`** (keep `IDisplayModule`/`capturePNG`/`showSystemState`/`DisplayServiceHelper`/#89 sub-pills), **re-apply the SLS boot rework** (SLS-driven `reportStatus`, `showBoot`/`showError`/`showOtaProgress` → skin or `FaultScreen`, delete the 3 `_builtIn*` screens, add `FaultScreen`/`showFault`, move `ui_init()` early, `_uiInitialized`/`_lastBootProgress`). |
| `modules/MCPServer/MCPBuiltinTools.cpp` | **Take `main`, drop ours** — #86 deleted the builtin `capture_display`; the WO7 `#ifndef WITH_DISPLAY_LVGL` guard is obsolete. |
| `src/main.cpp` (+ sample/template) | **Take `main`'s** `IDisplayModule*` wiring; ensure `SLSSystemScreens` is still passed to the LVGL module and boot-reporter order preserved. |
| `modules/LVGLDisplay/README.md`, `docs/developer/mcp-server.md` | Merge (keep DS + FaultScreen notes). |
| `.gitmodules`, `components/esp_littlefs` | Take `main`'s submodule state. |

## 3. Work Order Ledger

Stacked; wo1 bases on `origin/main`, wo2 on wo1.

- **[ ] [wo1-forward-port](wo1-forward-port/WORKORDER.md)** *(architect)* — Forward-port the clean SLS
  files, reconcile the 4 collision files + doc/submodule merges, drop the obsolete `capture_display`
  guard. **Gate:** native suite green + `crowpanel-128-rotary` + `esp32dev` + `esp32s3nano` all compile.
  Autonomous worker (objective build/test gate).
- **[ ] [wo2-hardware-configuring](wo2-hardware-configuring/WORKORDER.md)** *(architect, interactive)* —
  On physical CrowPanel: re-verify the WO7 wins on the new base (boot animation, colours), exercise
  `capturePNG` on-device (closes the #86 S3-ROM-`tdefl` runtime gap), then resume the CONFIGURING phase
  (Setup/Busy/Message via `_onAppConfiguring`) + reconcile `showSystemState(Configuring)`. Human-at-bench.

## 4. Open design decisions (resolve in WO1/WO2)

- `showSystemState(Configuring)` currently calls `reportStatus()` (paints the *boot* screen). After our
  rework `reportStatus` drives the SLS boot screen — wrong for configuring. **WO1:** leave as-is (compiles,
  behaviourally same-ish); **WO2:** repoint to the configuring dispatch or unify with `_onAppConfiguring`.
- Route our display web/MCP screenshot/status through `DisplayServiceHelper` (adopt #88) where applicable.

## 5. Session Log

| Date | Event |
|------|-------|
| 2026-08-11 | Reviewed #86 (twice; blockers fixed by author) + #87/#88/#89. Mapped merge surface (4 collision files, ~50 clean). Human directed forward-port (no merge/squash) + PA-led execution ("proceed please"). Created `adopt-display-interface/wo1-forward-port` off `origin/main` @ `6fffa41`. Decomposed into wo1 (forward-port to green) + wo2 (hardware + CONFIGURING). |
