---
name: pr-05c-wo1
description: Handoff instructions for Work Order 1 — Kernel service tests (wifiManager, timeManager, otaUpdateManager)
---

# PR-05c / WO1 — Kernel Service Tests

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Run these view_file calls as your first five actions, in this exact order:
1. `view_file .agents/skills/pio-manager/SKILL.md`
2. `view_file .agents/skills/embedded-systems/SKILL.md`
3. `view_file .agents/skills/house-style-docs/SKILL.md`
4. `view_file .agents/skills/oo-expert/SKILL.md`
5. `view_file .agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT read any project code or begin work until all five are loaded.

---

## Objective

Create comprehensive GoogleTest suites for three kernel services that currently have **zero** test coverage:

| Module | Target | Path |
|---|---|---|
| `wifiManager` | 15+ tests | `test/test_unit/test_wifiManager/test_wifiManager.cpp` |
| `timeManager` | 10+ tests | `test/test_unit/test_timeManager/test_timeManager.cpp` |
| `otaUpdateManager` | 15+ tests | `test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp` |

---

## Strict Boundary Box

- **DO NOT** modify any file under `modules/` or `lib/` EXCEPT to add or fix `NATIVE_BUILD` preprocessor guards that are required for the test to compile.
- **DO NOT** modify `platformio.ini`.
- **DO NOT** delete or modify any existing test files in `test/test_unit/`.
- **DO NOT** modify any existing interfaces under `include/`.
- **DO** create new test files ONLY in `test/test_unit/test_<moduleName>/`.
- You are authorized to make **minimal** `NATIVE_BUILD` guard additions to production headers if the header cannot compile under the native environment otherwise.

---

## Pre-Requisite: Fix `otaUpdateManager.hpp` for Native Build

The `otaUpdateManager.hpp` header currently has:
```cpp
#include <Arduino.h>
#include <Stream.h>
```
These are NOT guarded by `NATIVE_BUILD`. You MUST add guards so the header compiles under native:

```cpp
#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <Stream.h>
#else
#include <cstddef>  // for size_t
#include <stdint.h>
// Minimal Stream stub for NATIVE_BUILD
class Stream {};
#endif
```

This is the ONLY production code change permitted for this WO.

---

## Reference Material

Before writing tests, read the following headers to understand the public API:
- `modules/wifiManager/wifiManager.hpp` — WiFi state machine, `WiFiState` enum, credential management
- `modules/wifiManager/wifiManager.cpp` — Full implementation with `NATIVE_BUILD` stubs
- `modules/timeManager/timeManager.hpp` — NTP sync, `TimeSyncStatus` enum
- `modules/timeManager/timeManager.cpp` — Has full `NATIVE_BUILD` mock implementation
- `modules/otaUpdateManager/otaUpdateManager.hpp` — OTA lifecycle methods
- `modules/otaUpdateManager/otaUpdateManager.cpp` — `NATIVE_BUILD` guards return defaults

Also read an existing test for the pattern:
- `test/test_unit/test_eventbus/test_main.cpp` — Reference for test fixture, callback patterns

---

## Line-Item Instructions

### Task 1: `test_wifiManager.cpp` (15+ tests)

Create `test/test_unit/test_wifiManager/test_wifiManager.cpp`.

Test the following behaviors (the native build stubs many hardware calls, so focus on the testable logic):

- [ ] **Construction defaults** — After construction, `getSSID()` is empty, `hasPassword()` is false, `getAPMode()` is false, `isReady()` is false.
- [ ] **begin() sets hostname** — After `begin("test-host")`, state is WIFI_INIT.
- [ ] **updateWiFi() stores SSID** — After `updateWiFi("MySSID", "MyPass")`, `getSSID()` returns "MySSID", `hasPassword()` is true.
- [ ] **updateWiFi() with empty pass** — Only updates SSID, preserves existing password.
- [ ] **updateWiFi() with nullptr pass** — Handles gracefully without crash.
- [ ] **resetConfig()** — After `updateWiFi(...)` + `resetConfig()`, SSID is empty, password is cleared.
- [ ] **testConnection()** — In native build, returns true and sets ipOut to "192.168.1.100".
- [ ] **getMyUrl() default** — Returns "http://0.0.0.0" before any IP update.
- [ ] **isWifiPersistentError()** — In native build, always returns false.
- [ ] **notifyApiActivity()** — Call without crash (no-op in native).
- [ ] **Multiple begin() calls** — Re-calling begin() with different hostname doesn't crash.
- [ ] **SSID boundary** — 32-char max SSID stored correctly.
- [ ] **Password boundary** — 64-char max password stored correctly.
- [ ] **SSID overflow** — 33+ char SSID truncated to 32 chars (buffer is 33 including null).
- [ ] **getTabName()** — Returns "WiFi".

### Task 2: `test_timeManager.cpp` (10+ tests)

Create `test/test_unit/test_timeManager/test_timeManager.cpp`.

The native build mock provides a working implementation. Test:

- [ ] **Initial state** — `isTimeSet()` returns false before `begin()`.
- [ ] **begin() sets synced** — After `begin("GMT0")`, `isTimeSet()` returns true (mock sets SYNCED immediately).
- [ ] **getLocalTime() before begin** — Returns false, does not crash.
- [ ] **getLocalTime() after begin** — Returns true and populates `struct tm`.
- [ ] **getLocalTime() with nullptr** — Returns false, no crash.
- [ ] **setTimezone()** — Call `setTimezone("EST5EDT")` without crash (mock is no-op).
- [ ] **tick()** — Calling `tick()` after `begin()` is a no-op, no crash.
- [ ] **tick() before begin** — Calling `tick()` before `begin()` is safe.
- [ ] **Multiple begin() calls** — Re-calling begin with different timezone doesn't crash.
- [ ] **begin() with custom NTP servers** — `begin("GMT0", "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org")` succeeds.

### Task 3: `test_otaUpdateManager.cpp` (15+ tests)

Create `test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp`.

In native build, all OTA operations return defaults (false/0). Test the stub behavior and the non-hardware logic:

- [ ] **Construction** — `otaUpdateManager` can be constructed without crash.
- [ ] **beginUpdate() returns false** — In native build, `beginUpdate(1024, "abc123")` returns false.
- [ ] **beginUpdate() with nullptr MD5** — `beginUpdate(1024, nullptr)` returns false, no crash.
- [ ] **beginUpdate() with zero size** — `beginUpdate(0, "abc")` returns false.
- [ ] **endUpdate() returns false** — In native build, `endUpdate()` returns false.
- [ ] **markAppValid()** — Call without crash (no-op in native).
- [ ] **rollbackFirmware()** — Call without crash (no-op in native).
- [ ] **getTabName()** — Returns "OTA".
- [ ] **Global instance** — `otaManager` global instance is accessible and valid.
- [ ] **Lifecycle sequence** — `beginUpdate()` → `endUpdate()` doesn't crash.
- [ ] **markAppValid() multiple calls** — Multiple calls don't crash.
- [ ] **rollbackFirmware() multiple calls** — Multiple calls don't crash.
- [ ] **beginUpdate() after endUpdate()** — Can call beginUpdate again (reset lifecycle).
- [ ] **writeStream() returns 0** — In native build, returns 0 bytes written.
- [ ] **registerRoutes()** — Would need mock AsyncWebServer; verify it doesn't crash on native (guarded).

---

## Verification

1. All three test files compile under `pio test -e native`.
2. All tests pass (green).
3. No existing tests are broken.
4. The only production code change is the `NATIVE_BUILD` guard fix in `otaUpdateManager.hpp`.

---

## Closure

Once verified, **STOP and ASK the user** if the implementation is validated and if any iteration is needed. Do not wrap up or distill until explicitly approved. Once approved, invoke the `@distillery` skill to package your output, passing `.agents/plugins/principal-architect-workspace/pr-05c-test-hardening/wo1-kernel-tests/` as the target directory. Finally, append a status update to this `WORKORDER.md` file and drop control back.

### Status
COMPLETED: Tests implemented and verified. Distillation complete.
