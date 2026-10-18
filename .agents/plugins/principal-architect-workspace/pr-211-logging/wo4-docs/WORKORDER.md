---
name: pr-211-logging-wo4
description: Handoff instructions for Work Order 4 — Documentation realignment
complexity: mechanical
---

# Role Definition

> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Realign all documentation to reflect the new logging architecture: `Redactor` (Singleton with `ISanitizer`), `LogPipeline` (vprintf hook + 2 KB format buffer), and the deletion of `Logger`/`LOG_SECURE`. Update the design specification, framework-expert skill files, and agent skill references.

# Parent Project

`pr-211-logging` — [PROJECT.md](../PROJECT.md). Human approval recorded 2026-09-01T20:35:07+01:00.

# Strict Boundary Box

- **IN SCOPE:** `docs/design_specification.md`, `.agents/skills/framework-expert/SKILL.md`, `.agents/skills/framework-expert/embedded-safety/thread-safety.md`, `.agents/skills/framework-expert/references/audit-module-guide.md`, `.agents/skills/framework-expert/references/architecture.md`
- **OUT OF SCOPE:** All source code files (`lib/`, `modules/`, `test/`, `include/`). Do NOT modify any code.
- **Do NOT file GitHub issues.** Surface out-of-scope findings in Completion Report.

# Tooling Constraints

- No build or test tools needed — this is documentation only.
- Follow house-style documentation conventions.

# Context: What Changed

The following changes have been made in WO1–WO3 that documentation must reflect:

1. **`Redactor`** promoted from static utility class to Meyer's Singleton implementing `ISanitizer`. Fixed-capacity lock-free array (16 secrets max, reject < 4 chars). Zero-allocation in-place redaction (`strstr` + `memset`). Thread-safe: lock-free reads, writer spinlock. Lives in `lib/Redactor/`.

2. **`ISanitizer`** new interface at `include/interfaces/ISanitizer.hpp`: `virtual void sanitize(char* buf, size_t len) = 0`. AuditModule depends on this abstraction via DI.

3. **`LogPipeline`** new library at `lib/LogPipeline/`. Installs `esp_log_set_vprintf()` hook. 2 KB static format buffer in `.bss`. `thread_local` re-entrancy guard. Redacts ALL `ESP_LOG*` output globally. Installed as Step 1b in `Application::begin()`.

4. **`Logger` DELETED** — `lib/Logger/` removed entirely. `LOG_SECURE` macro no longer exists. `Logger::registerSecret()` replaced by `Redactor::instance().registerSecret()`.

5. **Caller migrations:** DeviceCrypto uses `ESP_LOGD` instead of `LOG_SECURE`. WifiManager and KeyManager call `Redactor::instance().registerSecret()` directly. HomeAssistant dead include removed.

6. **Ring buffer, ILogSink, MCP log tools** — all DEFERRED to future PR. Not implemented.

# Line-Item Instructions

## 1. Update `docs/design_specification.md`

- [ ] Find §11A (the Logging & Observability Architecture section). Search for "Logger" and "LogRouter" references.

- [ ] Update the module inventory (§4.1 or wherever the directory tree is listed):
  - Remove `lib/logger/` (or `lib/Logger/`) entry.
  - Add `lib/LogPipeline/` entry with description: "Global log interception pipeline (vprintf hook + secret redaction)".
  - Update `lib/Redactor/` description to: "Singleton secret registry with zero-allocation in-place redaction. Implements ISanitizer."
  - Add `include/interfaces/ISanitizer.hpp` if interfaces are listed.

- [ ] Update §11A content to reflect what IS implemented vs what is deferred:
  - §11A.1-3 (LogShim, ESP_LOGx, level control) — no change needed, these are still accurate.
  - §11A.4 (LogRouter pipeline) — update to describe `LogPipeline` (the actual implementation). Mention the 2 KB static format buffer, `esp_log_set_vprintf()` hook, `thread_local` re-entrancy guard, and that it calls `Redactor::instance().redact()` on every log line.
  - §11A.5 (ILogSink) — mark as DEFERRED / not yet implemented. Note that the interface will be added when consumers exist (MCP log tools, web portal log viewer).
  - §11A.6 (Ring buffer) — mark as DEFERRED / not yet implemented. Note the decision to defer (serial monitor + log file is current workflow; ring buffer is RAM lost on crash).
  - §11A.7 (Heartbeat) — no change if still accurate.
  - §11A.8 (MQTT) — no change, correctly deferred.

- [ ] Update the Decision Log entries (D27–D32 area):
  - Add a new decision entry or update existing ones to record:
    - `Logger` deleted, replaced by `LogPipeline`.
    - `Redactor` promoted to Singleton with `ISanitizer`.
    - Ring buffer and ILogSink deferred.
    - 2 KB static format buffer chosen over stack buffer (covers HTTP auth headers).
    - Reference: Issue #211.

- [ ] Search for any other mentions of `Logger`, `LOG_SECURE`, or `logSecure` in the design spec and update or remove them.

## 2. Update `.agents/skills/framework-expert/SKILL.md`

- [ ] Search for `Redactor` in the SKILL.md description/keywords. Update to reflect:
  - `Redactor` is now a Singleton accessed via `Redactor::instance()`.
  - `registerSecret()` is an instance method returning `bool`.
  - `ISanitizer` is the DIP interface for modules needing redaction.
  - `LogPipeline` is the new vprintf hook library.
  - Remove any references to `Logger`, `LOG_SECURE`, or `Logger.hpp`.
  - Add `LogPipeline` to the keyword triggers if appropriate.

## 3. Update `.agents/skills/framework-expert/embedded-safety/thread-safety.md`

- [ ] Find the Redactor thread-safety section. Update to reflect:
  - Redactor is now thread-safe: lock-free reads (`std::memory_order_acquire`), writer-side spinlock (`portMUX_TYPE` / `std::mutex`).
  - Fixed capacity (16 secrets max) — no vector reallocation race.
  - The old thread-safety contract ("init-only registration") is SUPERSEDED — runtime registration is now safe.
  - Reference `ISanitizer` interface for modules that receive redaction via DI.

## 4. Update `.agents/skills/framework-expert/references/audit-module-guide.md`

- [ ] Update the redaction section to reflect:
  - AuditModule now receives `ISanitizer*` via `begin(ISanitizer* sanitizer)`.
  - Fallback to `Redactor::instance()` if no sanitizer injected.
  - The old `Redactor::redact(record->details, sizeof(record->details))` is replaced by `_sanitizer->sanitize(...)`.
  - This makes AuditModule testable with a mock sanitizer.

## 5. Update `.agents/skills/framework-expert/references/architecture.md`

- [ ] If this file references the logging architecture or Logger, update to reflect:
  - `LogPipeline` replaces `Logger` as the logging infrastructure.
  - `Redactor` Singleton provides shared secret redaction for both `LogPipeline` and `AuditModule`.
  - `ISanitizer` interface enables DIP for modules.

# Verification Requirement

No build verification needed — documentation only. Read through each modified file to verify consistency. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

# Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete. All 5 in-scope documentation files updated, cross-referenced, and validated.

### Changes Made
1. **`docs/design_specification.md`**:
   - Updated architecture block diagram (§3.1) and component table (§4.1) to replace `logger` and `LogRouter` with `LogPipeline` (`lib/LogPipeline/`) and `Redactor` (`lib/Redactor/`).
   - Rewrote §11A.4 to document `LogPipeline` (vprintf hook installed at Step 1b in `Application::begin()`, 2 KB `.bss` buffer, `thread_local` re-entrancy guard, in-place redaction via `Redactor::instance().redact()`).
   - Marked §11A.5 (`ILogSink`) and §11A.6 (RAM Ring Buffer & Crash Persistence) as deferred (Issue #211).
   - Updated §11A.8 MQTT telemetry reference to `LogPipeline`.
   - Updated §12.1 Directory Tree with `ISanitizer.hpp`, `LogPipeline/`, `Redactor/` and removed `logger/`.
   - Updated §16.3 Mock Strategy for `LogPipeline` / `LogShim`.
   - Updated Decision Log: refined D28, marked D29/D30 as deferred, and added D39 documenting unified secret redaction via `Redactor` Singleton + `ISanitizer` DIP interface.
2. **`.agents/skills/framework-expert/SKILL.md`**:
   - Added `LogPipeline`, `ISanitizer`, `Redactor::instance()`, `registerSecret` to frontmatter description.
   - Added `ISanitizer` to the Interface Class table.
   - Added Logging & Redaction triggers to Trigger Reference; purged legacy `Logger` mentions.
3. **`.agents/skills/framework-expert/embedded-safety/thread-safety.md`**:
   - Updated Redactor thread-safety bullet: lock-free reads (`std::memory_order_acquire`), writer spinlock, fixed capacity (16 secrets max), and noted the old init-only constraint is superseded.
   - Referenced `ISanitizer` DIP interface for injectable secret redaction.
4. **`.agents/skills/framework-expert/references/audit-module-guide.md`**:
   - Updated §1 Overview and §10 Thread Safety to document `ISanitizer` injection via `begin(ISanitizer* sanitizer)` (defaulting to `&Redactor::instance()`).
   - Replaced §12 anti-pattern with concrete `Redactor` coupling anti-pattern and DIP recommendation.
5. **`.agents/skills/framework-expert/references/architecture.md`**:
   - Updated 3-Tier Model with `LogPipeline`, `Redactor`, and `ISanitizer`.
   - Updated BOOT Phase Step 1 to reflect `LogPipeline` installation early in boot.
   - Added dedicated `Logging & Secret Sanitization` reference section.

### Decisions Made
- Replaced all historical mentions of `Logger` and `LogRouter` with `LogPipeline` and `Redactor` across living documentation.
- Explicitly documented the deferred status and architectural reasoning for `ILogSink` and RAM ring buffer in §11A.5, §11A.6, D29, and D30.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.
