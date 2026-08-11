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

- **[x] [wo1-forward-port](wo1-forward-port/WORKORDER.md)** *(architect)* — Forward-port the clean SLS
  files, reconcile the 4 collision files + doc/submodule merges, drop the obsolete `capture_display`
  guard. **Gate:** native suite green + `crowpanel-128-rotary` + `esp32dev` + `esp32s3nano` all compile.
  Autonomous worker (objective build/test gate). **COMPLETE** — tip `216f160`.
- **[x] wo2-hardware-configuring** *(interactive, multi-agent)* —
  On physical CrowPanel: exercise `capturePNG` on-device (close the #86 S3-ROM-`tdefl` runtime gap),
  fix boot→configuring→running transition, verify on hardware. **COMPLETE** — screenshot API stable,
  boot transition restored, `v0.0.12-dev.44` verified on device. Merged to `main` at `b393e74`.

## 4. Open design decisions — RESOLVED

- ~~`showSystemState(Configuring)` calls `reportStatus()`~~ — **Resolved:** The forward-port's gutted
  `_onAppConfiguring`/`_onAppRunning` handlers were restored from the SLS branch. `_onAppConfiguring`
  now uses the #82 three-tier dispatch (Setup/Busy/Message via `GateActionStatus`). `_onAppRunning`
  clears system-screen state and loads `ui_ui_DiagScreen`. `showSystemState` is unused (Application
  drives display purely via EventBus `APP_CONFIGURING`/`APP_RUNNING` events). No loop risk.
- ~~Route screenshot through `DisplayServiceHelper`~~ — **Deferred:** Screenshot API works directly
  via WebServerManager + LVGLDisplayModule cross-thread snapshot. DisplayServiceHelper integration
  is a future enhancement, not blocking.

## 5. Session Log

| Date | Event |
|------|-------|
| 2026-08-11 | Reviewed #86 (twice; blockers fixed by author) + #87/#88/#89. Mapped merge surface (4 collision files, ~50 clean). Human directed forward-port (no merge/squash) + PA-led execution ("proceed please"). Created `adopt-display-interface/wo1-forward-port` off `origin/main` @ `6fffa41`. Decomposed into wo1 (forward-port to green) + wo2 (hardware + CONFIGURING). |
| 2026-08-11 | **WO1 complete.** Forward-port (steps 1–3) done across sessions `b095e8d`→`5c6a3b9`. Final step: trimmed 5 native tests to post-#88 module contract (user-approved Option 1) — `216f160`. Verification: native **482/482** PASSED; `crowpanel-128-rotary`, `esp32dev`, `esp32s3nano` all **[SUCCESS]**. Next: WO2 (flash CrowPanel + CONFIGURING). |
| 2026-08-11 | **WO2 complete (multi-agent).** Session `b52fe7ae` (screenshot debugging agent) found: (1) ROM `tdefl_compressor` is ~164KB not 82KB → hardcoded 64KB under-allocation caused `LoadProhibited` crash, (2) 4KB HTTP worker stack overflowed from tdefl callback recursion → bumped to 16KB, (3) `lv_snapshot` called from HTTP thread (unsafe) → deferred to LVGL tick via `_pendingSnapshot`/`_snapshotComplete` atomics, (4) screensaver early-return blocked snapshot processing → moved below event drain. Session `39c65de6` (this session) picked up where that agent hit its usage limit: (a) identified the **real** boot transition root cause — forward-port kept main's gutted `_onAppConfiguring`/`_onAppRunning` (my WORKORDER said "keep main's" — that was the miss), restored the SLS three-tier dispatch + `ui_ui_DiagScreen` load, (b) fixed native `test_pngWriter` build — `#include <esp_heap_caps.h>` was inside `#ifdef ARDUINO` so native never got heap_caps symbols; moved include outside guard, added `heap_caps_calloc` to central stub, stripped redundant ad-hoc stubs from test. Hardware verified: `v0.0.12-dev.44`, boot→running transition works, screenshot API returns 240×240 RGB PNG in ~220ms, heap healthy (78KB DRAM, 8.4MB PSRAM). Merged to `main` at `b393e74`, pushed. |

### 2026-08-11 — Project Wrap-Up

**Architectural outcome**: The SLS SquareLine Studio display stack (WO1–WO7 from `sls-screen-redesign`) is now live on `main`, coexisting with the #86 unified `IDisplayModule` interface. The forward-port replaced the 3 built-in C++ screens (`BootScreen`/`ErrorScreen`/`OTAProgressScreen`) with SLS-generated UI files + `SLSSystemScreens` + `FaultScreen`. The CONFIGURING lifecycle orchestration (#82 three-tier dispatch) is wired and functional. The screenshot API is production-stable on ESP32-S3 with PSRAM-backed buffers.

**Cross-cutting decisions**:
- `showSystemState()` is effectively dead code — `Application` drives display purely via EventBus. Could be removed in a future cleanup pass.
- The CONFIGURING screen is never visible on warm boots with saved WiFi credentials (gate clears in <1s). This is correct behaviour — visible only when WiFi is missing/slow or other gates are pending.
- PngWriter's `#include <esp_heap_caps.h>` must be unconditional (not inside `#ifdef ARDUINO`) since the function body uses heap_caps symbols unconditionally.

**Dead ends (project-wide)**:
- The session report from the screenshot-debugging agent hypothesized `_screens->mainScreen()` returning nullptr as the boot-stuck cause. Wrong — `ILVGLSystemScreens` has no `mainScreen()`. The real cause was gutted EventBus handlers.
- Commit `2100c57` tried to fix native PngWriter by adding `#include <esp_heap_caps.h>` to PngWriter.cpp but put it inside `#ifdef ARDUINO` — no effect on native. The proper fix was moving it outside the guard.

**Unresolved items (deferred)**:
- Route screenshot/display-status through `DisplayServiceHelper` (#88) for consistency with U8G2.
- Tick budget warnings during boot transition (Module 1 exceeds 33ms budget for ~400ms during screen load). The `pr-tick-budget-override` project (planned, per-module override) would eliminate these false positives.
