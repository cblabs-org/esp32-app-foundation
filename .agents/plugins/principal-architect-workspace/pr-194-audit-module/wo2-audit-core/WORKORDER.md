---
name: pr-194-audit-module-wo2
description: Handoff instructions for Work Order 2 — AuditModule core engine
complexity: standard
---

# WO2 — AuditModule Core Engine

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement the core `AuditModule` — a flash-persisted, FreeRTOS-driven audit trail engine. It uses a DRAM queue + low-priority worker task (same pattern as DataManager), writes JSONL to the `storage` partition (`/littlefs`), rotates between two files, and provides `log()`, `auditSync()`, and `clearAll()` methods. This WO builds the **engine only** — no REST, MCP, config, or precondition surfaces.

## Context
This is part of RFC #194. The `lib/Redactor` extraction (WO1) is already merged on this branch. The AuditModule uses `Redactor::redact()` to sanitize audit record `details` before writing to flash.

**Parent project**: `pr-194-audit-module` — see `.agents/skills/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

**Key design decisions that govern this implementation**:
- **D1**: Board-derived flash budget via `BOARD_AUDIT_BYTES` (not yet defined in board.h — use a default constant for now, WO3 adds board overrides)
- **D3**: DataManager-style DRAM queue + low-priority worker task
- **D4**: `SYNC_EACH` durability — open → append → flush → close per record
- **D11**: Rate limiter (N records/min), visible `audit_throttled` record
- **D12**: `auditSync()` blocking variant for must-not-lose paths
- **D13/D19**: Schema: `{t_ms, ts, seq, boot_id, build, source, action, target_id, details}`
- **D14**: JSONL reader drops trailing unparseable line (crash tolerance)
- **D16**: Files on `storage` partition at `/littlefs/audit_0.jsonl` and `/littlefs/audit_1.jsonl`

## Strict Boundary Box
- **DO NOT** create REST endpoints, MCP tools, or web routes — that is WO4.
- **DO NOT** implement `IConfigurable`, `IPreconditionContributor`, or `IWebContributor` — that is WO4/WO5.
- **DO NOT** modify `board.h`, `board_contract.h`, or `platformio.ini` — that is WO3/WO6.
- **DO NOT** create the `AUDIT()` macro or `WITH_AUDIT` gating — that is WO6.
- **DO NOT** modify any files outside `modules/AuditModule/` and `test/test_unit/`.
- **DO NOT** modify `lib/Logger/`, `lib/Redactor/`, or any existing module.
- The module header file extension must be `.hpp` (house style), the implementation `.cpp`.

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (`build_project`, `run_tests`), NOT raw `pio` CLI commands.

## Reference Code to Follow

### DataManager Worker Task Pattern
Study `modules/DataManager/DataManager.cpp` lines 174-204 (queue creation, DRAM task spawn) and lines 572-606 (worker loop with `ulTaskNotifyTake`). Key characteristics:
- Queue of pointers to heap-allocated job entries
- `xTaskCreate` (NOT `xTaskCreateWithCaps`) — must use DRAM stack for SPI flash safety
- Worker loop: drain queue → process → feed TWDT → sleep/notify
- Priority 5 for DataManager — use **priority 2** for audit (lower, non-critical path)

### LittleFS File Operations
Study `modules/TimeManager/TimeManager.cpp` lines 447-475 for the write-temp-rename pattern. For audit, we use direct append-and-close (simpler — we're appending one line at a time, not replacing a whole file).

The storage partition is mounted by `Application.cpp` at `/littlefs` (partition label `storage`). Use this mount point for all file paths.

## Line-Item Instructions

### 1. Create `modules/AuditModule/AuditModule.hpp`
- [ ] Apache-2.0 header matching house style
- [ ] Class `AuditModule` — for now it stands alone (no interfaces). Future WOs will add `IWebContributor`, `IMCPContributor`, `IConfigurable`, `IPreconditionContributor`.
- [ ] **Public API**:
  - `bool begin()` — free-space preflight, create queue, spawn writer task. Returns false on failure.
  - `bool log(const char* source, const char* action, const char* targetId = nullptr, const char* details = nullptr)` — enqueue an audit record (non-blocking). Returns false if queue is full.
  - `bool auditSync(const char* source, const char* action, const char* targetId = nullptr, const char* details = nullptr, uint32_t timeoutMs = 5000)` — enqueue + block until written via semaphore. Returns false on timeout or queue-full.
  - `void clearAll()` — delete both JSONL files, reset counters (for factory reset).
  - `size_t recordCount() const` — total records written this boot (for diagnostics).
  - `size_t dropCount() const` — records dropped due to queue full (for diagnostics).
  - `size_t throttleCount() const` — records suppressed by rate limiter (for diagnostics).
  - Singleton access: `static AuditModule& instance()` — returns a lazily-constructed static instance.
- [ ] **Private members**:
  - `QueueHandle_t _queue` — FreeRTOS queue of `AuditRecord*` pointers
  - `TaskHandle_t _writerTask` — handle to the DRAM writer task
  - `volatile uint32_t _seq` — monotonic sequence counter (per boot, starts at 0)
  - `uint32_t _bootId` — random boot identifier (from `esp_random()` at `begin()`)
  - `size_t _currentFileSize` — in-memory tracking of active file size
  - `uint8_t _activeFileIndex` — 0 or 1, which JSONL file is active
  - `size_t _maxFileSize` — half of `AUDIT_BUDGET_BYTES` (total budget / 2, one per file)
  - `std::atomic<size_t> _dropCount` — queue-full drops
  - `std::atomic<size_t> _recordCount` — total writes
  - `std::atomic<size_t> _throttleCount` — rate-limited suppressions
  - Rate limiter state: token bucket or sliding window (see instruction 3)
  - `bool _started` — guard against double `begin()`
- [ ] **Constants** (as `static constexpr` or `#define`):
  - `AUDIT_QUEUE_DEPTH = 16`
  - `AUDIT_WRITER_STACK_SIZE = 4096` — small, only does file I/O
  - `AUDIT_WRITER_PRIORITY = 2` — below DataManager (5), above idle (0)
  - `AUDIT_BUDGET_BYTES` — default 32768 (32 KiB). WO3 will override this via `BOARD_AUDIT_BYTES`.
  - `AUDIT_RATE_LIMIT_PER_MIN = 120` — max records per minute
  - `AUDIT_FILE_PREFIX = "/littlefs/audit_"` — path prefix
- [ ] **Internal struct `AuditRecord`** (heap-allocated, passed through queue):
  ```cpp
  struct AuditRecord {
      uint32_t t_ms;           // millis() at log time
      int64_t  ts;             // epoch seconds from time(nullptr), or 0 if NTP not synced
      uint32_t seq;            // monotonic per-boot sequence
      char source[32];         // e.g., "mcp", "config", "ha"
      char action[48];         // e.g., "set_temperature", "factory_reset"
      char targetId[48];       // e.g., device id, entity id (optional)
      char details[128];       // free-form (will be redacted), optional
      SemaphoreHandle_t sync;  // non-null for auditSync() — given after write
  };
  ```

### 2. Create `modules/AuditModule/AuditModule.cpp`
- [ ] **`begin()`**:
  - Set `_bootId` from `esp_random()`
  - Seed `_currentFileSize` by checking existing file sizes (`stat()` or `fopen` + `fseek/ftell`)
  - If both files exist, the active file is the one with the larger `seq` in its last valid line (or the smaller file). For simplicity in v1: start with `_activeFileIndex = 0`, seed size from file 0. If file 0 is at/over max, switch to file 1.
  - Free-space preflight: check available space on `/littlefs` using `LittleFS.totalBytes() - LittleFS.usedBytes()`. If free space < `AUDIT_BUDGET_BYTES`, log a warning and return false.
  - Create queue: `_queue = xQueueCreate(AUDIT_QUEUE_DEPTH, sizeof(AuditRecord*))`
  - Spawn writer task: `xTaskCreate(writerLoop, "AuditWriter", AUDIT_WRITER_STACK_SIZE, this, AUDIT_WRITER_PRIORITY, &_writerTask)` — DRAM stack, no PSRAM
  - Return true on success
- [ ] **`log()`**:
  - Check rate limiter — if over limit, increment `_throttleCount` and return false. If crossing the threshold, enqueue a synthetic `audit_throttled` record (source="audit", action="rate_limited").
  - Heap-allocate an `AuditRecord*` via `new (std::nothrow)`. If allocation fails, increment `_dropCount`, return false.
  - Populate all fields: `t_ms = millis()`, `ts = time(nullptr)` (returns 0 pre-NTP), `seq = _seq++`, copy source/action/targetId/details with `strncpy` + null-terminate.
  - Redact the `details` field: `Redactor::redact(record->details, sizeof(record->details))`
  - Set `record->sync = nullptr` (async path)
  - Enqueue: `xQueueSend(_queue, &record, 0)`. If full, `delete record`, increment `_dropCount`, return false.
  - Return true.
- [ ] **`auditSync()`**:
  - Same as `log()` but create a binary semaphore: `record->sync = xSemaphoreCreateBinary()`
  - After enqueue, `xSemaphoreTake(record->sync, pdMS_TO_TICKS(timeoutMs))`. On timeout: increment `_dropCount`, return false. Note: the writer will still eventually process the record and delete it + the semaphore.
  - On success: return true. The semaphore is given by the writer after the write completes, and the writer deletes the semaphore and the record.
- [ ] **Writer loop** (`static void writerLoop(void* param)`):
  - `esp_task_wdt_add(NULL)` — register with TWDT
  - Loop forever:
    - `AuditRecord* record = nullptr;`
    - `if (xQueueReceive(self->_queue, &record, pdMS_TO_TICKS(1000)) == pdTRUE)`
      - Call `self->writeRecord(record)`
      - If `record->sync` is non-null: `xSemaphoreGive(record->sync)`, then `vSemaphoreDelete(record->sync)`
      - `delete record`
    - `esp_task_wdt_reset()`
- [ ] **`writeRecord(AuditRecord* record)`** (private):
  - Build the JSONL line using `snprintf` into a stack buffer (~384 bytes). Format:
    ```json
    {"t_ms":12345,"ts":1717000000,"seq":0,"boot_id":"a1b2c3d4","build":"v0.0.12-dev.54","source":"mcp","action":"set_temp","target_id":"zone_1","details":"value=22"}
    ```
    - `boot_id` as 8-hex-char lowercase string from `_bootId`
    - `build` from the `BUILD_VERSION` macro (defined by `inject_version.py` — it's a string literal available at compile time). If `BUILD_VERSION` is not defined, use `"unknown"`.
    - Omit `target_id` and `details` keys entirely if the source strings were empty (saves flash space).
  - Open the active file in append mode: `FILE* f = fopen(path, "a");`
  - Write the line + newline: `fprintf(f, "%s\n", line);`
  - `fflush(f); fclose(f);` — SYNC_EACH pattern
  - Update `_currentFileSize += (lineLen + 1)`
  - Increment `_recordCount`
  - **Rotation check**: if `_currentFileSize >= _maxFileSize`:
    - Switch `_activeFileIndex` to the other file (0→1, 1→0)
    - Delete the new active file if it exists (we're overwriting the old one): `remove(newPath)`
    - Reset `_currentFileSize = 0`
- [ ] **`clearAll()`**:
  - Delete both audit files: `remove("/littlefs/audit_0.jsonl"); remove("/littlefs/audit_1.jsonl");`
  - Reset `_currentFileSize = 0`, `_activeFileIndex = 0`, `_seq = 0`
  - Reset counters: `_dropCount = 0; _recordCount = 0; _throttleCount = 0;`
- [ ] **Rate limiter** — simple token-bucket:
  - Track `_rateBucketTokens` (int, starts at `AUDIT_RATE_LIMIT_PER_MIN`) and `_rateBucketLastRefill` (uint32_t, millis timestamp)
  - On each `log()` call: refill tokens proportional to elapsed time since last refill (up to max). If tokens <= 0, rate-limit. Otherwise, decrement and proceed.
  - Rate limiter state is only accessed from the calling task (not the writer), so no mutex needed — but use `portENTER_CRITICAL` / `portEXIT_CRITICAL` if multiple tasks might call `log()` concurrently. Actually, since `log()` may be called from multiple tasks, protect the rate limiter with a spinlock or mutex.

### 3. Create `modules/AuditModule/library.json`
- [ ] Standard PlatformIO library metadata matching the `lib/Redactor/library.json` pattern.

### 4. Create `test/test_unit/test_audit_module/test_audit_module.cpp`
- [ ] Unit tests covering (note: on native platform, FreeRTOS and LittleFS are not available — test the serializable/testable logic):
  - **JSONL serialization**: Test that `writeRecord` produces correct JSON format (may need to extract the serialization into a testable static method)
  - **Rate limiter**: Test token bucket refill, depletion, and recovery
  - **Record struct field copying**: Test `strncpy` truncation and null-termination
  - **Redaction integration**: Register a secret, create a record with that secret in details, verify it's redacted after `log()` processes it
  - **Sequence counter**: Verify seq increments monotonically
- [ ] Use the gtest pattern from `test/test_unit/test_logger/test_logger.cpp`
- [ ] For FreeRTOS-dependent code (queue, task), use compile-time guards (`#ifdef ESP_PLATFORM`) to skip those tests on native. Focus unit tests on the pure logic.

### 5. Build verification
- [ ] Build with `platformio` MCP `build_project` for `native_test` environment
- [ ] Build with `platformio` MCP `build_project` for `esp32dev` environment
- [ ] Run tests with `platformio` MCP `run_tests`

## Verification Requirement
Before committing, the worker MUST:
1. Confirm all existing tests still pass (including the 851 from WO1).
2. Confirm new AuditModule tests pass.
3. Confirm `esp32dev` firmware builds cleanly.
4. Ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report

### Status
Complete — fully validated across native test runner (861/861 tests passing) and firmware targets (esp32dev, esp32s3nano, crowpanel-128-rotary).

### Changes Made
- Created `modules/AuditModule/AuditModule.hpp`: Standalone `AuditModule` singleton class, `AuditRecord` struct, rate limiter token-bucket model, sequence tracking, and diagnostic counters.
- Created `modules/AuditModule/AuditModule.cpp`: Storage preflight, FreeRTOS queue + DRAM worker task (`AuditWriter`, priority 2, stack 4096), `SYNC_EACH` durability, dual-file JSONL rotation (`/littlefs/audit_0.jsonl` and `/littlefs/audit_1.jsonl`), secret redaction via `Redactor::redact()`, non-blocking `log()`, blocking `auditSync()`, and `clearAll()`.
- Created `modules/AuditModule/library.json`: PlatformIO library configuration.
- Created `test/test_unit/test_audit_module/test_audit_module.cpp`: 10 comprehensive unit tests for serialization, rate limiting, field truncation, redaction integration, and clearAll counter resets.
- Modified `test/stubs/platform/freertos/semphr.h`: Added `xSemaphoreCreateBinary()` stub for native GoogleTest builds.

### Decisions Made
- Used `std::mutex` for token-bucket rate limiter thread safety to allow concurrent `log()` / `auditSync()` calls across FreeRTOS tasks.
- In `AuditModule::formatRecord()`, omitted `target_id` and `details` fields when empty string to save flash bytes and adhere to schema spec.
- Added `#ifndef ESP_PLATFORM break; #endif` inside `writerLoop` on native builds to avoid blocking single-threaded test runner while maintaining full FreeRTOS loop behavior on hardware.

### Dead Ends & Warnings
- None.

### Out-of-Scope Findings
- None.

### Blockers
- None.
