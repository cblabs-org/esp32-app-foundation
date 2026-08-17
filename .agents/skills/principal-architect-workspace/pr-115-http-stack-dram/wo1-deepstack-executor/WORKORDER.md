---
name: pr-115-http-stack-dram-wo1
description: Handoff instructions for Work Order 1 — DeepStackExecutor component
complexity: architect
---

# WORKORDER — WO1: `DeepStackExecutor`

**Parent project:** `pr-115-http-stack-dram` ([PROJECT.md](../PROJECT.md)) · **Parent issue:** #115
**Base branch:** `main` · **This branch:** `pr-115-http-stack-dram/wo1-deepstack-executor`
**Provenance:** This PR's decomposition is human-approved per the PROJECT.md status log. You are authorized to implement this WORKORDER autonomously within the Strict Boundary Box below (rules `00-operating-model`, `01-plan-before-execute`).

### Role Definition
> You are an expert senior embedded C++ / FreeRTOS engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Execute strictly the checklist below.

### Objective
Create a reusable framework service, `DeepStackExecutor`, that runs an arbitrary callable on a **persistent worker task with a PSRAM-backed static stack**, so that deep-call-stack work (PNG compression today; TLS/image work later) is decoupled from the shared httpd task. This WO delivers the **standalone component + tests only** — no caller is rewired here (that is WO2).

### Design specification (decisions already made — implement, do not re-litigate)
- **Location:** new `lib/DeepStackExecutor/` (siblings: `lib/PngWriter`, `lib/DisplayServiceHelper`). Add `library.json` mirroring `lib/DisplayServiceHelper/library.json`.
- **Public API:**
  ```cpp
  class DeepStackExecutor {
  public:
      // stackBytes: worker stack depth. Default from BOARD_DEEPSTACK_BYTES (fallback 16384).
      explicit DeepStackExecutor(size_t stackBytes = /* BOARD_DEEPSTACK_BYTES */ 16384,
                                 const char* name = "deepstack");
      ~DeepStackExecutor();                    // deletes task, frees stack (see lifecycle note)
      bool begin();                            // lazily allocates PSRAM stack + creates worker; idempotent
      bool available() const;                  // true iff worker is running on a PSRAM stack
      // Runs `work` on the worker; blocks caller until done or timeout. Serialized (one job at a time).
      bool run(const std::function<void()>& work, uint32_t timeoutMs);
      size_t stackHighWaterMarkBytes() const;  // uxTaskGetStackHighWaterMark → bytes, for empirical sizing
      DeepStackExecutor(const DeepStackExecutor&) = delete;
      DeepStackExecutor& operator=(const DeepStackExecutor&) = delete;
  };
  ```
- **Mechanism (mirror `OTAUpdateManager`'s proven persistent-worker pattern — decided 2026-08-17):**
  - **Reference implementation:** `modules/OTAUpdateManager/OTAUpdateManager.cpp` (`begin()` task creation `:233-262`, `otaTaskLoop`, destructor `:93-105`, `xTaskNotifyGive` handoff). Follow the same structure; the only differences are the memory caps (`SPIRAM` here vs `INTERNAL` there) and the synchronous done-signal.
  - Task creation: `xTaskCreateWithCaps(worker, name, stackBytes, this, tskIDLE_PRIORITY + 1, &_taskHandle, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`. If it returns `!= pdPASS` → `begin()` fails, `available()` stays false (caller falls back — WO2). Do **not** fall back to an internal-DRAM stack (that reintroduces the DRAM regression this project exists to fix).
  - Rationale (recorded): the framework already standardizes on the caps API — `OTAUpdateManager` is the sole, unconditional user of `xTaskCreateWithCaps`/`vTaskDeleteWithCaps` — so matching it is consistent and reuses a commented, proven pattern. A `xTaskCreateWithCaps(SPIRAM)` stack is gated by `CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM`, which is the intended capability gate.
  - Job handoff: worker loop blocks (`ulTaskNotifyTake` or a "start" binary semaphore); `run()` stores the `std::function` in a member, notifies the worker, then waits on a "done" binary semaphore with `pdMS_TO_TICKS(timeoutMs)`.
  - **Serialization:** `run()` takes a FreeRTOS mutex for its whole duration — one job at a time (matches the portal's existing `MAX_CONCURRENT_HIGH_MEM = 1`). Concurrent callers block.
  - **Timeout semantics:** if "done" is not signalled in time, return `false` and mark the executor busy so a subsequent `run()` refuses (returns false) until the in-flight job completes — never reuse the job slot while the worker may still be on it. Document this in the header.
  - Worker priority: `tskIDLE_PRIORITY + 1` (below the httpd/tcpip tasks). Pin: `tskNO_AFFINITY`.
- **Capability gate:** `available()` = worker created successfully on a PSRAM stack. Guard the whole hardware path with `#ifndef UNIT_TEST` so the native build links (see PngWriter/BufferStream precedent).
- **Lifecycle note (persistent worker):** the worker is created once and lives for process lifetime; because it never self-deletes, there is no "free your own stack" hazard. The destructor (tests only) must `vTaskDeleteWithCaps(_taskHandle)` — **NOT** plain `vTaskDelete`, which leaks a caps-allocated stack (this exact bug is documented in `OTAUpdateManager.cpp:12-17`/`:93-105`).
- **House style:** follow the repo header/comment conventions (see `lib/PngWriter/PngWriter.h`, `.cpp`). SPDX + module banner + Doxygen on public methods.

### Strict Boundary Box — do NOT touch
- Do **not** modify `WebServerManager`, `DisplayServiceHelper`, `LVGLDisplayModule`, `PngWriter`, or any route. No caller is wired in this WO.
- Do **not** change `platformio.ini` build flags except: (a) register the new `lib/DeepStackExecutor` if the native test needs it, and (b) add it to the `native` env `lib_ignore` **only if** the FreeRTOS path can't link natively (mirror how `PngWriter` is handled — check `[env:native] lib_ignore`).
- Do **not** define `BOARD_DEEPSTACK_BYTES` in any board header yet (WO2 wires board defaults). Use the in-code fallback only.
- Do **not** flash hardware. Do **not** create tags. Do **not** open a PR.

### Tooling Constraints
- Use the **`platformio` MCP** server (rule `04-mcp-tooling`): `build_project` and `run_tests`, not raw `pio`.
- Verify the `native` env (GoogleTest) builds and the new unit test passes.
- Also verify the `crowpanel-128-rotary` env **compiles** (build only — no flash) so the FreeRTOS path is confirmed against the real toolchain.

### Line-Item Instructions
- [ ] `lib/DeepStackExecutor/DeepStackExecutor.h` — class declaration + full house-style docs, per the spec above.
- [ ] `lib/DeepStackExecutor/DeepStackExecutor.cpp` — implementation: `begin()`, `available()`, `run()`, `stackHighWaterMarkBytes()`, dtor. Hardware path under `#ifndef UNIT_TEST`; provide a native-safe stub path (see below) so callers and tests link on `native`.
- [ ] `lib/DeepStackExecutor/library.json` — mirror `lib/DisplayServiceHelper/library.json` (name/version/description).
- [ ] Native stub behaviour: under `UNIT_TEST`, `available()` returns a controllable value and `run(work, _)` executes `work()` synchronously on the calling thread and returns true — so the gate/fallback contract is unit-testable without FreeRTOS.
- [ ] `test/test_unit/test_deepStackExecutor/test_deepStackExecutor.cpp` — GoogleTest cases: (a) `run()` invokes the callable exactly once and returns true (native synchronous path); (b) a callable that writes a sentinel is observed after `run()` returns (ordering/blocking contract); (c) `available()` reflects the stubbed capability; (d) contract when `available()` is false (documented behaviour for WO2's fallback decision). Register the test dir consistent with existing `test/test_unit/test_pngWriter` wiring.
- [ ] Confirm `test_filter`/`lib_ignore` in `[env:native]` still builds; add `DeepStackExecutor` to native `lib_ignore` **only** if the FreeRTOS symbols can't be stubbed for link.

### Verification Requirement
Run `run_tests` (native) — new test passes, existing suite unaffected. Run `build_project` for `crowpanel-128-rotary` — compiles clean. **Before committing, ask the user if they are happy with the changes.**

### Issue Surfacing
If you find a defect/concern outside this boundary box (e.g., a latent bug in `PngWriter` or the snapshot handshake), record it in the Completion Report — do **not** file an issue or fix it here.

---
## Completion Report
<!-- Written by the worker at task completion -->

### Status
COMPLETE

### Changes Made
- Created `lib/DeepStackExecutor` component (`DeepStackExecutor.h`, `DeepStackExecutor.cpp`, `library.json`).
- Added native test file `test/test_unit/test_deepStackExecutor/test_deepStackExecutor.cpp` covering stub capabilities.
- Verified native unit tests passed (667 tests).
- Verified `crowpanel-128-rotary` environment compiles cleanly.

### Decisions Made
- Used `#ifndef UNIT_TEST` stubs for native environment, removing the need to add `DeepStackExecutor` to `lib_ignore`.

### Dead Ends & Warnings
- N/A

### Out-of-Scope Findings
- N/A

### Blockers
- (none)
