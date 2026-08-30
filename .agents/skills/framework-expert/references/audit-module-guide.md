# AuditModule Developer Guide

The `AuditModule` (`modules/AuditModule/AuditModule.hpp`) provides a standardized, flash-persisted, structured audit trail for significant application and security events.

Unlike volatile debug logs (`ESP_LOG*`, `get_debug_logs`), which are lost on reset, audit records are persisted in JSONL format across reboots on the LittleFS `storage` partition.

Implements [`IWebContributor`](../interfaces/web-contributor.md), [`IMCPContributor`](../interfaces/mcp-contributor.md), [`IConfigurable`](../interfaces/configurable.md), and [`IPreconditionContributor`](../interfaces/precondition-contributor.md).

---

## 1. Overview & Architecture

- **Dedicated DRAM Writer Task**: An internal FreeRTOS task (`AuditWriter`, 4 KB stack, priority 2) drains a shallow DRAM queue (`AUDIT_QUEUE_DEPTH = 16`) to decouple callers from flash I/O latency.
- **`SYNC_EACH` Durability**: Every record write follows `fopen(append)` → `fprintf` → `fflush` → `fclose`. Records survive immediate brownouts, hard faults, or ungraceful reboots without periodic flushing timers.
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

| Field | Type | Description |
|:---|:---|:---|
| `t_ms` | `uint32_t` | Millis timestamp (`millis()`) when the record was enqueued |
| `ts` | `int64_t` | Epoch seconds from `time(nullptr)` (or `0` if NTP is not synced) |
| `seq` | `uint32_t` | Monotonic per-boot sequence counter |
| `boot_id` | `string` | 8-character hex identifier unique to each boot cycle (`esp_random()`) |
| `build` | `string` | `BUILD_VERSION` injected at compile time (or `"unknown"`) |
| `source` | `string` | Originating subsystem or module (max 32 chars, e.g. `"auth"`, `"config"`, `"mcp"`) |
| `action` | `string` | Specific action taken (max 48 chars, e.g. `"login_failed"`, `"setting_changed"`) |
| `target_id`| `string` | *Optional.* Target device, entity, or namespace (omitted if empty) |
| `details` | `string` | *Optional.* Free-form payload (max 128 chars, redacted before storage, omitted if empty) |

---

## 4. Board Budget & Flash Configuration

The total flash storage budget for audit logs is controlled by `BOARD_AUDIT_BYTES`:

- **Default**: `32768` bytes (32 KiB total = two 16 KiB rotation files).
- **Custom Budget**: Define `BOARD_AUDIT_BYTES` in your `hardware/<board-id>/board.h`:
  ```cpp
  #define BOARD_AUDIT_BYTES 65536  // 64 KiB total budget
  ```
- **Fallback Guarantee**: `hardware/_common/board_contract.h` provides the 32 KiB fallback if omitted.
- **Storage Preflight**: During `begin()`, `AuditModule` verifies that `LittleFS.totalBytes() - LittleFS.usedBytes() >= BOARD_AUDIT_BYTES`. If storage is insufficient, initialization safely aborts.

---

## 5. Rate Limiting & Flash Protection

To prevent flash wear from accidental tight loops or flood attacks, `AuditModule` includes a token-bucket rate limiter:

- **Default**: 120 records per minute (`AUDIT_RATE_LIMIT_PER_MIN`).
- **Runtime Configuration**: Modifiable via `IConfigurable` namespace `"audit"` (property `"rate_limit"`).
- **Throttling Behavior**:
  - The first record exceeding the rate limit triggers an automatic audit entry:
    `{"source": "audit", "action": "rate_limited", "details": "rate limit exceeded"}`
  - Subsequent excessive records are dropped until tokens refill, incrementing the `throttleCount()` diagnostic metric.

---

## 6. Runtime Toggle & Self-Documenting Gaps

Audit logging can be dynamically enabled or disabled via `applyConfig()`:

```json
{"enabled": false}
```

To prevent silent log tampering:
- When disabled, `AuditModule` writes a final self-documenting record before stopping:
  `{"source": "audit", "action": "audit_disabled"}`
- When re-enabled, `AuditModule` immediately records:
  `{"source": "audit", "action": "audit_enabled"}`

---

## 7. Factory Reset & Maintenance

- **Factory Reset**: By default, factory reset (`config_factory_reset`) wipes persisted configuration. `AuditModule::clearAll()` deletes `/littlefs/audit_0.jsonl` and `/littlefs/audit_1.jsonl`, resetting all counters (`seq`, `dropCount`, `throttleCount`, `recordCount`).
- **Manual Clearing**: Available via REST (`POST /api/system/audit/clear`) and MCP (`clear_audit_log`).

---

## 8. Multi-Surface Integration

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

| Precondition State | Status | Cause | Description |
|:---|:---|:---|:---|
| Uninitialized / Preflight failed | `false` | `Cause::InternalError` | Storage full or task creation failed |
| Disabled via config | `false` | `Cause::Unavailable` | Audit logging disabled in configuration |
| Dropping or throttling | `true` | `Cause::FallbackEngaged` | Queue full or rate limit exceeded |
| Normal operation | `true` | `Cause::Met` | Audit logging active |

> [!NOTE]
> The `FallbackEngaged` state is **sticky** — once a single drop or throttle occurs, the precondition stays degraded until `clearAll()` is called (which resets counters). This is intentional: an audit trail should surface historical reliability problems, not silently recover.

---

## 9. Anti-Patterns & Best Practices

> [!CAUTION]
> Avoid these common audit anti-patterns:

- ❌ **Directly including `AuditModule.hpp` in shared modules**
  - *Problem*: Creates hard coupling to `AuditModule` and causes compile errors if `WITH_AUDIT` is disabled.
  - *Fix*: Always `#include "Audit.h"` and use the `AUDIT()` macro.
- ❌ **Calling `AUDIT()` inside Interrupt Service Routines (ISRs)**
  - *Problem*: `AUDIT()` allocates memory and calls FreeRTOS queue primitives that are not ISR-safe.
  - *Fix*: Defer audit logging to a task or EventBus handler.
- ❌ **Calling `AUDIT_SYNC()` in tick loops or EventBus callbacks**
  - *Problem*: `AUDIT_SYNC()` blocks the calling thread until flash write completes (up to 5s). This will stall the main loop and trigger the Task Watchdog Timer (TWDT).
  - *Fix*: Use asynchronous `AUDIT()` in tick loops and EventBus subscribers. Reserve `AUDIT_SYNC()` for deliberate shutdown/handshake sequences in dedicated tasks.
- ❌ **Logging high-frequency sensor readings as audit events**
  - *Problem*: Rapid writes quickly exhaust the 120/min rate limit and cause premature flash wear.
  - *Fix*: Audit logs are for administrative, operational, and security events. Use telemetry channels, display buffers, or debug logs for high-frequency measurements.
