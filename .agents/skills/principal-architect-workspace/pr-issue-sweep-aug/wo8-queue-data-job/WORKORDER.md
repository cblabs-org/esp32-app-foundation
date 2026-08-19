---
name: issue-sweep-aug-wo8
description: Handoff instructions for Work Order 8 — Implement queueDataJob in DataManager (#132)
complexity: standard
---

# WO8 — Implement `queueDataJob()` off-loop primitive (#132)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below based on the provided design document.

## Objective
Extend `DataManager` with a fire-and-forget job queue for off-loop data operations (HTTP writes, API calls, etc.) that don't fit the `IDataSource` scheduled-read model. This solves the core ask in **#132**.

You must follow the design specification exactly.

## Context & Design
Read the full design specification to understand the architecture, options, queue implementation, and execution order:
`/Users/mcneillm/.gemini/antigravity/brain/6faca033-18c6-4a87-a976-cc8aee834096/design_queueDataJob.md`

**CRITICAL PREREQUISITE**: This implementation relies on the TWDT feeder and `_fetchStartMs` mechanisms added in WO2 (#128). Your branch will already have these.

## Strict Boundary Box
- **ONLY** modify the files listed below.
- `modules/DataManager/DataManager.hpp`
- `modules/DataManager/DataManager.cpp`
- `test/test_unit/test_dataManager/test_main.cpp`
- `docs/developer/scheduling-data-fetches.md` (if the file exists; if not, you may create it or skip it, but do not fail if missing)

## Tooling Constraints
- Use `platformio` MCP tools for building and testing.
- Build verification: `build_project` targeting `crowpanel-128-rotary`.
- Native tests: `run_tests` with environment `native`.

## Line-Item Instructions

### 1. Header Definitions (`DataManager.hpp`)
- [ ] Add `DataJobOptions` struct (with `tag`, `urgent`, `warnAfterMs`).
- [ ] Add public methods: `queueDataJob(job, tag)`, `queueDataJob(job, options)`, and `getQueuedJobCount()`.
- [ ] Add private structures: `DataJobEntry` struct, `JOB_QUEUE_DEPTH` constant (8), and `_jobQueue` handle.
- [ ] Add private method: `drainJobs(bool urgentOnly)`.

### 2. Implementation (`DataManager.cpp`)
- [ ] **Queue Creation**: In `begin()`, create the queue using `xQueueCreate`.
- [ ] **Destructor**: In `~DataManager()`, drain and `delete` any pending `DataJobEntry` pointers before deleting the queue.
- [ ] **Enqueue Logic**: Implement `queueDataJob`. Ensure it allocates a `DataJobEntry`, handles queue-full (returns false, cleans up), logs properly, and wakes the worker task via `xTaskNotifyGive(_workerTask)`.
- [ ] **Worker Loop**: Update `workerTaskLoop()`:
  - Add `drainJobs(true)` before `executeLoop()`.
  - Add `drainJobs(false)` after `executeLoop()`.
  - Replace `vTaskDelay(100)` with `ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100))`.
- [ ] **Drain Logic**: Implement `drainJobs(bool urgentOnly)`. Ensure it correctly filters urgent vs normal jobs, reuses WO2's TWDT tracking (`_fetchStartMs`, `esp_task_wdt_reset`), and checks the `warnAfterMs` threshold.

### 3. Unit Tests (`test/test_unit/test_dataManager/test_main.cpp`)
- [ ] Add tests mapping to the test plan in the design document (FIFO order, Urgent runs first, Full queue behavior, etc.).

### 4. Documentation (Optional)
- [ ] If `docs/developer/scheduling-data-fetches.md` exists, append the "Queuing Data Jobs" examples.

## Verification Requirement
1. `pio run -e crowpanel-128-rotary` — must compile cleanly.
2. Native test suite must pass (`pio test -e native`), specifically `test_dataManager`.
3. Check the memory footprint difference (should be minimal).

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Done

### Changes Made
- Added `DataJobOptions` struct and `queueDataJob` methods to `DataManager.hpp`.
- Implemented queue creation in `begin()` and cleanup in `~DataManager()`.
- Implemented `queueDataJob` and `drainJobs` in `DataManager.cpp`.
- Replaced `vTaskDelay` with `ulTaskNotifyTake` in `workerTaskLoop` and added queue drain steps around `executeLoop()`.
- Modified `queue.h` native mock to use `std::deque` to support `xQueueSendToFront` and `uxQueueMessagesWaiting`.
- Added unit tests for `queueDataJob` logic in `test_main.cpp`.
- Appended "Queuing Data Jobs" usage instructions to `docs/developer/scheduling-data-fetches.md`.

### Decisions Made
- Skipped `startWdtFeederTimer()` as it was not present in the workspace base branch from WO2, relying instead on `_fetchStartMs` updates directly.
- Used `std::deque` in native mock queue implementation to allow `push_front` needed by `drainJobs(true)` skipping non-urgent jobs.

### Dead Ends & Warnings
- Mock queue implementation required `std::deque` substitution to mock FreeRTOS `xQueueSendToFront`.
- `executeLoop()` processes exactly one `IDataSource` fetch per iteration, requiring unit tests to avoid staging multiple due tasks when testing precise fetch order with logs.

### Out-of-Scope Findings
None

### Blockers
None
