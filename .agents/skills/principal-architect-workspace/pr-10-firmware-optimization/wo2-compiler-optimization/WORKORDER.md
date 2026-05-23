---
name: pr-10-wo2
description: Handoff instructions for Work Order 2 — EXCLUDE_COMPONENTS for unused Arduino Core dependencies + Compiler/linker optimization flags.
---

# PR-10 / WO2 — Component Exclusion + Compiler Optimization

## Role Definition

You are an expert senior embedded systems engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Mandatory Skill Boot

Before doing ANY work, you MUST load these skills:
1. `view_file` on `.agents/skills/pio-manager/SKILL.md`
2. `view_file` on `.agents/skills/embedded-systems/SKILL.md`
3. `view_file` on `.agents/skills/house-style-docs/SKILL.md`

## Context

WO1 applied sdkconfig Kconfig changes that reduced the binary from 1,056,085 → 1,044,897 bytes (−11KB). However, the sdkconfig approach is the WRONG layer for disabling entire ESP-IDF components — Kconfig flags control features WITHIN a component, not whether the component is compiled.

The correct mechanism is `EXCLUDE_COMPONENTS` in `CMakeLists.txt`, which tells the ESP-IDF build system to completely skip compilation of named components. The Arduino Core's `idf_component.yml` declares 27 managed component dependencies — most are unused by this framework.

## Objective

1. **Part A:** Add `EXCLUDE_COMPONENTS` to `CMakeLists.txt` to exclude unused Arduino Core managed dependencies (Rainmaker, Zigbee, DSP, speech, modem, libsodium, Modbus, etc.)
2. **Part B:** Apply compiler optimization flags via sdkconfig (`-Os`, newlib nano, compile-time log ceiling, silent assertions)

## Strict Boundary Box

- **DO NOT** modify any C++ source files (`.cpp`, `.hpp`, `.h`)
- **DO NOT** undo any sdkconfig changes from WO1
- **DO NOT** remove any `lib_deps` entries from `platformio.ini`
- **DO NOT** change partition tables or filesystem types
- **DO NOT** disable Arduino autostart, OTA rollback, or mbedTLS
- **DO NOT** change the `native` test environment configuration in `platformio.ini`
- **DO NOT** attempt `-flto` (Link-Time Optimization) — unstable with ESP-IDF+Arduino combo
- **DO NOT** exclude `mdns` — it IS used by wifiManager (includes `<ESPmDNS.h>`)
- **DO NOT** exclude `littlefs` — it IS the framework's filesystem
- **DO NOT** exclude any core ESP-IDF components that are required by Arduino Core internals

## Line-Item Instructions

### Part A — EXCLUDE_COMPONENTS in CMakeLists.txt

- [ ] **1.** Modify `CMakeLists.txt` (currently 4 lines) to add the `EXCLUDE_COMPONENTS` set BEFORE the `include()` line. The final file should be:

```cmake
cmake_minimum_required(VERSION 3.16.0)

# --- PR-10: Exclude unused Arduino Core managed dependencies ---
# The Arduino Core's idf_component.yml declares ~27 managed dependencies.
# Most are unused by this framework. EXCLUDE_COMPONENTS tells the ESP-IDF
# build system to skip their compilation entirely, saving significant flash.
# IMPORTANT: This list MUST appear BEFORE include(project.cmake).

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
project(esp32-app-foundation)
```

> **NOTES on exclusion decisions:**
> - `espressif__esp_schedule`, `espressif__esp_secure_cert_mgr`, `espressif__esp_rcp_update`, `espressif__json_generator`, `espressif__json_parser`, `espressif__cjson` are NOT in the actual `idf_component.yml` — they may be transitive deps that the build system resolves automatically when their parent is excluded
> - `espressif__dl_fft` is NOT in the manifest — omit it
> - `espressif__esp_hosted`, `espressif__esp_wifi_remote`, `espressif__lan867x` are target-gated to `esp32p4` only — they won't compile for esp32/s3 anyway, no need to exclude
> - `mdns` is **KEPT** — wifiManager includes `<ESPmDNS.h>`
> - `littlefs` (joltwallet/littlefs) is **KEPT** — framework's exclusive filesystem

### Part A — Verification (BEFORE Part B)

- [ ] **2. Clean build esp32dev with EXCLUDE_COMPONENTS only:**
  First delete the cached config so PIO regenerates:
  ```bash
  rm -rf .pio/build/esp32dev/config/
  ```
  Then:
  - `mcp_platformio_clean_project` (environment: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - Wait for completion, then:
  - `mcp_platformio_build_project` (environment: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - **Record the firmware size** — this is the "Part A delta"

- [ ] **3. Troubleshoot any build errors:**
  If EXCLUDE_COMPONENTS causes a dependency chain error:
  - The error message will name the missing component (e.g., `CMake Error: Component 'X' depends on excluded component 'Y'`)
  - **Remove that component from the EXCLUDE_COMPONENTS list** — it's a transitive dependency that another kept component needs
  - Rebuild and re-test
  - **Document which component was removed and why**
  - Iterate until the build succeeds

### Part B — Compiler & Linker Optimization (sdkconfig)

After Part A is verified, apply these settings to **both** `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3nano`:

- [ ] **4. Switch to -Os (optimize for size):**
  Add to the PR-10 section:
  ```
  # --- PR-10 WO2: Compiler & linker optimization for size ---

  # Switch from -Og (debug) to -Os (optimize for size)
  CONFIG_COMPILER_OPTIMIZATION_SIZE=y
  ```

- [ ] **5. Enable Newlib nano formatting:**
  ```
  # Newlib nano — smaller printf/scanf (~25-50KB savings)
  CONFIG_NEWLIB_NANO_FORMAT=y
  ```

- [ ] **6. Set compile-time log maximum:**
  ```
  # Strip VERBOSE and DEBUG log strings at compile time
  CONFIG_LOG_MAXIMUM_LEVEL=3
  ```

- [ ] **7. Use silent assertions:**
  ```
  # Use silent assertions — saves flash from assertion message strings
  CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT=y
  ```

### Part B — Verification

- [ ] **8. Clean build esp32dev with all changes:**
  ```bash
  rm -rf .pio/build/esp32dev/config/
  ```
  - `mcp_platformio_clean_project` (environment: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (environment: `esp32dev`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - **Record the FINAL firmware size**

- [ ] **9. Build esp32s3nano:**
  - `mcp_platformio_clean_project` (environment: `esp32s3nano`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - `mcp_platformio_build_project` (environment: `esp32s3nano`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - Confirm it compiles successfully

- [ ] **10. Run native tests:**
  - `mcp_platformio_run_tests` (environment: `native`, projectDir: `/Users/mcneillm/Documents/Projects/esp32-app-foundation`, background: true)
  - Confirm all 349 tests pass

- [ ] **11. Troubleshooting for Part B:**
  - If `-Os` causes build failure: try `CONFIG_COMPILER_OPTIMIZATION_PERF=y` (-O2) instead
  - If newlib nano causes linking errors: revert `CONFIG_NEWLIB_NANO_FORMAT=y`
  - Document any reverts

## Expected Outcome

- **Part A (EXCLUDE_COMPONENTS):** 100–300KB savings — this is the biggest lever
- **Part B (compiler optimization):** 50–150KB additional savings
- **Combined WO2 target:** bring binary from ~1,045KB down to ~650–800KB
- Both `esp32dev` and `esp32s3nano` compile successfully
- All 349 native tests pass

## Completion Protocol

When finished, **STOP and send a message back to the Principal Architect** (your parent agent) with:
1. Part A firmware size (after EXCLUDE_COMPONENTS only)
2. Part B firmware size (after compiler optimization added)
3. Size delta from WO2 baseline (1,044,897 bytes)
4. Cumulative delta from original baseline (1,056,085 bytes)
5. Which components were successfully excluded
6. Which components (if any) had to be kept (removed from exclude list) and why
7. Which compiler settings were applied successfully
8. Native test results (pass/fail count)

Do NOT wrap up, distill, or commit until the PA explicitly approves your work.
