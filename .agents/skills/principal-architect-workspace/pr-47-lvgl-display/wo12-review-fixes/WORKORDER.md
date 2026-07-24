---
name: lvgl-display-wo12
description: Handoff instructions for Work Order 12 — PR-47 review remediation (thread safety, snapshot, cleanup)
complexity: standard
---

# lvgl-display / WO12 — Review Remediation: Thread Safety, Snapshot, Cleanup

## Provenance & Authorization
- **Parent project/PR:** PR-47 LVGL Display Module — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo11-docs`
- **Your branch:** `lvgl-display/wo12-review-fixes` (created for you by the Principal Architect — **do not create branches yourself**)
- **Stack position:** WO12 of 12 (remediation pass added after code review). Bases on WO11 tip.
- **Authorization:** Your authority to implement autonomously is your `WORKORDER.md` + `.agents/rules/`, valid **only** once PR-47's [`PROJECT.md`](../PROJECT.md) status log records the human's explicit approval. Verify that entry exists before making changes; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Fix all code-review findings from PR #41 that were accepted for remediation before merge. Two are merge-blocking (🔴), three are should-fix (🟠), four are minor polish (🟡). All are software-only — no hardware needed.

## Context You Need (paged in)
- The review comment is at `https://github.com/matthewmcneill/esp32-app-foundation/pull/41#issuecomment-5070483280`
- The response accepting/pushing-back is at the follow-up comment on the same PR.
- Read `modules/LVGLDisplay/LVGLDisplayModule.hpp` and `LVGLDisplayModule.cpp` — they are the primary targets.
- Read `house-style-docs` skill for header/comment standards.
- Read `pio-manager` skill before running any PlatformIO commands.

## Strict Boundary Box — do NOT:
- Modify the adapter (`CrowPanel128Adapter`) — encoder (#6) and DMA (#8) are WO10-gated.
- Change buffer line count (#5) — deferred to WO10 measurement.
- Touch U8G2Display, framework core, or any files outside the LVGL module + stubs + platformio.ini.
- Add new framework interfaces or change the `ILVGLHardwareAdapter` contract.
- Create branches or file issues.

## Tooling Constraints
Use the **`platformio` MCP server** tools, never raw `pio` CLI. Route all builds through the `pio-manager` skill. Relevant tools: `build_project`, `run_tests`.

## Line-Item Instructions

### 🔴 Fix #1 — Thread-safe EventBus callbacks (MERGE BLOCKER)

- [ ] In `LVGLDisplayModule.hpp`, add `#include <atomic>` and add cross-thread pending state members:
  - `std::atomic<bool> _pendingOtaUpdate{false}` — flag: new OTA progress to apply
  - `std::atomic<float> _pendingOtaProgress{0.0f}` — cached OTA progress value
  - `char _pendingOtaStatus[64] = {}` — cached OTA status string (fixed buffer, no heap)
  - `std::atomic<bool> _pendingOtaComplete{false}` — flag: OTA completed
  - `std::atomic<bool> _pendingOtaSuccess{false}` — cached OTA success result
  - `std::atomic<bool> _pendingError{false}` — flag: system error to display
  - `char _pendingErrorTitle[32] = {}` — cached error title
  - `char _pendingErrorMessage[128] = {}` — cached error message
  - `std::atomic<bool> _pendingMessage{false}` — flag: display message to show
  - `char _pendingMessageText[128] = {}` — cached message text

- [ ] Rewrite `_onOtaProgress()` (`LVGLDisplayModule.cpp`) to **only** store flags + `strncpy` strings. Remove ALL LVGL API calls (`showOtaProgress`, `restorePriorScreen`, `showError`). Remove the **`lv_timer_handler()`** call entirely. Set `_pauseRendering.store(true)` for in-progress, leave it for tick to clear on completion.

- [ ] Rewrite `_onDisplayMessage()` to only store `_pendingMessage` flag + `strncpy` the message. Remove `reportStatus()` call.

- [ ] Rewrite `_onSystemError()` to only store `_pendingError` flag + `strncpy` title/message. Remove `showError()` call. **Also fix the type confusion bug:** remove the `else` branch that reinterprets `SystemErrorPayload*` as `const char*` — standardise on `SystemErrorPayload` only.

- [ ] In `tick()`, add a pending-state drain section **after** screensaver tick but **before** the `_pauseRendering` check and `lv_timer_handler()`:
  ```
  1. Check _pendingOtaComplete (exchange false) → clear _pauseRendering, call restorePriorScreen or showError, resetActivity
  2. Else check _pendingOtaUpdate (exchange false) → call showOtaProgress, resetActivity
  3. Check _pendingError (exchange false) → call showError, resetActivity
  4. Check _pendingMessage (exchange false) → call reportStatus, resetActivity (see minor fix below for message semantic)
  ```
  **Order matters:** OTA complete must drain before OTA update, and both before error/message.

### 🔴 Fix #2 — Screenshot capture via `lv_snapshot_take` (MERGE BLOCKER)

- [ ] In `modules/LVGLDisplay/lv_conf.h`, add `#define LV_USE_SNAPSHOT 1` in the HAL section.

- [ ] In `LVGLDisplayModule.cpp`, replace the `memset(outBuffer, 0, outSize)` in `captureSnapshot()` with an actual LVGL snapshot:
  - Get active screen: `lv_obj_t* screen = lv_screen_active();`
  - Use LVGL v9's snapshot API to render the screen into the output buffer. The exact LVGL v9.5 API you should try is `lv_snapshot_take_to_buf()`. If that API doesn't exist or has a different signature, check `lv_snapshot.h` in the downloaded LVGL library (`.pio/libdeps/crowpanel-128-rotary/lvgl/src/others/snapshot/`).
  - If the snapshot call fails or the screen is null, `memset(outBuffer, 0, outSize)` as fallback.
  - The buffer format must remain RGB565 to match the existing `X-Display-Format` header and MCP tool response.

- [ ] In `test/stubs/platform/lvgl.h`, add a stub for `lv_snapshot_take_to_buf()` (or whatever API you use) that writes zeroes and returns success. Also add `#define LV_USE_SNAPSHOT 1` if the stub needs it.

### 🟠 Fix #3 — `std::atomic` for `_pauseRendering`

- [ ] In `LVGLDisplayModule.hpp`, change `bool _pauseRendering = false;` to `std::atomic<bool> _pauseRendering{false};`.
- [ ] Update all reads/writes in `LVGLDisplayModule.cpp` to use `.store()`/`.load()` where accessing from callbacks (already covered by #1 rewrite), and direct reads in `tick()` are fine with `std::atomic`.

### 🟠 Fix #4 — Remove U8g2 lib_dep from crowpanel env

- [ ] In `platformio.ini`, remove the `olikraus/U8g2@^2.36.18` line from `[env:crowpanel-128-rotary]`'s `lib_deps`.

### 🟡 Minor: `display.message` semantic fix

- [ ] In the pending-message drain in `tick()` (from #1), do NOT call `showBoot()` / `reportStatus()` if `_bootstrapMode` is `false`. Instead, just log the message (use `ESP_LOGI` or framework logger) and skip the screen switch. This prevents runtime EventBus messages from hijacking the boot screen. Comment: `// TODO: Add transient notification overlay widget in a future PR`.

### 🟡 Minor: `tick()` _lastTickMs == 0 sentinel

- [ ] Add `bool _tickInitialized = false;` member to `LVGLDisplayModule.hpp`.
- [ ] Replace `if (_lastTickMs == 0)` in `tick()` with `if (!_tickInitialized)`, and set `_tickInitialized = true;` inside that block.

### 🟡 Minor: Base64 transient allocation comment

- [ ] In the `capture_display` MCP tool handler (inside `registerMCPTools`), add a comment near the base64 encoding section: `// Note: ~150 KB transient PSRAM allocation for base64 encoding (48-byte chunks, 48 % 3 == 0)`.

## Verification Requirement
- [ ] `env:native` tests pass (`run_tests`) — 487+ cases, no regressions. If you added new test cases for pending-state drain, even better.
- [ ] `env:esp32s3nano` builds green (`build_project`).
- [ ] `env:crowpanel-128-rotary` builds green (`build_project`) — confirms U8g2 removal + `LV_USE_SNAPSHOT` + snapshot API compile.

## Completion Protocol
1. When implementation is validated, **STOP and send a message back to the Principal Architect** reporting results and any issues. Surface any out-of-boundary findings.
2. **Do not wrap up or distill until the human explicitly approves.**
3. Once approved: invoke the `@distillery` skill, passing `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo12-review-fixes/` as the target directory.
4. Append a status update to this `WORKORDER.md` and terminate your session.
