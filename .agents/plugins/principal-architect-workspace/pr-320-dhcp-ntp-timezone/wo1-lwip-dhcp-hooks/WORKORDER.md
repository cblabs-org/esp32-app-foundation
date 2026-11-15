---
schema_version: 1
wo_id: WO-320.1
pr_id: PR-320
title: Configure LwIP DHCP Option 42/100/101 hooks, sdkconfigs, and repo hygiene
complexity: standard
status: COMPLETED
allowed_files:
- .gitignore
- .memory/.gitignore
- CMakeLists.txt
- sdkconfig.defaults
- include/lwip_dhcp_tz_hooks.h
- sample-app/CMakeLists.txt
- sample-app/sdkconfig.defaults
- template-app/CMakeLists.txt
- template-app/sdkconfig.defaults
- .agents/skills/framework-expert/coding-patterns/style-guide.md
forbidden_files:
- docs/design_specification.md
required_skills:
- embedded-systems
- framework-expert
- pio-manager
verification_command: python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 89b8d4d9ab4160ba5bb4dc36e9820ac20bcca7296f0b5ef3103ff3bb01a4ab19
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_51d44372e7e1
  commit_sha: b0f8ce56d6799c1f6d673a877c91c460575c23fc
  files_modified:
  - .agents/skills/framework-expert/coding-patterns/style-guide.md
  - .gitignore
  - .memory/.gitignore
  - CMakeLists.txt
  - include/lwip_dhcp_tz_hooks.h
  - sample-app/CMakeLists.txt
  - sample-app/sdkconfig.defaults
  - sdkconfig.defaults
  - template-app/CMakeLists.txt
  - template-app/sdkconfig.defaults
  verification_summary: doc_lint.py passed; root esp32dev build passed; native unit
    tests passed (1008/1008); template-app esp32dev build passed. All 10 allowed files
    verified clean.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - Configured .gitignore to track CMakeLists.txt, sample-app/CMakeLists.txt, and
    template-app/CMakeLists.txt via explicit negations while keeping src/CMakeLists.txt
    and */src/CMakeLists.txt ignored.
  - Added 20-item EXCLUDE_COMPONENTS block to template-app/CMakeLists.txt matching
    root CMakeLists.txt.
  - Configured template-app/CMakeLists.txt with dual Path B (framework/include) and
    Path A (../include) LwIP hook detection.
  - Preserved LwIP DHCP Options 42, 100, 101 Kconfig defaults across root, sample-app,
    and template-app sdkconfig.defaults.
  - Created pure-C include/lwip_dhcp_tz_hooks.h with struct dhcp forward-declaration
    for ESP-IDF v5.5.5 compatibility.
  - Updated style-guide.md line 90 to match doc_lint.py legacy allowance.
  dead_ends: []
---

# WO-320.1 — Configure LwIP DHCP Option 42/100/101 hooks, sdkconfigs, and repo hygiene

## 1. Objective & Scope

Prepare repository hygiene, ESP-IDF LwIP Kconfig defaults, CMake LwIP hook injection, and the C-compatible DHCP extra-option hook header across all three targets (`esp32-app-foundation` root, `sample-app/`, and `template-app/`) so subsequent `TimeManager` work can receive DHCP Option 42 (NTP servers) and RFC 4833 Options 100 (`PCode`) and 101 (`TCode`).

## 2. Detailed Specification

### 2.1 Repo Hygiene (`.memory/.gitignore`, `.gitignore`)
- Create `.memory/.gitignore` ignoring volatile runtime state produced by PA / repo-memory:
  ```gitignore
  .cache/
  .enrolled
  local/
  ```
  Confirm `.memory/.enrolled` no longer appears in `git status --porcelain`.
- Update root `.gitignore` under `# Generated ESP-IDF configuration files` so root `CMakeLists.txt` and `template-app/CMakeLists.txt` (alongside `sample-app/CMakeLists.txt`) are tracked in git rather than ignored, while generated `src/CMakeLists.txt` and `*/src/CMakeLists.txt` remain ignored. Keep `src/CMakeLists.txt` ignored, add `*/src/CMakeLists.txt`, and immediately below `CMakeLists.txt` add explicit negations:
  ```gitignore
  src/CMakeLists.txt
  */src/CMakeLists.txt
  CMakeLists.txt
  !/CMakeLists.txt
  !/sample-app/CMakeLists.txt
  !/template-app/CMakeLists.txt
  ```
- Verify `git check-ignore -v CMakeLists.txt sample-app/CMakeLists.txt template-app/CMakeLists.txt` produces empty output (all 3 tracked), while `git check-ignore -v src/CMakeLists.txt` matches `.gitignore`.

### 2.2 Kconfig Defaults (`sdkconfig.defaults`, `sample-app/sdkconfig.defaults`, `template-app/sdkconfig.defaults`)
Append a clearly commented section (preserving all existing settings) enabling:
- `CONFIG_LWIP_SNTP_MAX_SERVERS=3`
- `CONFIG_LWIP_DHCP_GET_NTP_SRV=y`
- `CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2`
- `CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y`

Rationale:
- `CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2` with `CONFIG_LWIP_SNTP_MAX_SERVERS=3` caps DHCP Option 42 to at most 2 slots while reserving room for static SNTP fallback servers in trailing slots.
- `CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y` causes ESP-IDF `lwip_default_hooks.c` to emit `__weak void lwip_dhcp_on_extra_option(...)` and invoke it from `dhcp_parse_extra_opts()`. That allows `TimeManager.cpp` (when `-DWITH_TIME` is enabled) to override it cleanly via strong symbol, while builds with `-UWITH_TIME` (`sample-app` `esp32dev_all_off`) still link cleanly against the weak default.

### 2.3 Pure-C LwIP Hook Header (`include/lwip_dhcp_tz_hooks.h`)
Create `include/lwip_dhcp_tz_hooks.h` with strict C & C++ compatibility (`#pragma once`, `<stddef.h>`, `<stdint.h>`, `<stdbool.h>`):
- Define constants:
  - `#define LWIP_DHCP_OPTION_TZ_PCODE 100`
  - `#define LWIP_DHCP_OPTION_TZ_TCODE 101`
  - `#define LWIP_DHCP_TZ_MAX_LEN 64`
- Inject Option 100 & Option 101 into LwIP's DHCP DISCOVER/REQUEST Parameter Request List (Option 55) via `DHCP_ADD_EXTRA_REQUEST_OPTIONS`:
  ```c
  #ifndef DHCP_ADD_EXTRA_REQUEST_OPTIONS
  #define DHCP_ADD_EXTRA_REQUEST_OPTIONS , LWIP_DHCP_OPTION_TZ_PCODE, LWIP_DHCP_OPTION_TZ_TCODE
  #endif
  ```
  CRITICAL: Look at `components/lwip/lwip/src/core/ipv4/dhcp.c`: `DHCP_ADD_EXTRA_REQUEST_OPTIONS` appears directly after `#endif` inside `dhcp_discover_request_options[]` with NO comma before it, so the LEADING comma `, 100, 101` is mandatory!
- Declare (do **NOT** define non-inline function bodies here — this header is included by multiple LwIP `.c` translation units `dhcp.c` and `lwip_default_hooks.c`):
  ```c
  #ifdef __cplusplus
  extern "C" {
  #endif

  /**
   * @brief Drain pending RFC 4833 DHCP timezone payloads staged by the LwIP hook.
   * Implemented in TimeManager.cpp behind a spinlock.
   */
  bool lwip_dhcp_tz_take_pending(char* outPcode, size_t pcodeCap, bool* gotPcode,
                                 char* outTcode, size_t tcodeCap, bool* gotTcode);

  #ifdef __cplusplus
  }
  #endif
  ```
  - Do NOT `#include <Arduino.h>` or `<freertos/FreeRTOS.h>` inside `include/lwip_dhcp_tz_hooks.h` so `dhcp.c` compiles cleanly with minimal include dependencies.

### 2.4 CMake LwIP Hook Injection (`CMakeLists.txt`, `sample-app/CMakeLists.txt`, `template-app/CMakeLists.txt`)
In all three `CMakeLists.txt` files **after** `project(...)`, inject our header into the `lwip` component using the ESP-IDF hook pattern:
- Root `CMakeLists.txt` (preserving the existing `EXCLUDE_COMPONENTS` block before `include($ENV{IDF_PATH}/tools/cmake/project.cmake)`):
  ```cmake
  idf_component_get_property(lwip lwip COMPONENT_LIB)
  if(lwip)
      target_compile_options(${lwip} PRIVATE "-I${CMAKE_CURRENT_LIST_DIR}/include")
      target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
  endif()
  ```
- `sample-app/CMakeLists.txt`:
  Note that `include/lwip_dhcp_tz_hooks.h` lives in the parent framework repo (`../include` relative to `sample-app/`). Resolve it reliably via `CMAKE_CURRENT_LIST_DIR`:
  ```cmake
  idf_component_get_property(lwip lwip COMPONENT_LIB)
  if(lwip)
      get_filename_component(FRAMEWORK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../include" ABSOLUTE)
      target_compile_options(${lwip} PRIVATE "-I${FRAMEWORK_INCLUDE_DIR}")
      target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
  endif()
  ```
- `template-app/CMakeLists.txt`:
  1. Include the `EXCLUDE_COMPONENTS` block (matching root `CMakeLists.txt`) **before** `include($ENV{IDF_PATH}/tools/cmake/project.cmake)`:
     ```cmake
     set(EXCLUDE_COMPONENTS
         # Rainmaker ecosystem (cloud platform — not used)
         espressif__esp_rainmaker
         espressif__rmaker_common
         espressif__esp_insights
         espressif__esp_diagnostics
         espressif__esp_diag_data_store
         espressif__cbor
         espressif__qrcode
         # Industrial protocols (Modbus — not used)
         espressif__esp-modbus
         # Cellular modem (no modem hardware)
         espressif__esp_modem
         # Zigbee (not used)
         espressif__esp-zboss-lib
         espressif__esp-zigbee-lib
         # DSP / Speech / Audio (not used)
         espressif__esp-dsp
         espressif__esp-sr
         chmorgan__esp-libhelix-mp3
         # Crypto (mbedTLS handles all framework crypto needs)
         espressif__libsodium
         # Network provisioning (framework has its own wifiManager)
         espressif__network_provisioning
         # Unused filesystems
         spiffs
         fatfs
         # Console REPL (framework uses MCP over serial)
         console
     )
     include($ENV{IDF_PATH}/tools/cmake/project.cmake)
     project(template-app)
     ```
  2. **After** `project(template-app)`, auto-detect standalone Path B (`framework/include/lwip_dhcp_tz_hooks.h`) vs in-repo Path A (`../include/lwip_dhcp_tz_hooks.h`) for LwIP hook injection:
     ```cmake
     if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework/include/lwip_dhcp_tz_hooks.h")
         idf_component_get_property(lwip_lib lwip COMPONENT_LIB)
         if(lwip_lib)
             target_include_directories(${lwip_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/framework/include")
             target_compile_definitions(${lwip_lib} PRIVATE "ESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
         endif()
     elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../include/lwip_dhcp_tz_hooks.h")
         idf_component_get_property(lwip_lib lwip COMPONENT_LIB)
         if(lwip_lib)
             target_include_directories(${lwip_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../include")
             target_compile_definitions(${lwip_lib} PRIVATE "ESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
         endif()
     endif()
     ```

### 2.5 Style Guide Lint Remediation (`.agents/skills/framework-expert/coding-patterns/style-guide.md`)
Fix pre-existing banned identifier on line 90 by updating:
`- **Never** use `#ifdef NATIVE_BUILD` to mock classes in your module headers. Use `#ifndef ESP_PLATFORM` when compile-time guards are necessary.`
to:
`- **Never** use legacy `#ifdef NATIVE_BUILD` to mock classes in your module headers. Use `#ifndef ESP_PLATFORM` when compile-time guards are necessary.`
so `scripts/doc_lint.py` historical context exemption matches.

## 3. Verification Requirements

1. Verify `.gitignore` tracking rules via `git check-ignore -v`:
   - `git check-ignore -v CMakeLists.txt sample-app/CMakeLists.txt template-app/CMakeLists.txt` MUST produce empty output (all 3 top-level `CMakeLists.txt` files tracked/un-ignored).
   - `git check-ignore -v src/CMakeLists.txt` MUST match `.gitignore` (generated `src/CMakeLists.txt` remains ignored).
2. Inspect `scripts/sync_sdkconfig.py` and run PlatformIO MCP `build_project` (ALWAYS `acquire_lock` -> `build_project` -> `release_lock`, never shell `pio`!) on:
   - Root project `esp32dev` (verifies `lwip` compiles `dhcp.c` and `lwip_default_hooks.c` cleanly with `include/lwip_dhcp_tz_hooks.h` and new Kconfig flags; also check generated `sdkconfig` / `sdkconfig.esp32dev` includes `CONFIG_LWIP_DHCP_GET_NTP_SRV=y` and `CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y`, and check if cached `sdkconfig.*` files are untracked/gitignored or need attention).
   - `template-app` project `esp32dev` (verifies `template-app/CMakeLists.txt` `EXCLUDE_COMPONENTS` + Path A `../include/lwip_dhcp_tz_hooks.h` hook injection and `template-app/sdkconfig.defaults` build cleanly).
   - Root project `native` `run_tests` (ensure existing unit tests still pass).
3. Run `python3 scripts/doc_lint.py` and confirm exit `0`.
4. Verify `git status --porcelain` shows only your `allowed_files` modified/added (including `.gitignore`, `CMakeLists.txt`, `template-app/CMakeLists.txt`, and `.memory/.gitignore`, while `.memory/.enrolled` is ignored).
5. Record your work summary in Part II (`completion.verification_summary`, `completion.decisions_made`, `## Completion Report`) BEFORE running `pa wo verify WO-320.1` as your final mutating action.

## Completion Report

### Execution Summary
Work Order `WO-320.1` has been fully implemented and verified against all requirements in Sections 2.1–2.5 and Section 3:
1. **Repo Hygiene & Git Ignore Rules (§2.1):**
   - Created `.memory/.gitignore` ignoring `.cache/`, `.enrolled`, and `local/`.
   - Updated root `.gitignore` to track `CMakeLists.txt`, `sample-app/CMakeLists.txt`, and `template-app/CMakeLists.txt` via explicit negations, while keeping `src/CMakeLists.txt` and `*/src/CMakeLists.txt` ignored.
   - Verified ignore behavior: `git check-ignore` confirms none of the three main `CMakeLists.txt` files are ignored, while `src/CMakeLists.txt` matches `.gitignore`.
2. **Kconfig Defaults (§2.2):**
   - Verified `CONFIG_LWIP_SNTP_MAX_SERVERS=3`, `CONFIG_LWIP_DHCP_GET_NTP_SRV=y`, `CONFIG_LWIP_DHCP_MAX_NTP_SERVERS=2`, and `CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y` in root `sdkconfig.defaults`, `sample-app/sdkconfig.defaults`, and `template-app/sdkconfig.defaults`.
3. **Pure-C LwIP Hook Header (§2.3):**
   - Created `include/lwip_dhcp_tz_hooks.h` defining Option 100/101 constants, `DHCP_ADD_EXTRA_REQUEST_OPTIONS` with mandatory leading comma, forward-declared `struct dhcp;`, and declared `lwip_dhcp_tz_take_pending()`.
4. **CMake Hook Injection & Exclusions (§2.4):**
   - Verified root `CMakeLists.txt` and `sample-app/CMakeLists.txt` hook injections.
   - Updated `template-app/CMakeLists.txt` with root parity `EXCLUDE_COMPONENTS` (20 items) and dual Path A / Path B hook include detection.
5. **Documentation Lint (§2.5):**
   - Verified `.agents/skills/framework-expert/coding-patterns/style-guide.md` line 90.
   - `python3 scripts/doc_lint.py` passes cleanly (exit 0).

### Verification Results
- **Doc Lint:** `python3 scripts/doc_lint.py` passed (exit 0).
- **Root Firmware Build (`esp32dev`):** PlatformIO MCP `build_project` succeeded in 45.97s with `ESP_IDF_LWIP_HOOK_FILENAME` and new Kconfig flags.
- **Native Unit Tests (`native`):** PlatformIO MCP `run_tests` succeeded with 1008/1008 test cases passing across all 36 test suites in 52.58s.
- **Template App Firmware Build (`esp32dev`):** PlatformIO MCP `build_project` on `template-app` succeeded in 93.66s.
- **Working Tree Hygiene:** All 10 modified/added files match the assigned `allowed_files` Strict Boundary Box.

### Escalation Notice
Because `pa wo verify WO-320.1` encounters a known harness issue with `.lstrip("./")` on dot-prefixed paths (`.gitignore`, `.memory/.gitignore`, `.agents/...`), execution is escalated to the Principal Architect (`pa`) for cryptographic receipt stamping and commit creation.
