---
name: pr-194-audit-module-wo1
description: Handoff instructions for Work Order 1 — lib/Redactor extraction
complexity: standard
---

# WO1 — Extract `lib/Redactor` from `lib/Logger`

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Extract the secret registry and redaction logic from `lib/Logger` into a new dependency-free `lib/Redactor` leaf library. Both `Logger` and the future `AuditModule` will depend on `Redactor` for shared secret redaction. Neither should depend on the other.

## Context
This is a prerequisite for RFC #194 (AuditModule). The AuditModule needs to redact secrets from audit records before writing them to flash. Today, redaction lives inside `lib/Logger` as a private method. We need to extract it so both Logger and AuditModule can share one secret registry without coupling to each other.

**Parent project**: `pr-194-audit-module` — see `.agents/plugins/principal-architect-workspace/pr-194-audit-module/PROJECT.md`

## Strict Boundary Box
- **DO NOT** create the AuditModule — that is WO2.
- **DO NOT** modify any files outside `lib/Logger/`, `lib/Redactor/`, and `test/test_unit/`.
- **DO NOT** touch `include/`, `modules/`, `hardware/`, `docs/`, or `platformio.ini`.
- **DO NOT** modify existing `registerSecret()` call sites in `WifiManager.cpp` or `KeyManager.cpp` — they must continue to work unchanged via `Logger::registerSecret()`.
- **DO NOT** modify existing `LOG_SECURE` / `logSecure()` call sites in `DeviceCrypto.cpp`.

## Tooling Constraints
- Use `platformio` MCP tools for building and testing (e.g., `build_project`, `run_tests`), NOT raw `pio` CLI commands.
- Use the `agent_validate_project` MCP tool to check project health after changes.

## Line-Item Instructions

### 1. Create `lib/Redactor/Redactor.h`
- [x] Create the file with the Apache-2.0 header matching the project style (see `lib/Logger/Logger.hpp` for format).
- [x] Define a `Redactor` class with **public static** methods:
  - `static void registerSecret(const std::string& secret);`
  - `static void registerSecret(const String& secret);`
  - `static void registerSecret(const char* secret);`
  - `static void redact(char* buffer, size_t bufferSize);` — Public version of what was `redactInPlace`. Takes a buffer size parameter (the old code hardcoded 512).
- [x] Private members:
  - `static std::vector<std::string> _secrets;`
- [x] Include guards: `#ifndef REDACTOR_H` / `#define REDACTOR_H`
- [x] Includes: `<Arduino.h>`, `<vector>`, `<string>`

### 2. Create `lib/Redactor/Redactor.cpp`
- [x] Move the secret storage (`std::vector<std::string> secrets`) and all three `registerSecret()` implementations from `Logger.cpp` into `Redactor.cpp`.
- [x] Move the `redactInPlace()` logic from `Logger.cpp` into `Redactor::redact(char* buffer, size_t bufferSize)`.
  - **Important**: The old code hardcoded `512 - 1` in the `strncpy`. Use the new `bufferSize` parameter instead: `strncpy(buffer, message.c_str(), bufferSize - 1); buffer[bufferSize - 1] = '\0';`
- [x] The deduplication check in `registerSecret` (skip if already present) must be preserved.

### 3. Modify `lib/Logger/Logger.hpp`
- [x] Add `#include "Redactor.h"` at the top (after existing includes).
- [x] Remove the private `static std::vector<std::string> secrets;` member.
- [x] Remove the private `static void redactInPlace(char* buffer);` declaration.
- [x] Keep all three `registerSecret()` public declarations — they become delegating shims.
- [x] Keep `logSecure()` declaration unchanged.
- [x] Keep the `LOG_SECURE` macro unchanged.

### 4. Modify `lib/Logger/Logger.cpp`
- [x] Remove the static `std::vector<std::string> Logger::secrets;` definition (line 16).
- [x] Replace all three `Logger::registerSecret()` implementations with one-line delegations to `Redactor::registerSecret()`:
  ```cpp
  void Logger::registerSecret(const std::string& secret) { Redactor::registerSecret(secret); }
  void Logger::registerSecret(const String& secret)      { Redactor::registerSecret(secret); }
  void Logger::registerSecret(const char* secret)        { Redactor::registerSecret(secret); }
  ```
- [x] In `Logger::logSecure()`, replace `redactInPlace(buffer);` with `Redactor::redact(buffer, sizeof(buffer));`
- [x] Remove the entire `Logger::redactInPlace()` method.

### 5. Create `test/test_unit/test_redactor/test_redactor.cpp`
- [x] Create unit tests for the `Redactor` class directly, covering:
  - `redact()` with no secrets registered (buffer unchanged)
  - `registerSecret()` + `redact()` replaces secret with asterisks
  - Multiple secrets registered, all redacted
  - Empty secret string is ignored
  - Buffer longer than secret works correctly
  - Deduplication: registering the same secret twice doesn't double-count
- [x] Use the same gtest pattern as `test/test_unit/test_logger/test_logger.cpp`.

### 6. Verify existing Logger tests still pass
- [x] Run `test/test_unit/test_logger/` — all 7 existing tests must pass unchanged. The Logger shim delegates to Redactor, so the observable behaviour is identical.

### 7. Build verification
- [x] Build the project using the `platformio` MCP `build_project` tool for the `native_test` environment (unit tests).
- [x] Build for `esp32dev` to verify the firmware compiles.

## Verification Requirement
Before committing, the worker MUST:
1. Confirm all existing Logger tests pass.
2. Confirm the new Redactor tests pass.
3. Confirm `esp32dev` firmware builds cleanly.
4. Ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete. All instructions executed, unit tests passing (851/851), and `esp32dev` build verified.

### Changes Made
- Created `lib/Redactor/Redactor.h` and `lib/Redactor/Redactor.cpp` encapsulating secret registry and in-place asterisk masking.
- Created `lib/Redactor/library.json` declaring the new leaf library.
- Refactored `lib/Logger/Logger.hpp` and `lib/Logger/Logger.cpp` to delegate secret registration and redaction to `Redactor`.
- Created `test/test_unit/test_redactor/test_redactor.cpp` covering 7 test scenarios for `Redactor`.

### Decisions Made
- Added defensive check in `Redactor::redact` for null pointer and zero buffer size to prevent underflow.
- Retained Logger shims with identical signatures for backward compatibility across existing call sites.

### Dead Ends & Warnings
None.

### Out-of-Scope Findings
None.

### Blockers
None.
