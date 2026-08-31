# AuditModule Developer Guide

The `AuditModule` (`modules/AuditModule/AuditModule.hpp`) provides a standardized, flash-persisted, structured audit trail for significant application and security events.

Unlike volatile debug logs (`ESP_LOG*`, `get_debug_logs`), which are lost on reset, audit records are persisted in JSONL format across reboots on the LittleFS `storage` partition.

Implements [`IWebContributor`](../interfaces/web-contributor.md), [`IMCPContributor`](../interfaces/mcp-contributor.md), and [`IPreconditionContributor`](../interfaces/precondition-contributor.md).

---

## 1. Overview & Architecture

- **Dedicated DRAM Writer Task**: An internal FreeRTOS task (`AuditWriter`, 3 KiB stack, priority 2) drains a shallow DRAM queue (`AUDIT_QUEUE_DEPTH = 16`) to decouple callers from flash I/O latency.
- **Batch-Flush Durability**: The writer drains the queue and writes all records in a single `fopen(append)` → N × `fprintf` → `fflush` → `fclose` cycle. Records survive brownouts and hard faults without periodic flushing timers.
- **Two-File Rotation**: Stored at `/littlefs/audit_0.jsonl` and `/littlefs/audit_1.jsonl`. Each file is bounded to `BOARD_AUDIT_BYTES / 2`. When the active file fills up, the older file is deleted and becomes the new active target.
- **Automatic Secret Redaction**: All `details` strings are processed by `Redactor::redact()` before being committed to flash, preventing credentials, tokens, or passwords from leaking into audit storage.

---

## 2. Two-Tier API

The framework provides a strict two-tier access model:

### Tier 1: Soft Dependency (`include/Audit.h`) — Recommended for Shared Modules
Use the `AUDIT()` and `AUDIT_SYNC()` convenience macros in shared libraries, domain services, and consumer modules.

```cpp
#include "Audit.h"

// Asynchronous (non-blocking, enqueues to background task)
AUDIT("config", "setting_changed", "wifi", "ssid=HomeNet");

// Synchronous (blocks until record is flushed to flash)
AUDIT_SYNC("auth", "login_failed", "admin", "invalid password");
```

- **When `WITH_AUDIT` is defined**: Expands to `AuditModule::instance().log(...)` or `AuditModule::instance().auditSync(...)`.
- **When `WITH_AUDIT` is NOT defined**: Expands to `((void)0)` — zero flash, zero RAM, and zero CPU overhead.

### Tier 2: Direct API (`modules/AuditModule/AuditModule.hpp`) — Kernel & Dedicated Surfaces
Directly referencing `AuditModule::instance()` is reserved for modules, endpoints, or tools that strictly require audit functionality.

```cpp
#include "AuditModule.hpp"

AuditModule& audit = AuditModule::instance();
audit.log("ota", "update_started", "firmware", "v0.2.0");
```

> [!CAUTION]
> Direct inclusion of `AuditModule.hpp` without defining `WITH_AUDIT` produces a compile-time `#error` directing developers to `include/Audit.h`.

---

## 3. Audit Record Schema

Each line in the JSONL audit file is a single JSON object formatted as follows:

```json
{"t_ms":12540,"ts":1725012345,"seq":0,"boot_id":"a1b2c3d4","build":"v0.2.0-dev.1","source":"config","action":"setting_changed","target_id":"wifi","details":"ssid=HomeNet"}
```

### Field Definitions

| Field | Type | Max Chars | Description |
|:---|:---|:---|:---|
| `t_ms` | `uint32_t` | — | Millis timestamp (`millis()`) when the record was enqueued |
| `ts` | `int64_t` | — | Epoch seconds from `time(nullptr)` (or `0` if NTP is not synced) |
| `seq` | `uint32_t` | — | Monotonic per-boot sequence counter (reset by `clearAll()`) |
| `boot_id` | `string` | 8 | 8-character hex identifier unique to each boot cycle (`esp_random()`) |
| `build` | `string` | — | `BUILD_VERSION` injected at compile time (or `"unknown"`) |
| `source` | `string` | 31 | Originating subsystem or module (e.g. `"auth"`, `"config"`, `"mcp"`) |
| `action` | `string` | 47 | Specific action taken (e.g. `"login_failed"`, `"setting_changed"`) |
| `target_id`| `string` | 47 | *Optional.* Target device, entity, or namespace (omitted if empty) |
| `details` | `string` | 127 | *Optional.* Free-form payload (redacted before storage, omitted if empty) |

> [!NOTE]
> Max Chars is the usable string capacity (C struct size minus null terminator).

---

## 4. Board Budget & Flash Configuration

The total flash storage budget for audit logs is controlled by `BOARD_AUDIT_BYTES`:

- **Default**: `32768` bytes (32 KiB total = two 16 KiB rotation files).
- **Custom Budget**: Set `-DBOARD_AUDIT_BYTES=N` in your board's `build_flags` in `platformio.ini`:
  ```ini
  build_flags = -DBOARD_AUDIT_BYTES=131072  ; 128 KiB for S3 boards
  ```
- **Framework defaults**: S3 boards (esp32s3nano, crowpanel-128-rotary) set 128 KiB. esp32dev uses the 32 KiB default.
- **Fallback**: `hardware/_common/board_contract.h` provides the 32 KiB fallback if omitted.
- **Storage Preflight**: During `begin()`, `AuditModule` verifies that `LittleFS.totalBytes() - LittleFS.usedBytes() >= BOARD_AUDIT_BYTES`. If storage is insufficient, initialization safely aborts.

---

## 5. Rate Limiting & Flash Protection

To prevent flash wear from burst logging, `AuditModule` includes a token-bucket
rate limiter that runs at the **writer boundary**:

- **Default**: 120 records per minute (`AUDIT_RATE_LIMIT_PER_MIN`).
- **Writer-side enforcement**: Callers always enqueue unconditionally. The writer
  task peeks at the queue, checks the rate limit, and delays (50ms) when throttled.
  Records stay safely in the queue — the only drop point is queue-full.
- **Batch flush**: When the rate limit allows, the writer drains the entire queue
  and writes all records in a single `open → write-N → close` for flash efficiency.

### Flash Wear Guidance

At the default rate limit of 120 records/min (~200 bytes each), the 32 KiB budget
rotates in approximately 1.4 minutes at sustained max rate. LittleFS wear-leveling
distributes writes across the partition, but sustained high-rate logging should be
avoided.

**Recommended operational budgets:**
- **Normal operation**: < 10 events/hour (administrative, config, security events)
- **Burst scenario**: < 60 events/minute during incident response or OTA
- **Anti-pattern**: > 120 events/minute sustained — this will trigger rate limiting and delay writes

---

## 6. Security Design — No Runtime Toggle

Audit cannot be disabled at runtime. There is no `IConfigurable` implementation,
no `enabled` config property, and no MCP tool to toggle auditing. This is a
deliberate security decision: a compromised agent cannot disable auditing,
perform malicious actions, then re-enable it.

The binary either has full auditing (`WITH_AUDIT` defined) or none (macro no-ops).
There is no runtime attack surface.

---

## 7. Factory Reset & Maintenance

- **Factory Reset**: `config_factory_reset` (MCP) and the web portal factory reset wipe **everything** — module configuration files (`/mod/*.json`), the config manifest, and audit trail files. `AuditModule::clearAll()` is called automatically, deleting both JSONL files and resetting all counters (`seq`, `dropCount`, `recordCount`). A factory reset is a factory reset.
- **Manual Clearing**: Also available independently via REST (`POST /api/system/audit/clear`) and MCP (`clear_audit_log`), which call `clearAll()` without affecting other configuration.

---

## 8. Framework-Level Audit Events

The framework instruments these security-relevant operations out of the box:

| Source | Action | When | Sync? |
|---|---|---|---|
| `audit` | `started` | `begin()` succeeds — first record in every boot chain | No |
| `ota` | `pipeline_start` | OTA worker task begins manifest-first flow | No |
| `ota` | `pipeline_complete` | Firmware verified, pre-reboot | **Yes** (`AUDIT_SYNC`) |
| `ota` | `pipeline_failed` | Download/verify failure (error in details) | No |
| `keys` | `key_added` | Key stored to encrypted registry (ID only, never secret) | No |
| `keys` | `key_deleted` | Key removed from registry | No |

Consumer apps add their own events via `AUDIT()` / `AUDIT_SYNC()` macros.

---

## 9. Multi-Surface Integration

### REST Endpoints

| Endpoint | Method | Response | Description |
|:---|:---|:---|:---|
| `/api/system/audit` | `GET` | `application/x-ndjson` | Paginated audit log retrieval. Supports query params: `lines` (default 50, max 200), `offset` (byte offset, default 0), `file` (`0` or `1`, default active). Headers returned: `X-Audit-Truncated`, `X-Audit-Next-Offset`. |
| `/api/system/audit/clear` | `POST` | `application/json` | Clears all audit log files and resets counters. Returns `{"ok": true}`. |

### MCP Tools

| Tool | Type | Description | Parameters |
|:---|:---|:---|:---|
| `get_audit_log` | Read-Only | Retrieve flash-persisted structured audit records. | `lines` (int, default 50, max 200), `offset` (int, default 0), `file` (int, 0 or 1). |
| `clear_audit_log` | Mutating | Wipe audit log files and reset diagnostic counters. | None. (Blocked when MCP is in `readOnly` mode). |

### Preconditions (`audit_health`)

Contributed via `IPreconditionContributor` with role `PreconditionRole::Operational`:

| Precondition State | Status | Cause | Help Text |
|:---|:---|:---|:---|
| Uninitialized / Preflight failed | `false` | `Cause::InternalError` | "Audit storage or task initialization failed" |
| Disabled via config | `false` | `Cause::Unavailable` | "Audit logging disabled in configuration" |
| Dropping records | `true` | `Cause::FallbackEngaged` | "Audit logging degraded — dropping records" |
| Normal operation | `true` | `Cause::Met` | "Audit logging active" |

> [!NOTE]
> The `FallbackEngaged` state is **sticky** — once a single drop occurs, the precondition stays degraded until `clearAll()` is called (which resets counters). This is intentional: an audit trail should surface historical reliability problems, not silently recover.

---

## 10. Thread Safety & Concurrency

> [!IMPORTANT]
> Understanding the thread-safety model is critical for correct use in multi-task firmware.

| Function | Thread-Safe? | Notes |
|:---|:---|:---|
| `log()` | ✅ Yes | Non-blocking `xQueueSend` with zero-wait. Safe from any FreeRTOS task. |
| `auditSync()` | ✅ Yes | Per-call binary semaphore. Safe from any task, but **blocks the caller** (see below). |
| `clearAll()` | ✅ Yes | File deletion + atomic counter resets. Writer task may interleave a write. |
| `checkRateLimit()` | ✅ Yes | Writer-side only, protected by `_rateLimitMutex`. |
| `recordCount()`, `dropCount()` | ✅ Yes | `std::atomic` reads, safe from any context. |
| `activeFileIndex()`, `currentFileSize()` | ⚠️ Eventually consistent | Written by the writer task without atomics. Reads from other threads may see slightly stale values. Safe but not real-time precise. |
| `begin()` | ❌ Single-call | Must be called once from the main task during `setup()`. |
| `log()` / `auditSync()` | ❌ **Not ISR-safe** | Heap allocation (`new`) and FreeRTOS queue primitives are not ISR-safe. |

### `auditSync()` Timeout Semantics

`auditSync()` blocks the calling thread for up to **5 seconds** (default `timeoutMs = 5000`) waiting for the writer task to flush the record to flash.

- **On success**: Returns `true`. The record is confirmed written to flash.
- **On timeout**: Returns `false` and increments `_dropCount`. However, the record **remains in the queue** and **will still be written to flash** by the writer task. The semaphore is cleaned up by the writer (no memory leak). The `false` return means "not confirmed within your deadline", not "record lost".
- **On queue-full**: Returns `false` immediately (the record is deleted, not enqueued).

### Redactor Thread-Safety Contract

`Redactor::registerSecret()` must only be called during single-threaded initialization (`setup()`) before `begin()` returns. The internal `_secrets` vector has no mutex protection. Concurrent `registerSecret()` calls from background tasks will cause undefined behavior.

---

## 11. Limitations & Known Constraints

> [!WARNING]
> Read this section carefully before integrating audit logging into production firmware.

### JSON Field Escaping
`formatRecord()` injects string fields via `%s` (C `snprintf`) **without JSON string escaping**. This is safe for current callers because `source`, `action`, and `targetId` are short alphanumeric constants, and `details` is redacted before formatting. However, if `details` ever carries arbitrary user-supplied text containing `"`, `\`, or control characters, the JSONL record will be malformed. **Mitigation**: Keep `details` to simple key=value pairs. If arbitrary text must be logged, sanitize it before passing to `AUDIT()`.

### 512-Byte Record Buffer
Each record is formatted into a 512-byte stack buffer in `writeBatch()`. If the serialized JSON exceeds 511 bytes (unlikely with the field size limits), `snprintf` silently truncates it, producing a malformed JSON line. The field size limits (31+47+47+127 = 252 chars of payload) keep records well within this budget under normal use.

### Queue Saturation Under Load
The DRAM queue holds 16 records. When the queue is full:
- `log()` returns `false` immediately and **deletes the record** — it is permanently lost.
- `_dropCount` is incremented and `audit_health` degrades to `FallbackEngaged` (sticky).
- The queue uses zero-wait semantics (`xQueueSend` with `0` timeout) to avoid blocking callers.

### No EventBus Integration
`AuditModule` does not publish to or subscribe from the EventBus. There is no `audit.record.written` event. This is intentional to avoid circular dependencies (if EventBus handlers themselves emit audit records).

### `begin()` Must Precede All Logging
If `log()` or `auditSync()` is called before `begin()` completes, the function returns `false` immediately. Ensure `registerModule(&auditModule)` executes during `Application::begin()` before any module emits audit records.

---

## 12. Anti-Patterns & Best Practices

> [!CAUTION]
> Avoid these common audit anti-patterns:

- ❌ **Directly including `AuditModule.hpp` in shared modules**
  - *Problem*: Creates hard coupling to `AuditModule` and causes compile errors if `WITH_AUDIT` is disabled.
  - *Fix*: Always `#include "Audit.h"` and use the `AUDIT()` macro.
- ❌ **Calling `AUDIT()` inside Interrupt Service Routines (ISRs)**
  - *Problem*: `AUDIT()` allocates memory and calls FreeRTOS queue primitives that are not ISR-safe.
  - *Fix*: Defer audit logging to a task or EventBus handler.
- ❌ **Calling `AUDIT_SYNC()` in tick loops or EventBus callbacks**
  - *Problem*: `AUDIT_SYNC()` blocks the calling thread for up to 5 seconds. This will stall the main loop and trigger the Task Watchdog Timer (TWDT).
  - *Fix*: Use asynchronous `AUDIT()` in tick loops and EventBus subscribers. Reserve `AUDIT_SYNC()` for deliberate shutdown/handshake sequences in dedicated tasks.
- ❌ **Logging high-frequency sensor readings as audit events**
  - *Problem*: Rapid writes quickly exhaust the 120/min rate limit and cause premature flash wear.
  - *Fix*: Audit logs are for administrative, operational, and security events. Use telemetry channels, display buffers, or debug logs for high-frequency measurements.
- ❌ **Calling `Redactor::registerSecret()` from background tasks**
  - *Problem*: The secret registry is a `std::vector` without mutex protection. Late registration races with concurrent `redact()` calls from the writer task.
  - *Fix*: Register all secrets during `setup()` before `Application::begin()` returns.
