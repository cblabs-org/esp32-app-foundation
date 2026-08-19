---
name: issue-sweep-aug-wo5
description: Handoff instructions for Work Order 5 — Skill/doc updates, PSRAM audit, EventBus payload lifetime, SLS naming, sample-app test fix
complexity: standard
---

# WO5 — Skill/Doc Updates, PSRAM Audit, EventBus Payload Lifetime, SLS Naming, Sample-App Test Fix

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Five deliverables bundled together: (1) add `LV_CONF_PATH` consumer warning + **PSRAM-preference audit checklist** to framework-expert skill, (2) **document the EventBus publisher-side payload lifetime rule**, (3) fix SLS screen naming inconsistency, (4) fix sample-app native test link failure. Closes **#127**, **#130**, **#97**, **#113**. Also delivers the PSRAM auditing intent from closed PR #116.

## Strict Boundary Box
- **ONLY** modify the files listed below.
- Do NOT modify the framework C++ source code **except** `EventBus.hpp` documentation comments (this WO is skill/doc/test/SLS only, plus one header doc update).
- For the SLS rename, only change the `.spj` file — do NOT re-export the generated C files (that requires SquareLine Studio).
- For the test fix, do NOT change `WeatherModule.cpp` or `WeatherModule.hpp` source code.
- Do NOT change `EventBus.cpp` or any EventBus dispatch mechanics — only documentation comments in the header.

## Tooling Constraints
- Use `platformio` MCP tools for testing (rule `04-mcp-tooling.md`).
- For #113, verify with: `run_tests` environment `native` on the sample-app project (path: `sample-app/`).

## Line-Item Instructions

### #127 + PR #116 Intent — framework-expert Skill: PSRAM Guidance & Audit

The framework-expert skill currently has **zero** mentions of `LV_CONF_PATH`, `lv_conf.h`, or PSRAM-preference auditing. This deliverable adds a unified PSRAM guidance section that agents will use when reviewing or building **any** module on a PSRAM-equipped board.

- [ ] **`.agents/skills/framework-expert/performance/psram.md`**
  Add **two** new sections to this file:

  **Section 1 — Consumer Warning: `LV_CONF_PATH` Override**

  ```markdown
  ## Consumer Warning: `LV_CONF_PATH` Override

  > [!WARNING]
  > **If your project sets `-DLV_CONF_PATH` to its own `lv_conf.h`, the framework's
  > LVGL memory configuration does not apply to your build.** Framework updates to
  > `modules/LVGLDisplay/lv_conf.h` — including the PSRAM pool placement — will
  > never reach you. You must mirror them in your own file.

  In particular, ensure `LV_MEM_SIZE` is PSRAM-backed via `LV_MEM_POOL_ALLOC`; on a
  PSRAM board an internal-DRAM LVGL pool is likely to be your **single largest DRAM
  consumer** (measured: 128 KB on one consumer project, reducing `heap_min_free` to
  1,004 bytes).

  ### Diagnostic Recipe — Finding Internal-DRAM Hogs

  ```bash
  xtensa-esp32s3-elf-nm -C --size-sort -r -S firmware.elf | grep -iE "^3f[cd]"
  ```

  Internal DRAM lives at `0x3FC…`/`0x3FD…`; `0x3C…` is flash-mapped rodata and costs
  no RAM. The naive `nm` output is dominated by images and fonts that cost nothing —
  this filter isolates the real DRAM consumers.

  ### After Bumping the Submodule

  After upgrading the framework submodule, always check whether any `lv_conf.h` change
  needs mirroring into your own override. The changelog and PR description will note
  LVGL memory changes, but the `LV_CONF_PATH` override makes them invisible to your
  build unless you manually apply them.
  ```

  **Section 2 — PSRAM-Preference Audit Checklist**

  This is the key new deliverable — a concrete checklist for agents auditing any module's
  memory allocation strategy. The guiding principle: **prefer PSRAM for everything possible,
  preserve internal DRAM for the things that cannot avoid it.**

  ```markdown
  ## PSRAM-Preference Audit

  When reviewing or building any module on a PSRAM-equipped board, apply this checklist.
  The principle is simple: **internal DRAM is scarce and non-expandable — use PSRAM for
  everything that can tolerate its slightly higher latency, and reserve DRAM for the
  things that physically require it.**

  ### Mandatory PSRAM Candidates
  - [ ] **Large buffers (>4 KB)** — image buffers, JSON documents, TLS scratch,
        compression workspace → allocate with `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`
  - [ ] **LVGL widget heap** — `LV_MEM_POOL_ALLOC` must point to PSRAM
        (see LV_CONF_PATH warning above)
  - [ ] **Task stacks for non-flash-critical tasks** — use
        `xTaskCreateWithCaps(..., MALLOC_CAP_SPIRAM)` for worker tasks that
        don't touch SPI flash (e.g. DeepStackExecutor, data processing)
  - [ ] **String/JSON accumulation buffers** — ArduinoJson `JsonDocument`,
        `String` concatenation for large payloads

  ### Must Stay in Internal DRAM
  - [ ] **httpd task stack** — OTA direct-upload and LittleFS writes execute here
  - [ ] **OTA worker task stack** — flash write routines
  - [ ] **DMA buffers** — SPI, I2S, LCD DMA descriptors
  - [ ] **ISR-accessible data** — anything touched from an interrupt context
  - [ ] **lwIP pbufs** — TCP/IP stack requires internal DRAM

  ### Red Flags (Audit Triggers)
  - `malloc()` or `new` for buffers >4 KB without `MALLOC_CAP_SPIRAM`
  - `LV_MEM_SIZE` set to >64 KB without `LV_MEM_POOL_ALLOC`
  - Task stacks >8 KB in internal DRAM for non-flash-critical work
  - Static arrays >4 KB in `.bss` or `.data` segments
  - Any module allocating >10% of available internal DRAM at init
  ```

- [ ] **`.agents/skills/framework-expert/performance/heap-management.md`**
  Add a cross-reference note pointing to the new `psram.md` sections:

  ```markdown
  > **See also:** [PSRAM — LV_CONF_PATH Override Warning & PSRAM-Preference Audit](psram.md)
  > — if your project overrides `LV_CONF_PATH`, framework LVGL memory changes do not
  > apply to you. The PSRAM-Preference Audit checklist provides a systematic review
  > for any module's memory allocation strategy.
  ```

- [ ] **Check for KEEPING-CURRENT or upgrade guide** in the framework-expert skill or `docs/`. If a submodule upgrade guide exists, add a one-line caveat: *"After bumping the submodule, check whether any `lv_conf.h` change needs mirroring into your own override."* If no such guide exists, note this in your Completion Report.

### #130 — EventBus Publisher-Side Payload Lifetime Documentation

The EventBus contract (§5) documents the **subscriber-side** rule ("payload pointers are valid ONLY for the duration of the callback") but never states the **publisher-side** obligation: on target, dispatch is async, so the payload must remain valid from `publish()` until the callback executes — which is *after* `publish()` has returned.

Five framework modules independently invented ring buffers or persistent members to keep payloads alive. A consumer reading the contract has no way to infer this requirement.

- [ ] **`modules/EventBus/EventBus.hpp`** (lines 68–72, §5)
  Add the publisher-side rule alongside the existing subscriber rule. Insert after line 71:

  ```cpp
   *    - PUBLISHER-SIDE: On target, dispatch is asynchronous (§1). The payload
   *      must remain valid from publish() until all subscribers have executed —
   *      which is AFTER publish() returns. Do NOT pass the address of a stack
   *      local. Use one of these patterns:
   *        (a) A persistent member variable (e.g. Application::_systemHeapPayload)
   *        (b) A ring buffer sized to the maximum publish cadence (e.g.
   *            ButtonModule::_eventRing, EncoderModule::_eventRing)
   *      On native (UNIT_TEST), dispatch is synchronous, so stack-locals appear
   *      to work — but the bug manifests only on target, intermittently.
  ```

  Also add a brief worked example after the contract block, or as a comment block after §6:
  ```cpp
   * Example — ring buffer pattern (recommended for tick-driven publishers):
   *
   *   MyEvent& evt = _eventRing[_ringHead];
   *   evt.field = value;
   *   _ringHead = (_ringHead + 1) % RING_SIZE;
   *   _bus->publish("my.topic", &evt);
   *
   * The ring slot remains valid until overwritten on the next wrap.
  ```

### #97 — SLS Screen Naming Consistency

- [ ] **`squareline/crowpanel-128/SquareLine_Project.spj`** (line 6404 area)
  Rename `"ui_DiagScreen"` → `"uiDiagScreen"` in all occurrences within the `.spj` file. Search for all instances of the string `ui_DiagScreen` and replace with `uiDiagScreen`. This includes:
  - The screen object `Name` property
  - Any child widget references that include `DiagScreen` in their name path

  **Important:** The `.spj` file is JSON. Ensure the replacement preserves valid JSON structure. Use a global find-and-replace of `ui_DiagScreen` → `uiDiagScreen` within the file.

  **Do NOT** re-export C files from SquareLine Studio — that requires the GUI tool. The C files in `modules/LVGLDisplay/ui/` will be out of sync until the next SLS export. Note this in your Completion Report.

### #113 — Sample-App Native Test Link Failure

- [ ] **`sample-app/test/test_unit/test_weather/test_main.cpp`** (line 23)
  The test does `#include "WeatherModule.cpp"` which pulls in `WeatherModule::begin(Application&)`, causing unresolved `Application::getEventBus()` / `getKeyManager()` / `getDataManager()` / `getConfigManager()` at link time.

  **Fix approach (choose one):**

  **Option A (preferred):** Guard the `begin()` method inclusion for native tests. Add a preprocessor guard in the test file so `begin()` is not exercised:
  ```cpp
  // Instead of #include "WeatherModule.cpp", include only the header
  // and provide stubs for the specific methods under test.
  #include "WeatherModule.hpp"
  // ... test only config/gate/metadata logic, not begin()
  ```

  **Option B:** Add the `test_weather` to the sample-app's native `test_ignore` if the test is not intended to run natively:
  ```ini
  ; in sample-app/platformio.ini [env:native]:
  test_filter = test_unit/* -test_unit/test_weather
  ```

  **Option C:** Create a minimal `Application` stub in `sample-app/test/stubs/` that provides the missing accessor symbols. Follow the pattern from the framework's own `test/stubs/`.

  Investigate which tests in `test_main.cpp` actually exercise `begin()` vs. config/metadata logic, and choose the approach that preserves maximum test coverage.

## Verification Requirement
1. If #113 fix is Option A or C: `pio test -e native` in `sample-app/` must pass.
2. If #113 fix is Option B: `pio test -e native` in `sample-app/` must pass (test_weather excluded).
3. `.spj` file must remain valid JSON after the rename.
4. **CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
_(pending)_

### Changes Made
_(pending)_

### Decisions Made
_(pending)_

### Dead Ends & Warnings
_(pending)_

### Out-of-Scope Findings
_(pending)_

### Blockers
_(pending)_
