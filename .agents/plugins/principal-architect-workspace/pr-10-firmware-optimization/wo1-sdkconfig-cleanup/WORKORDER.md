---
name: pr-10-wo1
description: Handoff instructions for Work Order 1 — Disable unused ESP-IDF components via sdkconfig.defaults to reduce firmware binary size.
---

# PR-10 / WO1 — sdkconfig.defaults Cleanup

## Role Definition

You are an expert senior embedded systems engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Mandatory Skill Boot

Before doing ANY work, you MUST load these skills:
1. `view_file` on `.agents/skills/pio-manager/SKILL.md`
2. `view_file` on `.agents/skills/embedded-systems/SKILL.md`
3. `view_file` on `.agents/skills/house-style-docs/SKILL.md`

## Objective

Disable unused ESP-IDF components in `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3nano` to eliminate dead code from the firmware binary. The framework uses its own `wifiManager` with captive portal — not ESP-IDF provisioning. It uses LittleFS exclusively — not SPIFFS or FatFS. It uses MCP over serial — not ESP-IDF console. It does not use cellular modems, Rainmaker, or libsodium.

## Strict Boundary Box

- **DO NOT** modify any C++ source files (`.cpp`, `.hpp`, `.h`)
- **DO NOT** modify `platformio.ini` (that is WO2's scope)
- **DO NOT** disable Bluetooth — it is already disabled (`CONFIG_BT_ENABLED=n`)
- **DO NOT** disable mbedTLS — it is required for OTA signature verification
- **DO NOT** disable LittleFS — it is the framework's exclusive filesystem
- **DO NOT** disable Arduino autostart (`CONFIG_AUTOSTART_ARDUINO=y`)
- **DO NOT** disable OTA rollback (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`)
- **DO NOT** disable WiFi — only WiFi *provisioning* (a separate ESP-IDF component)
- **DO NOT** change `CONFIG_ESP_MAIN_TASK_STACK_SIZE` or `CONFIG_FREERTOS_HZ`
- **DO NOT** change mbedTLS buffer sizes (`CONFIG_MBEDTLS_SSL_IN/OUT_CONTENT_LEN`)
- **DO NOT** touch S3-specific PSRAM settings (`CONFIG_SPIRAM_*`)
- **ONLY** modify `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3nano`

## Line-Item Instructions

### Checklist — `sdkconfig.defaults`

Apply these additions to the file **below the existing entries**, in a clearly commented new section:

- [ ] **1. Disable WiFi Provisioning + Protocomm**
  ```
  # --- PR-10: Disable unused ESP-IDF components to reduce binary size ---

  # WiFi Provisioning — framework uses its own wifiManager with captive portal
  CONFIG_WIFI_PROV_SCAN_MAX_ENTRIES=0
  CONFIG_ESP_WIFI_SOFTAP_SUPPORT=y
  ```
  > **NOTE:** The Arduino+ESP-IDF combo framework does NOT expose `CONFIG_WIFI_PROVISIONING` as a direct toggle. The provisioning library is pulled in by ESP-IDF defaults. Setting `SCAN_MAX_ENTRIES=0` effectively neuters it. We must keep SoftAP support ON because the captive portal AP mode uses it.

- [ ] **2. Disable SPIFFS**
  ```
  # SPIFFS — framework uses LittleFS exclusively
  CONFIG_SPIFFS_MAX_PARTITIONS=0
  ```
  > **NOTE:** The partition table labels the LittleFS partition as SubType `spiffs` (required by ESP-IDF's VFS layer), but the actual SPIFFS library code is unused. Setting max partitions to 0 prevents SPIFFS from initializing.

- [ ] **3. Disable FatFS**
  ```
  # FatFS — no SD card support in framework
  CONFIG_FATFS_VOLUME_COUNT=0
  ```

- [ ] **4. Disable ESP Console/REPL**
  ```
  # ESP Console — framework uses MCP over serial, not ESP-IDF REPL
  CONFIG_ESP_CONSOLE_NONE=y
  CONFIG_ESP_CONSOLE_UART=n
  ```
  > **CAUTION:** This disables the ESP-IDF console component but serial UART itself still works for Arduino `Serial.begin()` and `ESP_LOGx` output. If the build fails with this setting, revert to just removing `CONFIG_ESP_CONSOLE_UART_DEFAULT` without setting NONE.

- [ ] **5. Disable PPP/Modem support**
  ```
  # PPP/Modem — no cellular modem hardware
  CONFIG_LWIP_PPP_SUPPORT=n
  ```

- [ ] **6. Disable libsodium**
  ```
  # libsodium — all crypto handled by mbedTLS (OTA signature verification, etc.)
  CONFIG_LIBSODIUM_USE_MBEDTLS_SHA=n
  ```
  > **NOTE:** `CONFIG_LIBSODIUM_USE_MBEDTLS_SHA=n` disables libsodium's mbedTLS integration. There is no master `CONFIG_LIBSODIUM_ENABLE` toggle in the Arduino+ESP-IDF build. The linker should then garbage-collect the unused libsodium code.

### Checklist — `sdkconfig.defaults.esp32s3nano`

Apply the **exact same set of disables** to this file as well, below the existing entries:

- [ ] **7.** Copy the entire `# --- PR-10: Disable unused ESP-IDF components ---` block from `sdkconfig.defaults` and append it to `sdkconfig.defaults.esp32s3nano`, preserving all S3-specific settings that are already there.

### Verification

- [ ] **8. Clean build (esp32dev):** Delete the cached sdkconfig so PIO regenerates it:
  ```
  rm -f .pio/build/esp32dev/config/sdkconfig.h
  ```
  Then run a full clean + build via MCP:
  - `mcp_platformio_clean_project` (environment: `esp32dev`, background: true)
  - Wait for completion, then:
  - `mcp_platformio_build_project` (environment: `esp32dev`, background: true)
  - Record the EXACT firmware size from the build output (the `.bin` size in bytes)

- [ ] **9. Clean build (esp32s3nano):** Same procedure for the S3 target:
  - `mcp_platformio_clean_project` (environment: `esp32s3nano`, background: true)
  - `mcp_platformio_build_project` (environment: `esp32s3nano`, background: true)
  - Confirm it compiles successfully (we only track size for esp32dev)

- [ ] **10. Run native tests:**
  - `mcp_platformio_run_tests` (environment: `native`, background: true)
  - Confirm all 349 tests pass

- [ ] **11. Troubleshooting:** If any setting causes a build failure:
  - **Identify** which specific CONFIG caused the failure from the error log
  - **Revert** only that specific setting
  - **Rebuild** and re-verify
  - **Document** which setting was reverted and why in your completion report

## Expected Outcome

- Binary size should decrease from the 1,056,085 byte baseline
- Estimated savings: 50–200KB (linker GC determines actual impact)
- Both `esp32dev` and `esp32s3nano` targets compile successfully
- All 349 native tests pass

## Completion Protocol

When finished, **STOP and send a message back to the Principal Architect** (your parent agent) with:
1. The final binary size for `esp32dev` (exact bytes)
2. The size delta from the 1,056,085 baseline
3. Which settings were applied successfully
4. Which settings (if any) had to be reverted and why
5. Native test results (pass/fail count)

Do NOT wrap up, distill, or commit until the PA explicitly approves your work.
