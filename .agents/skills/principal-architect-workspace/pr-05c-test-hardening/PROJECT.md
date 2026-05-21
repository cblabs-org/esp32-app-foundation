# PR-05c: Test Hardening — PROJECT.md

**Branch:** `pr-05c-test-hardening`  
**PA Session:** (current)  
**Depends on:** PR-01 through PR-05b (all merged)  
**Blocks:** PR-07 (template must demonstrate a well-tested framework)  
**Status:** IN PROGRESS

---

## Objective

Fill critical test coverage gaps across kernel services, data infrastructure,
web portal, and low-level libraries. All modules that interact with ESP-IDF
hardware APIs must be tested via mocked dependencies under the `NATIVE_BUILD`
guard pattern established in PR-05/PR-05a.

**CRITICAL CONSTRAINT:** This PR writes ONLY tests (in `test/test_unit/`).
No production source modifications unless absolutely necessary for testability
(e.g., adding `NATIVE_BUILD` guards to headers that lack them).

---

## Coverage Audit

| Module | Location | Current Tests | Target | WO |
|---|---|---|---|---|
| `wifiManager` | `modules/wifiManager/` | 0 | 15+ | WO1 |
| `timeManager` | `modules/timeManager/` | 0 | 10+ | WO1 |
| `otaUpdateManager` | `modules/otaUpdateManager/` | 0 | 15+ | WO1 |
| `configManager` | `modules/configManager/` | 7 | 15+ | WO2 |
| `dataManager` | `modules/dataManager/` | 6 | 12+ | WO2 |
| `eventBus` | `modules/eventBus/` | 8 | 15+ | WO2 |
| `webServer` | `modules/webServer/` | 2 | 15+ | WO2 |
| `keyManager` | `modules/keyManager/` | 5 | 12+ | WO3 |
| `deviceCrypto` | `lib/deviceCrypto/` | 0 | 8+ | WO3 |
| `logger` | `lib/logger/` | 0 | 8+ | WO3 |
| `md5Utils` | `lib/md5Utils/` | 0 | 6+ | WO3 |
| `githubClient` | `lib/githubClient/` | 0 | 8+ | WO3 |

---

## Work Order Ledger

- **[ ] [wo1-kernel-tests](wo1-kernel-tests/WORKORDER.md):** Kernel service tests — `wifiManager`, `timeManager`, `otaUpdateManager`. Mock ESP-IDF APIs, test state machines, credential management, OTA lifecycle.
- **[ ] [wo2-data-web-tests](wo2-data-web-tests/WORKORDER.md):** Data + web expansion — `configManager` (+8), `dataManager` (+6), `eventBus` (+7), `webServer` (+13). Edge cases, error paths, dispatch mechanics.
- **[ ] [wo3-lib-tests](wo3-lib-tests/WORKORDER.md):** Low-level lib tests — `deviceCrypto`, `logger`, `md5Utils`, `githubClient` (all new) + `keyManager` expansion (+7). Mock hardware crypto, test redaction, streaming hash, URL construction.

---

## Discovered Dependencies for Native Testing

| Module | `NATIVE_BUILD` Guards | Notes |
|---|---|---|
| `wifiManager.hpp` | ✅ Has guards | `Arduino.h` guarded, stub `String` class provided |
| `wifiManager.cpp` | ✅ Has guards | `millis()` and `delay()` stubbed, all HW calls guarded |
| `timeManager.hpp` | ✅ Has guards | `esp_sntp.h` guarded |
| `timeManager.cpp` | ✅ Full mock impl | Separate `NATIVE_BUILD` implementation block |
| `otaUpdateManager.hpp` | ❌ **Missing** | `#include <Arduino.h>` and `#include <Stream.h>` unguarded — WO1 must fix |
| `otaUpdateManager.cpp` | ✅ Has guards | All `Update.*` and `esp_ota_*` calls guarded |
| `configManager` | ✅ Has guards | Uses local file path for native testing |
| `dataManager` | ✅ Has guards | FreeRTOS types stubbed, `simulatedTimeMs` helper |
| `eventBus` | ✅ No ESP deps | Pure C++, no guards needed |
| `webServer.hpp` | ✅ Has guards | Full mock `AsyncWebServer` class in header |
| `keyManager` | ✅ Has guards | Uses local file path for native testing |
| `deviceCrypto.hpp` | ✅ Has guards | `Arduino.h` guarded |
| `deviceCrypto.cpp` | ⚠️ Needs review | May need full mock encrypt/decrypt for native |
| `logger.hpp` | ✅ Has guards | ESP log macros stubbed via `PIO_UNIT_TESTING` |
| `md5Utils.hpp` | ❌ **Missing** | `#include <Arduino.h>` unguarded — WO3 must fix |
| `githubClient.hpp` | ❌ **Missing** | `#include <Arduino.h>` unguarded — WO3 must fix |

---

## Test Pattern Reference

All existing tests follow this pattern:
- GoogleTest (`#include <gtest/gtest.h>`)
- Test fixtures using `::testing::Test` with `SetUp()`/`TearDown()`
- Files named `test_main.cpp` or `test_<module>.cpp`
- `main()` at bottom: `::testing::InitGoogleTest(&argc, argv); return RUN_ALL_TESTS();`
- `#if defined(NATIVE_BUILD)` guard around `main()` in some files

---

## Architectural Notes

1. **No std::function** — EventBus tests use static fn-ptr callbacks, never lambdas stored in `std::function`.
2. **ConfigManager** uses file I/O on native (regular `fstream`) — test cleanup with `remove()`.
3. **KeyManager** depends on `DeviceCrypto*` injection — mock crypto needed for native.
4. **DataManager** exposes `processNext()` and `setSimulatedTimeMs()` for native testing.
5. **otaUpdateManager** exposes a global instance `otaManager` — tests can use it directly.
6. **Logger** uses `PIO_UNIT_TESTING` flag (defined by PlatformIO test runner) to stub ESP log macros.

---

*Created by Principal Architect — PR-05c Test Hardening — 2026-05-21*
