---
name: pr-194-audit-module-wo3
description: Handoff instructions for Work Order 3 — Board constants for BOARD_AUDIT_BYTES
complexity: mechanical
---

# WO3 — Board Constants + Flash Budget

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Add `BOARD_AUDIT_BYTES` to each board definition and provide a sensible fallback in `board_contract.h`. The AuditModule (already implemented in WO2) uses `BOARD_AUDIT_BYTES` via `#ifdef BOARD_AUDIT_BYTES` to set its flash budget. The fallback provides a safe default; board-specific overrides tune it for the available storage partition.

## Context
**Parent project**: `pr-194-audit-module` — see `.agents/skills/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

**Storage partition sizes** (from `hardware/*/partitions.csv`):
| Board | Storage Partition |
|---|---|
| `esp32dev` | 128 KiB (0x20000) |
| `_TEMPLATE` | 128 KiB (0x20000) |
| `esp32s3nano` | 2.75 MiB (0x2C0000) |
| `crowpanel-128-rotary` | 2.75 MiB (0x2C0000) |

**Budget sizing rule**: Audit budget must be a small fraction of the storage partition to leave space for other files (timezones.json, cached data, etc). Conservative defaults:
- 4 MB boards (128 KiB storage): `16384` (16 KiB) — leaves 112 KiB for other files
- 16 MB boards (2.75 MiB storage): `65536` (64 KiB) — leaves ~2.7 MiB for other files

## Strict Boundary Box
- **DO NOT** modify any files outside `hardware/`.
- **DO NOT** modify `modules/AuditModule/` — it already reads `BOARD_AUDIT_BYTES` via `#ifdef`.
- **DO NOT** modify `platformio.ini`, `lib/`, `include/`, `modules/`, `src/`, or `test/`.

## Tooling Constraints
- Use `platformio` MCP tools for building (`build_project`), NOT raw `pio` CLI commands.

## Line-Item Instructions

### 1. Modify `hardware/_common/board_contract.h`
- [ ] Add a new section after the WebServer Configuration Fallbacks section (after line ~19):
  ```cpp
  // ── Audit Budget Fallback ─────────────────────────────────────────────────
  #ifndef BOARD_AUDIT_BYTES
  #define BOARD_AUDIT_BYTES 32768              // 32 KiB default; boards override in board.h
  #endif
  ```
  This provides a safe default that the AuditModule's `#ifdef BOARD_AUDIT_BYTES` will pick up.

### 2. Modify `hardware/esp32dev/board.h`
- [ ] Add after the "Identity & capabilities" section (after line ~84, before the Display section):
  ```cpp
  // ── Audit budget (storage partition = 128 KiB) ────────────────────────────
  #define BOARD_AUDIT_BYTES 16384              // 16 KiB — conservative for 128 KiB storage
  ```

### 3. Modify `hardware/esp32s3nano/board.h`
- [ ] Add in the appropriate section:
  ```cpp
  // ── Audit budget (storage partition = 2.75 MiB) ───────────────────────────
  #define BOARD_AUDIT_BYTES 65536              // 64 KiB — comfortable for 2.75 MiB storage
  ```

### 4. Modify `hardware/crowpanel-128-rotary/board.h`
- [ ] Add in the appropriate section:
  ```cpp
  // ── Audit budget (storage partition = 2.75 MiB) ───────────────────────────
  #define BOARD_AUDIT_BYTES 65536              // 64 KiB — comfortable for 2.75 MiB storage
  ```

### 5. Modify `hardware/_TEMPLATE/board.h`
- [ ] Add a commented section as guidance for new boards:
  ```cpp
  // ── Audit budget (optional — override default in board_contract.h) ────────
  // #define BOARD_AUDIT_BYTES 32768           // Total bytes for audit log files
  //                                          // Default: 32 KiB (see board_contract.h)
  //                                          // 4 MB boards: use 16384 (16 KiB)
  //                                          // 16 MB boards: use 65536 (64 KiB)
  ```

### 6. Build verification
- [ ] Build with `platformio` MCP `build_project` for `esp32dev` — verify `BOARD_AUDIT_BYTES=16384` is picked up (no compile errors)
- [ ] Build with `platformio` MCP `build_project` for `esp32s3nano` — verify `BOARD_AUDIT_BYTES=65536` is picked up
- [ ] Build with `platformio` MCP `build_project` for `crowpanel-128-rotary` — verify clean build

## Verification Requirement
Before committing, the worker MUST:
1. Confirm all 3 board environments build cleanly.
2. Ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
