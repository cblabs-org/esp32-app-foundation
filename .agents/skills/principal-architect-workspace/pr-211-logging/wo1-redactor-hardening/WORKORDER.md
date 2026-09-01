---
name: pr-211-logging-wo1
description: Handoff instructions for Work Order 1 — Redactor Hardening + OO Redesign
complexity: architect
---

# Role Definition

> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Harden the `Redactor` class from a static utility with thread-safety violations into a robust Singleton with fixed-capacity lock-free reads, zero-allocation in-place redaction, and an `ISanitizer` interface for AuditModule dependency injection. This is the foundation that `LogPipeline` (WO2) will build on.

# Parent Project

`pr-211-logging` — [PROJECT.md](../PROJECT.md). Human approval recorded 2026-09-01T20:35:07+01:00.

# Strict Boundary Box

- **IN SCOPE:** `lib/Redactor/`, `include/interfaces/ISanitizer.hpp` (new), `modules/AuditModule/AuditModule.hpp`, `modules/AuditModule/AuditModule.cpp`, `test/test_unit/test_redactor/test_redactor.cpp`
- **OUT OF SCOPE:** `lib/Logger/` (WO3), `lib/LogPipeline/` (WO2), `modules/Application/` (WO2), `modules/WifiManager/` (WO3), `modules/KeyManager/` (WO3), `docs/` (WO4), `.agents/skills/` (WO4). Do NOT delete Logger or create LogPipeline — those are later WOs.
- **Do NOT file GitHub issues.** Surface any out-of-scope findings in the Completion Report.

# Tooling Constraints

- Use PlatformIO MCP tools (`build_project`, `run_tests`) rather than raw `pio` CLI when available.
- Build verification: `pio run -e esp32dev` (hardware) and `pio test -e native` (unit tests).
- Follow house-style documentation (read `.agents/skills/house-style-docs/SKILL.md` if needed).

# Line-Item Instructions

## 1. Create `ISanitizer` interface

- [ ] Create `include/interfaces/ISanitizer.hpp`:
  ```cpp
  #pragma once
  #include <cstddef>

  class ISanitizer {
  public:
      virtual ~ISanitizer() = default;
      virtual void sanitize(char* buffer, size_t bufferSize) = 0;
  };
  ```
  Follow the house-style header comment format (see other files in `include/interfaces/`).

## 2. Redesign `lib/Redactor/Redactor.h`

- [ ] Replace the entire class with a Singleton design:
  - Remove `#include <Arduino.h>` — replace with `#include <string>` and `#include <atomic>`. Use `#include <cstddef>` for `size_t`.
  - Add `#include "interfaces/ISanitizer.hpp"`.
  - Class `Redactor` implements `ISanitizer`.
  - `static Redactor& instance()` — Meyer's Singleton (thread-safe by C++11 guarantee).
  - `static constexpr size_t MAX_SECRETS = 16;`
  - `bool registerSecret(const char* secret)` — instance method, returns false if full or too short.
  - `bool registerSecret(const std::string& secret)` — delegates to the `const char*` overload.
  - `void redact(char* buffer, size_t bufferSize)` — instance method.
  - `void sanitize(char* buffer, size_t bufferSize) override` — delegates to `redact()`.
  - `size_t secretCount() const` — returns current count.
  - `void clearSecrets()` — resets count to 0 (for test teardown).
  - **Remove** the `String&` overload (Arduino dependency). Callers can use `.c_str()`.
  - Private member variables:
    - `std::string _secrets[MAX_SECRETS]` — fixed-capacity array, no vector.
    - `std::atomic<size_t> _count{0}` — acquire/release for lock-free reads.
    - On ESP32 (`#ifndef NATIVE_BUILD`): `portMUX_TYPE _writerLock` for writer-side spinlock.
    - On native: `std::mutex _writerMutex` for writer-side lock.
  - Private constructor (Singleton).
  - Delete copy/move constructors and assignment operators.

## 3. Rewrite `lib/Redactor/Redactor.cpp`

- [ ] Implement the new Redactor:
  - `instance()`: standard Meyer's Singleton pattern.
  - `registerSecret(const char* secret)`:
    - Null/empty guard → return false.
    - Minimum length guard: `strlen(secret) < 4` → return false (short secrets mask legitimate content).
    - Acquire writer lock.
    - Deduplicate: iterate `_secrets[0.._count)`, return true if already present.
    - Cap check: `_count >= MAX_SECRETS` → log warning via raw `printf` (NOT `ESP_LOG*` — avoid recursion), release lock, return false.
    - Store: `_secrets[_count.load(std::memory_order_relaxed)] = secret;`
    - Increment: `_count.fetch_add(1, std::memory_order_release);`
    - Release writer lock.
    - Return true.
  - `registerSecret(const std::string&)`: delegate to `registerSecret(s.c_str())`.
  - `redact(char* buffer, size_t bufferSize)`:
    - Null/zero guard.
    - `size_t count = _count.load(std::memory_order_acquire);` — lock-free snapshot.
    - `size_t bufLen = strnlen(buffer, bufferSize);`
    - For each `_secrets[i]` where `i < count`:
      - Use `strstr()` to find occurrences in `buffer`.
      - For each match: `memset(matchPos, '*', secretLen)` — in-place, zero-allocation.
      - Continue scanning from `matchPos + secretLen` for additional occurrences.
    - **No `std::string` construction. No heap allocation. Pure C string operations.**
  - `sanitize()`: simply calls `redact()`.
  - `secretCount()`: return `_count.load(std::memory_order_acquire)`.
  - `clearSecrets()`: acquire writer lock, set `_count.store(0, std::memory_order_release)`, release lock.

## 4. Wire AuditModule to ISanitizer

- [ ] In `modules/AuditModule/AuditModule.hpp`:
  - Replace `#include "Redactor.h"` with `#include "interfaces/ISanitizer.hpp"`.
  - Add a private member: `ISanitizer* _sanitizer = nullptr;`
  - Change `begin()` signature to `bool begin(ISanitizer* sanitizer = nullptr);`
  - The `= nullptr` default preserves backward compat for any existing callers.

- [ ] In `modules/AuditModule/AuditModule.cpp`:
  - Add `#include "Redactor.h"` (needed for fallback if no sanitizer injected).
  - In `begin()`: store the injected sanitizer. If nullptr, fallback to `&Redactor::instance()`.
  - At line 396 (or equivalent), change `Redactor::redact(record->details, sizeof(record->details));` to `if (_sanitizer) _sanitizer->sanitize(record->details, sizeof(record->details));`.

- [ ] In `modules/Application/Application.cpp` where `auditModule.begin()` is called (inside `#ifdef WITH_AUDIT` block):
  - Change to `auditModule.begin(&Redactor::instance());` — inject the Singleton as ISanitizer.
  - Add `#include "Redactor.h"` at the top of Application.cpp if not already present.

## 5. Update test_redactor.cpp

- [ ] The existing tests use `Redactor::registerSecret(...)` and `Redactor::redact(...)` as static calls. Update ALL calls to use the Singleton instance:
  - `Redactor::registerSecret("x")` → `Redactor::instance().registerSecret("x")`
  - `Redactor::redact(buf, sz)` → `Redactor::instance().redact(buf, sz)`
  - The `String(...)` overload tests should be removed (Arduino String dependency removed).

- [ ] Add `Redactor::instance().clearSecrets()` to the `SetUp()` method so each test starts clean.

- [ ] Add new test cases:
  - `TEST_F(RedactorTest, MaxSecretsCapEnforced)` — register 16 secrets, 17th returns false.
  - `TEST_F(RedactorTest, SecretCountReturnsCorrectValue)` — register 3, assert `secretCount() == 3`.
  - `TEST_F(RedactorTest, ShortSecretRejected)` — register "ab" (2 chars), assert returns false and secretCount is 0.
  - `TEST_F(RedactorTest, SingletonIdentity)` — `&Redactor::instance() == &Redactor::instance()`.
  - `TEST_F(RedactorTest, ClearSecretsResetsCount)` — register 5, clear, assert secretCount == 0, verify no redaction occurs.
  - `TEST_F(RedactorTest, SanitizeInterfaceDelegates)` — Cast `Redactor::instance()` to `ISanitizer*`, register a secret via the Redactor, call `sanitize()` via the interface, verify redaction works.

## 6. Build verification

- [ ] Run `pio test -e native` — all tests must pass (existing + new).
- [ ] Run `pio run -e esp32dev` — firmware must compile cleanly.

# Verification Requirement

The worker must verify both native tests pass and firmware compiles. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

# Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status

### Changes Made

### Decisions Made

### Dead Ends & Warnings

### Out-of-Scope Findings

### Blockers
