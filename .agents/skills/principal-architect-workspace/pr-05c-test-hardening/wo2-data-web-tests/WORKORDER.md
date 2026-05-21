---
name: pr-05c-wo2
description: Handoff instructions for Work Order 2 — Data + web test expansion (configManager, dataManager, eventBus, webServer)
---

# PR-05c / WO2 — Data + Web Test Expansion

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

Expand existing test suites for four modules that already have some coverage but fall short of targets:

| Module | Current | Target | Gap | File |
|---|---|---|---|---|
| `configManager` | 7 | 15+ | +8 | `test/test_unit/test_configManager/test_configManager.cpp` |
| `dataManager` | 6 | 12+ | +6 | `test/test_unit/test_dataManager/test_main.cpp` |
| `eventBus` | 8 | 15+ | +7 | `test/test_unit/test_eventbus/test_main.cpp` |
| `webServer` | 2 | 15+ | +13 | `test/test_unit/test_webServer/test_webServer.cpp` |

---

## Strict Boundary Box

- **DO NOT** create new test directories — all four directories already exist.
- **DO NOT** modify any file under `modules/` or `lib/`.
- **DO NOT** modify `platformio.ini`.
- **DO NOT** delete or alter ANY existing test cases — ONLY append new ones.
- **DO NOT** modify any existing interfaces under `include/`.
- You are ONLY adding new `TEST_F()` or `TEST()` macros to the existing test files.

---

## Reference Material

Before writing tests, read each module's header AND the existing test file to understand:
1. What is already tested (avoid duplication).
2. The public API surface.
3. The test fixture pattern used.

### Required reads:
- `modules/configManager/configManager.hpp` + `modules/configManager/configManager.cpp`
- `test/test_unit/test_configManager/test_configManager.cpp` (existing: 7 tests)
- `modules/dataManager/dataManager.hpp` + `modules/dataManager/dataManager.cpp`
- `test/test_unit/test_dataManager/test_main.cpp` (existing: 6 tests)
- `modules/eventBus/EventBus.hpp` + `modules/eventBus/EventBus.cpp`
- `test/test_unit/test_eventbus/test_main.cpp` (existing: 8 tests, note the `test_eventbus` dir name)
- `modules/webServer/webServer.hpp` + `modules/webServer/webServer.cpp`
- `test/test_unit/test_webServer/test_webServer.cpp` (existing: 2 tests)
- `include/interfaces/IWebContributor.hpp`
- `include/interfaces/iDataSource.hpp`

---

## Line-Item Instructions

### Task 1: Expand `test_configManager.cpp` (+8 tests)

Append to `test/test_unit/test_configManager/test_configManager.cpp`. Existing fixture is `ConfigManagerTest`.

- [ ] **SaveConfig without prior load** — `saveConfig()` succeeds even with defaults.
- [ ] **Load corrupted JSON** — Write invalid JSON to `SYSTEM_CONFIG_PATH`, `loadConfig()` returns false, applies defaults.
- [ ] **Load empty file** — Write empty file, `loadConfig()` returns false, applies defaults.
- [ ] **Hostname boundary** — Set 32-char hostname, save/load round-trip preserves it exactly.
- [ ] **Hostname overflow** — Set 33+ char hostname, truncated to buffer size.
- [ ] **Timezone boundary** — Set 63-char timezone, save/load round-trip preserves it exactly.
- [ ] **Multiple save/load cycles** — Save, load, modify, save, load — final state correct.
- [ ] **getMutableConfig() modification** — Modify via mutable ref, verify via const ref.

### Task 2: Expand `test_dataManager/test_main.cpp` (+6 tests)

Append to `test/test_unit/test_dataManager/test_main.cpp`. Read existing test to understand the `MockDataSource` and fixture pattern.

- [ ] **Register nullptr** — `registerSource(nullptr)` doesn't crash.
- [ ] **Unregister non-existent source** — `unregisterSource()` on unregistered source is safe.
- [ ] **Unregister nullptr** — `unregisterSource(nullptr)` doesn't crash.
- [ ] **Request priority on unregistered source** — `requestPriorityFetch()` on unknown source is safe.
- [ ] **Double registration** — Registering the same source twice doesn't duplicate it or crash.
- [ ] **Register after unregister** — Unregister then re-register a source works correctly.

### Task 3: Expand `test_eventbus/test_main.cpp` (+7 tests)

Append to `test/test_unit/test_eventbus/test_main.cpp`. Existing fixture is `EventBusTest` with `testCallback`.

- [ ] **Unsubscribe non-existent** — `unsubscribe("no/such", testCallback)` returns false.
- [ ] **Publish with nullptr topic** — `publish(nullptr)` returns 0, no crash.
- [ ] **Publish with empty topic** — `publish("")` returns 0.
- [ ] **Null callback subscription** — `subscribe("sys/test", nullptr)` returns false.
- [ ] **Multiple topics same callback** — Subscribe to two different topics with same callback, publish each independently, verify correct dispatch.
- [ ] **Unsubscribe then resubscribe** — After unsubscribe, resubscribing to same topic works.
- [ ] **Publish with nullptr payload** — Subscriber receives nullptr payload and handles it.

### Task 4: Expand `test_webServer.cpp` (+13 tests)

Append to `test/test_unit/test_webServer/test_webServer.cpp`. Use the existing mock `AsyncWebServer` class from `webServer.hpp` (native build block).

- [ ] **Construction** — `WebServerManager` constructs without crash.
- [ ] **begin() with empty contributors** — `begin({})` succeeds, server is initialized.
- [ ] **begin() with nullptr in list** — Handle gracefully if a nullptr contributor is in the vector.
- [ ] **Single contributor registration** — One contributor, verify `registered == true`.
- [ ] **Three contributors** — Three contributors all get `registered == true`.
- [ ] **getServer() returns valid ref** — Call `getServer()` after `begin()`, verify usable.
- [ ] **Routes count after begin** — After `begin({})`, verify `/api/tabs` and `/api/system` routes exist.
- [ ] **Route method verification** — `/api/tabs` is registered as HTTP_GET.
- [ ] **Contributor with tab name** — MockContributor with tab name returns correct value.
- [ ] **Contributor without tab name** — MockContributor returning nullptr for `getTabName()`.
- [ ] **POST body size cap** — Verify `AsyncWebServer::lastMaxBodySize == 4096`.
- [ ] **Multiple begin() calls** — Re-calling `begin()` doesn't crash (idempotent).
- [ ] **Route handler invocation** — After `begin()`, find a route and invoke its handler with nullptr request to verify it's callable (or just verify it was registered).

---

## Verification

1. `pio test -e native` passes all tests including new ones.
2. All existing tests remain green (no regressions).
3. No production code modified.
4. Test counts: configManager ≥ 15, dataManager ≥ 12, eventBus ≥ 15, webServer ≥ 15.

---

## Closure

Once verified, **STOP and ASK the user** if the implementation is validated and if any iteration is needed. Do not wrap up or distill until explicitly approved. Once approved, invoke the `@distillery` skill to package your output, passing `.agents/skills/principal-architect-workspace/pr-05c-test-hardening/wo2-data-web-tests/` as the target directory. Finally, append a status update to this `WORKORDER.md` file and drop control back.

## Execution Status Update
- **Branch Checked Out**: `pr-05c-wo2-data-web-tests`
- **Modules Expanded**:
  - `configManager`: Added 9 tests (Total: 15)
  - `dataManager`: Added 7 tests (Total: 12)
  - `eventBus`: Added 8 tests (Total: 15)
  - `webServer`: Added 14 tests (Total: 15)
- **Status**: All targeted modules pass `pio test -e native`. 
Note: Pre-existing `wifiManager` and `otaUpdateManager` suites fail to compile in native mode due to missing mock headers (e.g., Arduino.h). As per the boundary box rules restricting modifications outside the specific targets and prohibiting changes to `modules/`, these were left unmodified.
