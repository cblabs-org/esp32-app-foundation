---
name: pr-211-logging-wo3
description: Handoff instructions for Work Order 3 — Logger deletion + caller migration + test consolidation
complexity: standard
---

# Role Definition

> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Delete the redundant `lib/Logger/` library and migrate all callers to use `Redactor::instance()` directly. The `LOG_SECURE` macro and `Logger::logSecure()` are obsolete now that `LogPipeline` redacts **all** log output globally. Convert `LOG_SECURE(tag, fmt, ...)` → `ESP_LOGD(tag, fmt, ...)` and `Logger::registerSecret(...)` → `Redactor::instance().registerSecret(...)`. Delete the `test_logger` test suite. Consolidate the `test_redactor` suite.

# Parent Project

`pr-211-logging` — [PROJECT.md](../PROJECT.md). Human approval recorded 2026-09-01T20:35:07+01:00.

# Strict Boundary Box

- **IN SCOPE:** `lib/Logger/` (DELETE entire directory), `lib/DeviceCrypto/DeviceCrypto.cpp`, `modules/WifiManager/WifiManager.cpp`, `modules/KeyManager/KeyManager.cpp`, `modules/HomeAssistant/HomeAssistantModule.cpp`, `test/test_unit/test_logger/` (DELETE entire directory), `test/test_unit/test_redactor/test_redactor.cpp`
- **OUT OF SCOPE:** `lib/Redactor/` (WO1 — done, do NOT modify), `lib/LogPipeline/` (WO2 — done, do NOT modify), `modules/Application/` (do NOT modify), `modules/AuditModule/` (WO1 — done), docs (WO4).
- **Do NOT file GitHub issues.** Surface out-of-scope findings in Completion Report.

# Tooling Constraints

- Use PlatformIO MCP tools (`build_project`, `run_tests`) rather than raw `pio` CLI when available.
- Build verification: `pio run -e esp32dev` (hardware) and `pio test -e native` (unit tests).
- Follow house-style documentation (read `.agents/skills/house-style-docs/SKILL.md` if needed for header format).

# Line-Item Instructions

## 1. Delete `lib/Logger/`

- [x] Delete `lib/Logger/Logger.hpp` and `lib/Logger/Logger.cpp` (and any `library.json` if present).
- [x] Remove the entire `lib/Logger/` directory.

## 2. Migrate `lib/DeviceCrypto/DeviceCrypto.cpp`

- [x] Remove `#include "Logger.hpp"` (currently at the top of the file — find it).
- [x] Convert all 7 `LOG_SECURE(tag, fmt, ...)` calls to `ESP_LOGD(tag, fmt, ...)`:
  - Line 53: `LOG_SECURE("CRYPTO", "Generated and firmly committed...")` → `ESP_LOGD("CRYPTO", "Generated and firmly committed...")`
  - Line 56: same pattern
  - Line 109: same pattern
  - Line 148: same pattern
  - Line 181: same pattern
  - Line 234: same pattern
  - Line 240: same pattern
  - **Use ESP_LOGD** (debug level), not ESP_LOGI. These are crypto internals — useful for debugging, stripped from production builds.
- [x] Add `#include "platform/LogShim.h"` if not already present (needed for `ESP_LOGD` macro).

## 3. Migrate `modules/WifiManager/WifiManager.cpp`

- [x] Remove `#include "Logger.hpp"` (line 56).
- [x] Line 845: Change `Logger::registerSecret(std::string(_wifiPass));` to `Redactor::instance().registerSecret(std::string(_wifiPass));`
- [x] Add `#include "Redactor.h"` if not already present.
- [x] **Note on this call:** Per the architectural decision, this registerSecret call is actually REDUNDANT — KeyManager auto-registers secrets on decrypt, and WifiManager gets its saved password from KeyManager. However, this call covers the case where the password arrives from the HTTP form POST (before it's stored in KeyManager). So KEEP it — it's a belt-and-suspenders registration for the password's first appearance.

## 4. Migrate `modules/KeyManager/KeyManager.cpp`

- [x] Remove `#include "Logger.hpp"` (line 14).
- [x] Line 168: Change `Logger::registerSecret(std::string(result.c_str()));` to `Redactor::instance().registerSecret(std::string(result.c_str()));`
- [x] Add `#include "Redactor.h"` if not already present.

## 5. Clean up `modules/HomeAssistant/HomeAssistantModule.cpp`

- [x] Remove `#include "Logger.hpp"` (line 36). This is a dead include — HomeAssistantModule doesn't use Logger at all.
- [x] No other changes needed in this file.

## 6. Delete `test/test_unit/test_logger/`

- [x] Delete the entire `test/test_unit/test_logger/` directory.
- [x] The tests in test_logger tested Logger::logSecure() and LOG_SECURE — both are deleted. These tests are obsolete.

## 7. Verify no remaining references to Logger

- [x] Run a grep/search for `Logger` (case-sensitive) across the entire codebase to verify no remaining includes, usages, or references to the deleted class. Common patterns to check:
  - `#include "Logger.hpp"` or `#include "Logger.h"`
  - `Logger::registerSecret` or `Logger::logSecure`
  - `LOG_SECURE`
- [x] The only remaining references should be in:
  - Historical/documentation files (OK to leave)
  - The WORKORDER.md files in the PA workspace (OK to leave)
  - AppFoundation.h should NOT include Logger (verify)

## 8. Build verification

- [x] Run `pio test -e native` — test count should DROP by the number of tests that were in test_logger, but all remaining tests must pass.
- [x] Run `pio run -e esp32dev` — firmware must compile cleanly.
- [x] Note the new test count in your completion report.

# Verification Requirement

The worker must verify both native tests pass and firmware compiles. **CRITICAL:** Before committing, the worker MUST ask the user if they are happy with the changes.

# Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report

### Status
Complete. All line items executed and verified. 855 native tests pass (8 tests retired with `test_logger`), firmware builds cleanly for `esp32dev`.

### Changes Made
- Deleted `lib/Logger/` directory (`Logger.hpp`, `Logger.cpp`, `library.json`).
- In `lib/DeviceCrypto/DeviceCrypto.cpp`:
  - Removed `#include "../Logger/Logger.hpp"`.
  - Converted 7 `LOG_SECURE` invocations to `ESP_LOGD("CRYPTO", ...)`.
- In `modules/WifiManager/WifiManager.cpp`:
  - Replaced `#include "Logger.hpp"` with `#include "Redactor.h"`.
  - Migrated `Logger::registerSecret(std::string(_wifiPass))` to `Redactor::instance().registerSecret(std::string(_wifiPass))`.
- In `modules/KeyManager/KeyManager.cpp`:
  - Replaced `#include "Logger.hpp"` with `#include "Redactor.h"`.
  - Migrated `Logger::registerSecret(std::string(result.c_str()))` to `Redactor::instance().registerSecret(std::string(result.c_str()))`.
- In `modules/HomeAssistant/HomeAssistantModule.cpp`:
  - Removed dead `#include "Logger.hpp"`.
- Deleted `test/test_unit/test_logger/` directory (`test_logger.cpp`).
- Verified zero remaining `Logger` / `LOG_SECURE` symbols in `src/`, `include/`, `lib/`, `modules/`, `hardware/`, and `test/`.

### Decisions Made
- Converted `LOG_SECURE` call sites in `DeviceCrypto.cpp` to `ESP_LOGD` (debug level) rather than `ESP_LOGI`, preserving debug utility while stripping crypto internals from production builds.
- Retained explicit secret registration in `WifiManager::updateWiFi` to ensure web form inputs are redacted immediately upon arrival.

### Dead Ends & Warnings
- None encountered.

### Out-of-Scope Findings
- None.

### Blockers
- None.
