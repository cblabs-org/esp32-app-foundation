---
schema_version: 1
wo_id: WO-144.4
pr_id: PR-144
title: Fix C++17 ODR multiple definition of mock_mqtt_publish_return_value in native_ha
complexity: standard
status: COMPLETED
allowed_files:
- test/stubs/platform/mqtt_client.h
- test/test_native/test_homeassistant/test_main.cpp
- test/test_unit/test_homeassistant/test_homeassistant.cpp
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills: []
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 996d7b06da202b4aa303bf9ce16f69587100e8815cb991c9ac88027a061c8197
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_d884534e9ccf
  commit_sha: 2bc1cf510284c9e905b020ec2da07ea4b5e5fe7f
  files_modified:
  - test/stubs/platform/mqtt_client.h
  - test/test_native/test_homeassistant/test_main.cpp
  - test/test_unit/test_homeassistant/test_homeassistant.cpp
  verification_summary: pytest scripts/tests/ (93 passed) & doc_lint.py (exit 0);
    left unstaged for Coordinator PlatformIO MCP native_ha verification.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Unified mock_mqtt_publish_return_value on the single C++17 inline variable in
    test/stubs/platform/mqtt_client.h and removed MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED
    plus strong non-inline definitions from test/test_native/test_homeassistant/test_main.cpp
    and test/test_unit/test_homeassistant/test_homeassistant.cpp.
  dead_ends: []
---

# WO-144.4 — Fix C++17 ODR multiple definition of mock_mqtt_publish_return_value in native_ha

## 1. Objective & Scope

Resolve the Linux `GNU ld` C++17 One Definition Rule (ODR) link failure in `native_ha` (`test_native/test_homeassistant` and `test_unit/test_homeassistant`) on Ubuntu 24.04:
- `test/stubs/platform/mqtt_client.h` (included by `modules/HomeAssistant/HomeAssistant.hpp` and compiled into `libHomeAssistant.a(HomeAssistant.o)`) defines `inline int mock_mqtt_publish_return_value = 0;` inside `#ifndef MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED`.
- `test/test_native/test_homeassistant/test_main.cpp:8-9` and `test/test_unit/test_homeassistant/test_homeassistant.cpp:8-9` `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` and emit a non-`inline` strong `.bss` definition `int mock_mqtt_publish_return_value = 0;`.
- While macOS `ld64` coalesces them, Linux `GNU ld` rejects mixing an ELF COMDAT `inline` definition from `libHomeAssistant.a(HomeAssistant.o)` with the strong `.bss` symbol from the test object file (`multiple definition of 'mock_mqtt_publish_return_value'`).
- Both test files include `HomeAssistant.hpp` (which includes `<mqtt_client.h>`) inside `#ifdef WITH_HOMEASSISTANT` and never reference `mock_mqtt_publish_return_value` outside `#ifdef WITH_HOMEASSISTANT`, so unifying on the single C++17 `inline` variable definition in `test/stubs/platform/mqtt_client.h` satisfies `-std=c++17` on both Linux `GNU ld` and macOS `ld64`.

Scope is strictly bounded to `test/stubs/platform/mqtt_client.h`, `test/test_native/test_homeassistant/test_main.cpp`, and `test/test_unit/test_homeassistant/test_homeassistant.cpp`.

## 2. Detailed Specification

### 2.1 `test/stubs/platform/mqtt_client.h` (lines 83–85)

- Remove `#ifndef MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` (line 83) and `#endif` (line 85), leaving the unconditional C++17 `inline` variable definition directly before `esp_mqtt_client_publish(...)`:
  ```cpp
  inline int mock_mqtt_publish_return_value = 0;
  ```

### 2.2 `test/test_native/test_homeassistant/test_main.cpp` (lines 8–9)

- Delete `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` and `int mock_mqtt_publish_return_value = 0;` (lines 8–9), keeping clean whitespace between the file header comment and `#include <gtest/gtest.h>`.

### 2.3 `test/test_unit/test_homeassistant/test_homeassistant.cpp` (lines 8–9)

- Delete `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` and `int mock_mqtt_publish_return_value = 0;` (lines 8–9), keeping clean whitespace between the file header comment and `#include <gtest/gtest.h>`.
- Preserve all `mock_mqtt_publish_return_value` references inside `HomeAssistantBackoffTest` (which resolve directly to the C++17 `inline` variable defined in `test/stubs/platform/mqtt_client.h` via `modules/HomeAssistant/HomeAssistant.hpp`).

## 3. Verification Requirements

1. **Mandatory Gate Command:**
   - `pytest scripts/tests/ && python3 scripts/doc_lint.py` (must exit `0` with zero failures).
2. **Boundary & Completion Hygiene:**
   - Keep all edits strictly inside `allowed_files` (plus Part II of this `WORKORDER.md`).
   - Keep `completion.verification_summary` strictly `<= 280` characters.
3. **PlatformIO `native_ha` Verification & Receipt Handoff:**
   - Coordinator will verify `native_ha` (`1044/1044` across `37/37` suites, including `test_native/test_homeassistant` and `test_unit/test_homeassistant`) via PlatformIO MCP and stamp centrally if skipped by Tier-3 (`ESCALATED_TO_PA`).

## Completion Report

Implemented all `WO-144.4` changes strictly within `allowed_files` and left all edits unstaged for Coordinator PlatformIO MCP `native_ha` verification and central receipt stamping (`pa wo verify` / `git commit` not run by worker per dispatch instructions):

- **`test/stubs/platform/mqtt_client.h` (`L83-L84`):** Removed `#ifndef MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` and `#endif`, leaving unconditional `inline int mock_mqtt_publish_return_value = 0;`.
- **`test/test_native/test_homeassistant/test_main.cpp` (`L1-L10`):** Deleted `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` and `int mock_mqtt_publish_return_value = 0;`, keeping clean whitespace before `#include <gtest/gtest.h>`.
- **`test/test_unit/test_homeassistant/test_homeassistant.cpp` (`L1-L10`):** Deleted `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` and `int mock_mqtt_publish_return_value = 0;`, keeping clean whitespace before `#include <gtest/gtest.h>` while preserving all `mock_mqtt_publish_return_value` references in `HomeAssistantBackoffTest`.

### Verification Results
- `pytest scripts/tests/`: `93 passed` (exit `0`).
- `python3 scripts/doc_lint.py`: exit `0`.
- `git diff --cached --name-only`: empty (all changes unstaged).
- PlatformIO MCP `native_ha` test suite & `pa wo verify`: deferred to Coordinator per handoff (`ESCALATED_TO_PA`).
