---
schema_version: 1
wo_id: WO-320.1
pr_id: PR-320
title: Configure LwIP DHCP Option 42/100/101 hooks, sdkconfigs, and repo hygiene
complexity: standard
status: PENDING
allowed_files:
- .memory/.gitignore
- CMakeLists.txt
- sdkconfig.defaults
- include/lwip_dhcp_tz_hooks.h
- sample-app/CMakeLists.txt
- sample-app/sdkconfig.defaults
- template-app/CMakeLists.txt
- template-app/sdkconfig.defaults
forbidden_files:
- docs/design_specification.md
required_skills:
- embedded-systems
- framework-expert
- pio-manager
verification_command: python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 87eaa5fa6e6b446631a3bf9180b4591e6bf02fdd80d49c233f1407ceadc3d140
completion: null
---

# WO-320.1 — Configure LwIP DHCP Option 42/100/101 hooks, sdkconfigs, and repo hygiene

## 1. Objective & Scope

Prepare repository hygiene, ESP-IDF LwIP Kconfig defaults, CMake LwIP hook injection, and the C-compatible DHCP extra-option hook header across all three targets (`esp32-app-foundation` root, `sample-app/`, and `template-app/`) so subsequent `TimeManager` work can receive DHCP Option 42 (NTP servers) and RFC 4833 Options 100 (`PCode`) and 101 (`TCode`).

## 2. Detailed Specification

### 2.1 Repo Hygiene (`.memory/.gitignore`)
- Create `.memory/.gitignore` ignoring volatile runtime state produced by PA / repo-memory:
  ```gitignore
  .cache/
  .enrolled
  local/
  ```
- Confirm `.memory/.enrolled` no longer appears in `git status --porcelain`.

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
- Root `CMakeLists.txt`:
  ```cmake
  idf_component_get_property(lwip lwip COMPONENT_LIB)
  if(lwip)
      target_compile_options(${lwip} PRIVATE "-I${PROJECT_DIR}/include")
      target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
  endif()
  ```
- `sample-app/CMakeLists.txt` and `template-app/CMakeLists.txt`:
  Note that `include/lwip_dhcp_tz_hooks.h` lives in the parent framework repo (`../include` relative to `sample-app/` and `template-app/`). Resolve it reliably via `CMAKE_CURRENT_LIST_DIR`:
  ```cmake
  idf_component_get_property(lwip lwip COMPONENT_LIB)
  if(lwip)
      get_filename_component(FRAMEWORK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../include" ABSOLUTE)
      target_compile_options(${lwip} PRIVATE "-I${FRAMEWORK_INCLUDE_DIR}")
      target_compile_definitions(${lwip} PRIVATE "-DESP_IDF_LWIP_HOOK_FILENAME=\"lwip_dhcp_tz_hooks.h\"")
  endif()
  ```

## 3. Verification Requirements

1. Inspect `scripts/sync_sdkconfig.py` and run PlatformIO MCP `build_project` (ALWAYS `acquire_lock` -> `build_project` -> `release_lock`, never shell `pio`!) on:
   - Root project `esp32dev` (verifies `lwip` compiles `dhcp.c` and `lwip_default_hooks.c` cleanly with `include/lwip_dhcp_tz_hooks.h` and new Kconfig flags; also check generated `sdkconfig` / `sdkconfig.esp32dev` includes `CONFIG_LWIP_DHCP_GET_NTP_SRV=y` and `CONFIG_LWIP_HOOK_DHCP_EXTRA_OPTION_DEFAULT=y`, and check if cached `sdkconfig.*` files are untracked/gitignored or need attention).
   - Root project `native` `run_tests` (ensure existing unit tests still pass).
2. Run `python3 scripts/doc_lint.py` and confirm exit `0`.
3. Verify `git status --porcelain` shows only your `allowed_files` modified/added (and `.memory/.enrolled` is ignored).
4. Record your work summary in Part II (`completion.verification_summary`, `completion.decisions_made`, `## Completion Report`) BEFORE running `pa wo verify WO-320.1` as your final mutating action.

## Completion Report
