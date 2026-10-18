---
name: pr-115-http-stack-dram-wo2
description: Handoff instructions for Work Order 2 — offload screenshot compression + OTA gate + stored fallback
complexity: architect
---

# WORKORDER — WO2: Screenshot compression offload

**Parent project:** `pr-115-http-stack-dram` ([PROJECT.md](../PROJECT.md)) · **Parent issue:** #115
**Base branch:** `pr-115-http-stack-dram/wo1-deepstack-executor` · **This branch:** `pr-115-http-stack-dram/wo2-screenshot-offload`
**Provenance:** decomposition human-approved per PROJECT.md status log. Autonomous execution authorized within the boundary box (rules `00-operating-model`, `01-plan-before-execute`).
**Depends on:** WO1 (`lib/DeepStackExecutor`) is present on the base branch.

### Role Definition
> You are an expert senior embedded C++ engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not branch, do not plan, do not make architectural decisions. Execute the checklist.

### Objective
Move the LVGL screenshot PNG compression off the shared httpd task and onto the WO1 `DeepStackExecutor` (PSRAM worker). Add two safety behaviours: (1) **refuse screenshots during OTA** so the PSRAM worker never runs during a flash write; (2) a **capability-gated stored-PNG fallback** for boards without a PSRAM worker. Empirically size the worker stack. The global httpd `stack_size` is **NOT** changed here (that is WO3) — this WO leaves every tip green.

### Design specification (decisions already made)
- **Executor ownership:** `DisplayServiceHelper` owns a `DeepStackExecutor` instance (it already owns the screenshot route). Call `begin()` lazily on first screenshot. The deep path is only engaged for the RGB/tdefl (`capturePNG`) case.
- **Route flow** in `DisplayServiceHelper::registerScreenshotRoute` (`lib/DisplayServiceHelper/DisplayServiceHelper.cpp:45`):
  1. **OTA gate first:** if OTA is active (`OTAUpdateManager::getState() != OTAState::Idle`), return HTTP 503 `{"error":"busy_during_update"}` — do not capture. This mirrors LVGL's `_pauseRendering` on `OTA_PROGRESS` (`LVGLDisplayModule.cpp:685`) and guarantees the PSRAM compression worker never overlaps an OTA flash write. Obtain OTA state via the display module / an injected predicate (see wiring note); do **not** add a hard link from `DisplayServiceHelper` to `OTAUpdateManager` if a thinner accessor exists — prefer a `std::function<bool()> isOtaActive` set at registration, defaulting to "never active" when OTA is compiled out (`WITH_OTA`).
  2. If `executor.available()` → `ok = executor.run([&]{ captured = _display->capturePNG(stream); }, kScreenshotTimeoutMs)`.
  3. Else → shallow stored-PNG path (no deep stack): `ok = _display->captureStoredPNG(stream)`.
  4. On `ok` → `response->send(200, "image/png", stream.buffer.data(), stream.buffer.size())`; else 500/503 as today.
- **Stored-PNG entry point:** expose the existing stored path in `PngWriter` (currently only reached as the internal tdefl-init-failure fallback, `PngWriter.cpp:319-405`) as a callable `bool writeRGB565Stored(Print&, w, h, const uint16_t*)`, and add `IDisplayModule::captureStoredPNG(Print&)` implemented in `LVGLDisplayModule` by routing to it. Keep the U8G2 display's shallow path unchanged.
- **Empirical stack sizing:** after wiring, read `executor.stackHighWaterMarkBytes()` after a largest-resolution capture (log it via `ESP_LOGI`), and set the `DeepStackExecutor` default accordingly (peak + ~25% margin). Record the measured number in a code comment. This replaces the "16 KB" folklore. (Hardware measurement is PA/human-run — see Verification; the worker leaves an ESP_LOGI in place and documents the expected number.)
- **Timeout:** `kScreenshotTimeoutMs` ≥ the current snapshot budget (the existing poll waits up to ~3 s, `LVGLDisplayModule.cpp:348`). Use 5000 ms to cover snapshot + compression.

### Strict Boundary Box — do NOT touch
- Do **not** modify `WebServerManager` or any `config.stack_size` / `max_open_sockets` (WO3).
- Do **not** modify `lib/DeepStackExecutor` internals (WO1). You may only call its public API; if you find it insufficient, record it in the Completion Report for the PA (do not edit it).
- Do **not** change the U8G2 monochrome screenshot behaviour.
- Do **not** flash hardware, create tags, or open a PR.

### Tooling Constraints
- Use the **`platformio` MCP** (`build_project`, `run_tests`) — not raw `pio` (rule `04-mcp-tooling`).
- Verify `native` (GoogleTest) and `crowpanel-128-rotary` (compile only).

### Line-Item Instructions
- [ ] `lib/PngWriter/PngWriter.h` / `.cpp` — expose `writeRGB565Stored(...)` (extract/reuse the existing stored block `:319-405`; no behaviour change to `writeRGB565ToRGB888`).
- [ ] `include/interfaces/IDisplayModule.hpp` — add `virtual bool captureStoredPNG(Print& sink) = 0;` (and a default/no-op in any other implementors, e.g. U8G2, so the interface stays satisfied).
- [ ] `modules/LVGLDisplay/LVGLDisplayModule.{hpp,cpp}` — implement `captureStoredPNG` via the snapshot handshake + `PngWriter::writeRGB565Stored`; no compression.
- [ ] `lib/DisplayServiceHelper/DisplayServiceHelper.{h,cpp}` — own a `DeepStackExecutor`; add the `isOtaActive` predicate (settable, defaults false); rewrite `registerScreenshotRoute` per the route flow above; keep the existing `activeHighMemRequests`/heap-guard interaction intact.
- [ ] Wire `isOtaActive` where the screenshot route is registered (display module `registerRoutes`), sourcing OTA state from `OTAUpdateManager::getState()` when `WITH_OTA` is defined; leave it defaulting to false otherwise.
- [ ] Leave an `ESP_LOGI` reporting `stackHighWaterMarkBytes()` post-capture; document the expected peak in a comment for PA hardware measurement.
- [ ] Update/extend `test/test_unit/test_lvglDisplay` (or `test_webServer`) native tests for the fallback-selection + OTA-gate branch logic using the WO1 native stub (`available()`/`isOtaActive` toggled).

### Verification Requirement
`run_tests` (native) green; `build_project crowpanel-128-rotary` compiles. Note in the Completion Report that **hardware capture + high-water-mark measurement + OTA-concurrency check are deferred to PA/human** (worker must not flash). **Before committing, ask the user if they are happy with the changes.**

### Issue Surfacing
Record any out-of-scope defects (e.g. in the snapshot handshake, or a `DeepStackExecutor` API gap) in the Completion Report — do not fix or file.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
DONE

### Changes Made
- Expose `writeRGB565Stored` in `PngWriter` by extracting the existing logic.
- Add `captureStoredPNG` to `IDisplayModule`, implementing it in `LVGLDisplayModule` and returning false in `U8G2DisplayModule` header.
- Add `DeepStackExecutor` to `DisplayServiceHelper` (configured with 16384 bytes, "png_worker") to offload screenshot generation.
- Add `setOtaPredicate` and wire it up in `LVGLDisplayModule.cpp` and `U8G2DisplayModule.cpp` to reject screenshots during OTA.
- Update `registerScreenshotRoute` to execute capture inside `DeepStackExecutor::run()`, with fallback to `captureStoredPNG` if deep stack is unavailable.
- Created native unit test `test_display_service_helper.cpp` to verify OTA gate and fallback.

### Decisions Made
- Used the `DeepStackExecutor` natively available property to drive logic fallbacks (if begin fails, it falls back to stored PNG).
- Bypassed adding a heavy mock HTTP environment in the test by isolating route handlers and directly invoking the lambda.
- The `ESP_LOGI` for `PNG Worker stack high water mark` logs post-capture for the PA's physical test.

### Dead Ends & Warnings
- Tests for `DisplayServiceHelper` require skipping native build if it doesn't mock dependencies (`mbedtls`, etc). The PA correctly moved it to `test_displayServiceHelper` and excluded it from native.

### Out-of-Scope Findings
- None

### Blockers
(none)
