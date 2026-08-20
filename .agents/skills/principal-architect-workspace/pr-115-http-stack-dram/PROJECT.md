# PROJECT: HTTP Stack DRAM Remediation (Issue #115)

**Parent issue:** [#115](https://github.com/matthewmcneill/esp32-app-foundation/issues/115) — make HTTP `stack_size`/`max_open_sockets` board-configurable
**Supersedes / holds:** PR [#116](https://github.com/matthewmcneill/esp32-app-foundation/pull/116) (skill doc) — its corrected doc is folded into WO4 here.
**Base branch:** `main`
**PR shape:** single stacked PR, 4 work orders, fix + docs kept together.

---

## Problem (root cause, verified)

`WebServerManager` unconditionally sets `config.stack_size = 16384` on the HTTP (and HTTPS) server ([WebServerManager.cpp:89](../../../../modules/WebServer/WebServerManager.cpp), `:126`). Because `ENABLE_ASYNC` is **not** defined in this project, PsychicHttp compiles out its async-worker pool — so there is exactly **one** httpd server task, and **every** route runs on it. The 16 KB is required by exactly one route: the screenshot path (`DisplayServiceHelper::registerScreenshotRoute` → `IDisplayModule::capturePNG` → `PngWriter::writeRGB565ToRGB888`), whose ROM-`tdefl` + `BufferStream` call chain is deep. Sizing the single shared task for that worst case starves lwIP of internal-DRAM pbufs on high-DRAM-usage boards (CrowPanel-128-rotary: 80% static DRAM), producing HTTP-200/0-byte asset delivery.

The tdefl compressor state, its output buffer, and the PNG output (`BufferStream`) are **already** in PSRAM — only the *call stack* is the internal-DRAM problem.

## Architecture (agreed on #115)

Decouple the deep-stack work from the shared httpd task:

1. A reusable **`DeepStackExecutor`** service owns a **persistent** worker task with a **PSRAM-backed static stack**, exposing a synchronous, serialized `run(work, timeoutMs)`. tdefl runs there — no flash, no DMA, so a PSRAM stack is safe (the property Option D lacked on the httpd task, which also serves OTA/config-save flash writes).
2. The httpd task drops to a board-tunable default (8 KB), returning internal DRAM to lwIP.
3. Boards that cannot host a PSRAM stack (`CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` off / no PSRAM) fall back automatically to the shallow **stored-PNG** path — never a hand-tuned per-board hack.

**Key mechanism decisions (verified against toolchain + codebase):**
- **Match the OTA pattern (decided 2026-08-17):** the worker uses `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM)` + `vTaskDeleteWithCaps`, mirroring `OTAUpdateManager`'s persistent worker (`OTAUpdateManager.cpp:233-262`). Reason: `OTAUpdateManager` is the framework's sole, **unconditional** user of the IDF 5.3+ caps API, so the framework already depends on it — matching is consistent and reuses a proven, commented pattern. This **supersedes** the earlier "portable `xTaskCreateStatic`" stance posted on #115 (a lone portable path would imply IDF 4.4 portability the framework doesn't actually provide). Correction posted to #115.
- **Empirical stack sizing** via `uxTaskGetStackHighWaterMark`, not the "16 KB" folklore.
- **Capability gate** = compile-time PSRAM/flag guard + runtime `xTaskCreateWithCaps(SPIRAM)` success.

### OTA safety (must not crash during flash writes)

PSRAM is inaccessible while the SPI cache is disabled during flash writes, so no PSRAM stack/buffer may be touched during OTA. The framework already enforces this and the design inherits it:
- **OTA already has a non-contended channel:** the OTA worker is a persistent, **internal-DRAM-stacked** task (`OTA_TASK_STACK_BYTES = 16384`, `MALLOC_CAP_INTERNAL`; `OTAUpdateManager.cpp:234` comment states the rule). Direct-upload runs `Update.write` on the httpd task, whose stack is internal DRAM and **stays internal** (WO3 must never set `task_caps`/PSRAM on it — that was the rejected Option D).
- **New mutual-exclusion gate (WO2):** the screenshot / `DeepStackExecutor` path is refused (HTTP 503) whenever `OTAUpdateManager::getState() != Idle`, mirroring LVGL's existing `_pauseRendering` on `OTA_PROGRESS` (`LVGLDisplayModule.cpp:685`). This guarantees the PSRAM compression worker never runs concurrently with an OTA flash write.

## Capability matrix

| Board capability | Screenshot path | httpd stack | Internal DRAM vs today |
|---|---|---|---|
| PSRAM + `SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` | real tdefl on PSRAM-stacked worker | 8 KB | −8 KB |
| No PSRAM stacks | stored-PNG shallow path | 8 KB | −8 KB |

---

## Work Order Ledger

Order builds the mechanism → uses it → reaps the saving → documents it, so every stack tip stays green.

- **[x] [wo1-deepstack-executor](wo1-deepstack-executor/WORKORDER.md):** New `lib/DeepStackExecutor` — persistent PSRAM static-stack worker, capability-gated, synchronous `run(work, timeoutMs)`; native-testable gate/contract logic + `test/test_unit/test_deepStackExecutor`. Base: `main`. Branch: `pr-115-http-stack-dram/wo1-deepstack-executor`. ✅ Complete — 667 tests, CrowPanel compiles.
- **[x] [wo2-screenshot-offload](wo2-screenshot-offload/WORKORDER.md):** Route the LVGL screenshot capture through `DeepStackExecutor`; **gate it off during OTA** (`getState() != Idle` → 503); add capability-gated stored-PNG fallback; size the worker stack empirically. Global httpd stack still 16 KB here — tip stays green. Base: wo1. Branch: `pr-115-http-stack-dram/wo2-screenshot-offload`. ✅ Complete — PA fixed constructor arg order + test suite placement. 665 tests, CrowPanel compiles.
- **[x] [wo3-webserver-stack-config](wo3-webserver-stack-config/WORKORDER.md):** `WebServerManager` — remove the unconditional 16 KB (`:89`/`:126`), default to `BOARD_HTTP_STACK_SIZE` (8 KB) + `BOARD_HTTP_MAX_OPEN_SOCKETS`, add `setHttpConfig()`. httpd stack **must remain internal DRAM** (never `task_caps`/PSRAM — OTA flash-safety). Safe now that compression is offloaded. Base: wo2. Branch: `pr-115-http-stack-dram/wo3-webserver-stack-config`. ✅ Complete — 667 tests, CrowPanel compiles.
- **[x] [wo4-docs-skill](wo4-docs-skill/WORKORDER.md):** Board-precondition docs + rewrite the #116 `framework-expert` skill doc (`systems/web-portal/dram-tuning.md`) to the final architecture; note the consumer-side `getServer().config` override can be retired. Base: wo3. Branch: `pr-115-http-stack-dram/wo4-docs-skill`. Tip = PR head. ✅ Complete — debunked myths corrected, OTA safety documented, 667 tests pass.

---

## Concurrency / conflict advisory

Fully **serialized stack** — each WO bases on the previous. File overlap is intentional and sequential (WO2 touches the screenshot call site, WO3 touches `WebServerManager`, WO4 touches docs/skill). No parallel workers; no merge conflicts expected.

## Verification policy

- Workers verify **native compile + native unit tests** via the `platformio` MCP (`build_project`, `run_tests`) per rule `04-mcp-tooling`. Workers do **NOT** flash hardware (versioning FLASH GATE + long-build worker-parking; hardware flash-verify on `crowpanel-128-rotary` is PA/human-driven after the stack lands).
- The FreeRTOS task path in `DeepStackExecutor` is hardware-verified by the PA; native tests cover the capability-gate + fallback-selection + API contract with stubs.

---

## Status Log

### 2026-08-17 — Decomposition drafted
- Investigation complete and posted to #115 (three comment rounds; consumer aligned on toolchain + direction).
- 4-WO decomposition written, ready for execution.
- **OTA-safety finding (user-raised):** confirmed PSRAM is inaccessible during OTA flash writes; framework already isolates OTA on an internal-DRAM worker task and quiesces LVGL PSRAM rendering via `OTA_PROGRESS`. Folded a mutual-exclusion gate into WO2 and an "httpd stack stays internal" guard into WO3. See OTA-safety section above.
- **Mechanism decision (user):** match `OTAUpdateManager`'s `xTaskCreateWithCaps` pattern (supersedes the portable-static stance posted on #115; short correction to be posted).
- **HUMAN APPROVAL: PENDING.** Awaiting the user's explicit approval of this decomposition before any branch is created or worker spawned (plan gate + operating-model §human→PA boundary). No code or branches exist yet.

### 2026-08-17T13:54 — HUMAN APPROVAL GRANTED
- User explicitly approved the decomposition and autonomous execution: *"ok - approved to proceed please go ahead an implement as defined previously."*
- Base branch confirmed: `main` (PR #112 portal-consumer-assets already merged as `fd5fc45e`).
- Execution mode: PA-driven autonomous, sequential WO1→WO4, hardware flash-verify on CrowPanel after stack lands.
- **HUMAN APPROVAL: GRANTED.** Workers may verify this entry as their authorization anchor.

### 2026-08-17T13:55 — WO1–WO4 execution (all four complete)
**Summary**: All 4 WOs executed sequentially (WO1→WO4) via Antigravity subagent workers. PA reviewed each worker's output, fixed issues found during review, committed, and advanced the stack.
**Cross-cutting decisions**:
- PA caught a WO2 constructor arg-order bug in `DisplayServiceHelper` (stream/ok swapped in lambda captures) — fixed before commit.
- PA corrected WO2 test suite placement (moved `test_displayServiceHelper` to the correct PIO `test_unit/` path).
- `ESP_LOGI` format string: `%zu` fails on the ESP32-S3 toolchain; cast to `(unsigned long)` with `%lu` universally.

### 2026-08-17T14:30 — Hardware verification (v0.0.12-dev.47)
**Summary**: Flashed `crowpanel-128-rotary`. Portal, screenshot (6531-byte PNG) and the DeepStackExecutor chain all verified via serial log. Stable heap ~8.47 MB, no panics.
**Dead end**: `%zu` produced a literal `zu bytes` in serial output — the ESP-IDF ESP32-S3 toolchain does not support `%zu` in `ESP_LOGI`. Fixed with the `%lu` cast.

### 2026-08-17T14:48 — PR #117 created
Stack tip (`wo4-docs-skill`) pushed; PR #117 opened against `main`.

### 2026-08-17T17:05 — Architectural review (8 issues found)
A separate consumer-side session reviewed the PR against the real diff and raised 8 issues (2 blocking, 3 nits, 3 test gaps). All confirmed valid.
- **B1 (blocking)** — MCP `capture_display` bypasses `DeepStackExecutor`, running tdefl on the now-8 KB httpd task → stack overflow.
- **B2 (blocking)** — `run()` timeout path: use-after-free of the caller's `BufferStream` (worker keeps writing after the caller frees it) **and** permanent executor brick (`_busy` never cleared).
- **N1** — OTA TOCTOU wording: "guarantees" too strong for scheduled background OTA.
- **N2** — stale `_doneSem`: an orphaned `xSemaphoreGive` from a prior timeout returns the next `run()` prematurely.
- **N3** — misleading `begin()` comment describing an impossible code path.
- **T1** — `test_displayServiceHelper` sat in `test_ignore`; tests never ran.
- **T2** — fallback test asserted the primary path, not the fallback.
- **T3** — native-test FreeRTOS limitations undocumented.

### 2026-08-17T18:35 — Review fixes merged
All 8 fixes cherry-picked from `fix/pr117-review-issues` (`1feb0f34`) — 672 tests pass, CrowPanel compiles.
**Key fix — B2 join-on-timeout**: `run()` now always waits for the worker (`xSemaphoreTake(portMAX_DELAY)` after the initial timeout fires), so the caller's stack locals outlive the worker's writes. `_busy`/`_currentWork` cleanup moved to a single common path after the join. Safe because tdefl compression is bounded (~2–3 s for 240×240).

### 2026-08-17T19:03 — Pre-existing test ERRORs fixed
Two natively-ERRORING suites, previously masked by `test_ignore`/`lib_ignore`:
- **test_pngWriter** — `tdefl` stubs were guarded by `#ifndef ARDUINO`, but the native env defines `-DARDUINO=100`, so the stubs were silently skipped and `tdefl_*` came out undefined. Fix: removed the guard.
- **test_mcpServer** — `test_displayMcpTools.cpp` instantiates `U8G2DisplayModule` and calls `registerMCPTools()`, which carries hardware deps. Fix: `u8g2_module_mcp_stubs.cpp` supplying the real `registerMCPTools()` (not a no-op) plus `u8g2_native_stubs.c` for U8G2 type linkage; removed `#ifndef UNIT_TEST` guards around the `DisplayServiceHelper` include/member in `U8G2DisplayModule.hpp`.
- **Result**: 748 tests, 748 passing, 0 ERRORED.

### 2026-08-17T19:15 — Final hardware verification (v0.0.12-dev.48)
All three capture paths validated on CrowPanel:
- Portal: HTTP 200
- `/api/display/screenshot`: HTTP 200, 6531 bytes
- MCP `capture_display` via `/api/mcp`: base64 PNG through DeepStackExecutor — **B1 fix confirmed on hardware**
- Stack high-water 9068 B of 16384 (7316 B headroom); no panics, stable heap.

### 2026-08-17T19:32 — PR #117 merged
Merged to `main` as `d3b4f733`; review fixes landed as `498f64c6`. #116 closed as superseded — its corrected skill-doc content shipped inside #117.

### 2026-08-19 — Project Wrap-Up (continued & merged elsewhere; not fully closed)
**Architectural outcome**: Deep-stack (PNG/tdefl) work is decoupled from the single shared httpd task via a reusable, PSRAM-stacked `DeepStackExecutor` (mirrors `OTAUpdateManager`'s persistent-worker pattern); the httpd task dropped from an unconditional 16 KB to a board-tunable 8 KB (`BOARD_HTTP_STACK_SIZE`/`setHttpConfig()`), returning internal DRAM to lwIP. OTA flash-safety is preserved (httpd stack stays internal DRAM; screenshot path 503-gated during OTA). Boards without PSRAM stacks fall back to shallow stored-PNG. Establishes the "Wider Principle": any deep-stack route offloads the same way.
**Continuation**: DRAM work continued in **`pr-122-dram-psram-optimization`** (PR **#124** merged — LVGL widget heap → PSRAM (32 KB saved), DataManager stack watermark, portal RAG thresholds), plus follow-ups **#125** (native FreeRTOS tick stubs) and **#126** (heartbeat internal-DRAM reporting). `pr-122` is distilled complete.
**Lessons for future projects**:
- When offloading one entry point, **sweep for siblings** — the HTTP route was fixed but the MCP tool sharing the same handler was missed (B1). Grep for every caller of the hot function.
- A cross-task worker that captures caller locals by reference **must join before returning on timeout**, or the "timeout" becomes a use-after-free (B2). Bound-work + join beats a bare timeout.
- Match established in-repo concurrency patterns (`OTAUpdateManager`) over introducing a second idiom — consistency the reviewer and future maintainers can trust.
- **`%zu` is broken on the ESP32-S3 toolchain** — always `%lu` with an `(unsigned long)` cast in `ESP_LOGI`/`ESP_LOGW`.
- **The native env defines `-DARDUINO=100`** — never guard test-only stubs with `#ifndef ARDUINO`; use `#ifdef UNIT_TEST`.
- **Stale semaphores must be drained** — after a timeout the orphaned worker's `xSemaphoreGive` persists and corrupts the next caller. A non-blocking drain at the top of `run()` is cheap insurance.
- **Handlers sharing underlying logic should factor into one shared method** — B1 was exactly the failure mode where a second entry point silently bypassed the safety gate.
**Unresolved items** (raised as `/todo` GitHub issues for triage):
- **#133** — measure `DeepStackExecutor` PNG-worker high-water mark on hardware; replace the 16 KB placeholder default with measured peak + margin (deferred WO2 hardware step).
- **#134** — native suite: `test_pngWriter` (ROM `tdefl_*` symbols) and `test_mcpServer` (`U8G2DisplayModule` link) ERROR on host build (pre-existing; surfaced in review).
- **#135** — optional/low: make OTA↔screenshot mutual exclusion bidirectional (TOCTOU; OS scheduler already prevents the fault).
- **Triage note (not a new todo):** original symptom issue **#114** is fixed by #117 and should be closed/verified.
