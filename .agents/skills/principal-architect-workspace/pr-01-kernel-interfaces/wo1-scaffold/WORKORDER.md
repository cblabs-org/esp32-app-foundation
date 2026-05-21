---
name: pr-01-wo1
description: Handoff instructions for Work Order 1 — Repo Scaffold, PlatformIO Config, Partition Tables, sdkconfig.defaults
---

# PR-01 / WO1 — Repo Scaffold & PlatformIO Baseline

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

## Objective

Bootstrap `esp32-app-foundation` as a proper git repository, create the `pr-01-kernel-interfaces` feature branch and your `wo1-scaffold` working branch, then write all PlatformIO configuration, partition tables, and sdkconfig files. This WO creates the skeleton directories and root-level config that all subsequent WOs depend on.

## Context

- **Repo location:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation/` (absolute path for git/MCP commands only — all file content must use repo-relative paths)
- **Current state:** The directory contains only `docs/` and `.agents/` — NOT yet a git repo
- **Framework decision (CA-06):** Dual framework `arduino + espidf` on all hardware envs
- **Platform target:** `espressif32` — two board variants: `esp32dev` and `esp32s3nano`
- **Native test env:** `[env:native]` + GoogleTest for host-only unit tests
- **Partition strategy:** Custom CSV tables per board; both reserve 2MB OTA slots + 1.5MB LittleFS
- **sdkconfig rollback:** `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is **mandatory** in both sdkconfig files

## Strict Boundary Box

- ✅ Git bootstrap, `.gitignore`, directory skeleton, `platformio.ini`, partition CSVs, sdkconfig files
- ❌ Do NOT write any `.cpp` or `.hpp` source files (those are WO2–WO4)
- ❌ Do NOT touch `departures-board/` (reference only — do not import files from it)
- ❌ Do NOT add any `lib_deps` for optional modules (U8G2, etc.) — core dependencies only
- ❌ Do NOT modify `docs/` or `.agents/`

---

## Execution Checklist

### Phase 0 — Git Bootstrap

> [!CAUTION]
> This is the FIRST action you must take. The repo does not exist yet.

- [ ] **0.1** Write `.gitignore` at the repo root with AT MINIMUM:
  ```
  .pio/
  .vscode/
  build/
  .DS_Store
  *.o
  *.a
  *.elf
  *.bin
  *.map
  include/portal_assets/
  ```

- [ ] **0.2** Run the git bootstrap sequence:
  ```bash
  cd /Users/mcneillm/Documents/Projects/esp32-app-foundation
  git init
  git checkout -b main
  git add .gitignore docs/ .agents/
  git commit -m "chore: initial repo scaffold — docs, agents, gitignore"
  ```

- [ ] **0.3** Create the PR-01 feature branch:
  ```bash
  git checkout -b pr-01-kernel-interfaces main
  ```

- [ ] **0.4** Create the WO1 working branch:
  ```bash
  git checkout -b pr-01-kernel-interfaces/wo1-scaffold pr-01-kernel-interfaces
  ```

- [ ] **0.5** Verify you are on the correct branch:
  ```bash
  git branch --show-current
  # Expected output: pr-01-kernel-interfaces/wo1-scaffold
  ```

---

### Phase 1 — Directory Skeleton

Create the following empty directories (add `.gitkeep` files where needed):

- [ ] **1.1** Create directory structure:
  ```
  include/
  include/interfaces/
  lib/
  modules/
  modules/application/
  src/
  test/
  test/test_unit/
  test/test_unit/test_application/
  test/test_hardware/
  test/test_e2e/
  data/
  scripts/
  web/
  web/portal/
  template/
  ```
  Add a `.gitkeep` to each empty leaf directory so git tracks it.

---

### Phase 2 — `platformio.ini`

- [ ] **2.1** Write `platformio.ini` at the repo root. Exact content required:

```ini
; =============================================================================
; ESP32 App Foundation — PlatformIO Configuration
; =============================================================================
; Dual-framework (Arduino + ESP-IDF) monorepo configuration.
; Three environments:
;   esp32dev        — Espressif ESP-WROOM-32 (4MB flash, PSRAM optional)
;   esp32s3nano     — Unexpected Maker ESP32-S3 Nano (8MB flash, QSPI PSRAM)
;   native          — Host CPU GoogleTest runner (no hardware required)
;
; IMPORTANT: When building or flashing, ALWAYS specify the environment explicitly
; to prevent PlatformIO from targeting all envs simultaneously.
; =============================================================================

[platformio]
default_envs = esp32dev, esp32s3nano

; ---------------------------------------------------------------------------
; Shared base — inherited by both hardware environments
; ---------------------------------------------------------------------------
[env:base_esp32]
platform              = espressif32
framework             = arduino, espidf
monitor_speed         = 115200
upload_speed          = 921600
build_flags           =
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_ASYNC_TCP_RUNNING_CORE=1
    -DCONFIG_ASYNC_TCP_STACK_SIZE=4096
    -DAPP_FOUNDATION_VERSION=\"0.1.0\"
build_unflags         = -std=gnu++11
build_flags           += -std=gnu++17
lib_compat_mode       = strict

; ---------------------------------------------------------------------------
; esp32dev — ESP-WROOM-32 / generic ESP32
; ---------------------------------------------------------------------------
[env:esp32dev]
extends               = env:base_esp32
board                 = esp32dev
board_build.partitions = partitions.esp32dev.csv
board_build.filesystem = littlefs
custom_sdkconfig      = sdkconfig.defaults
lib_deps              =
    bblanchon/ArduinoJson@^7.4.3
    ESP32Async/ESPAsyncWebServer@^3.7.7
    ESP32Async/AsyncTCP@^3.4.5

; ---------------------------------------------------------------------------
; esp32s3nano — Unexpected Maker ESP32-S3 Nano
; ---------------------------------------------------------------------------
[env:esp32s3nano]
extends               = env:base_esp32
board                 = um_nanos3
board_build.partitions = partitions.esp32s3nano.csv
board_build.filesystem = littlefs
board_build.arduino.memory_type = qio_opi
custom_sdkconfig      = sdkconfig.defaults.esp32s3nano
build_flags           =
    ${env:base_esp32.build_flags}
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
lib_deps              =
    bblanchon/ArduinoJson@^7.4.3
    ESP32Async/ESPAsyncWebServer@^3.7.7
    ESP32Async/AsyncTCP@^3.4.5

; ---------------------------------------------------------------------------
; native — GoogleTest host runner (no hardware)
; ---------------------------------------------------------------------------
[env:native]
platform              = native
build_flags           =
    -std=c++17
    -DNATIVE_BUILD
    -DUNIT_TEST
lib_deps              =
    google/googletest@^1.15.2
test_framework        = googletest
```

---

### Phase 3 — Partition Tables

- [ ] **3.1** Write `partitions.esp32dev.csv`:

```csv
# Name,   Type, SubType, Offset,   Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0xF0000,
app1,     app,  ota_1,   0x100000, 0xF0000,
littlefs, data, spiffs,  0x1F0000, 0x180000,
coredump, data, coredump,0x370000, 0x10000,
```

> [!NOTE]  
> esp32dev: 4MB flash. app0+app1 = 2×960KB OTA slots. LittleFS = 1.5MB.

- [ ] **3.2** Write `partitions.esp32s3nano.csv`:

```csv
# Name,   Type, SubType, Offset,   Size,    Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x1F0000,
app1,     app,  ota_1,   0x200000, 0x1F0000,
littlefs, data, spiffs,  0x3F0000, 0x3C0000,
coredump, data, coredump,0x7B0000, 0x10000,
```

> [!NOTE]  
> esp32s3nano: 8MB flash. app0+app1 = 2×2MB OTA slots. LittleFS = 3.75MB (PSRAM board gets more headroom).

---

### Phase 4 — sdkconfig.defaults

- [ ] **4.1** Write `sdkconfig.defaults`:

```
# ESP32 App Foundation — sdkconfig.defaults (esp32dev)
# OTA rollback support — MUST NOT be disabled
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y

# Increase main task stack for complex startup sequences
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192

# Enable LittleFS
CONFIG_LITTLEFS_FOR_IDF_3_2=y

# Disable Bluetooth to reclaim ~70KB RAM (not needed for this framework)
CONFIG_BT_ENABLED=n

# HTTPS/TLS — mbedTLS heap-friendly settings
CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=8192
CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN=4096

# FreeRTOS tick rate
CONFIG_FREERTOS_HZ=1000
```

- [ ] **4.2** Write `sdkconfig.defaults.esp32s3nano`:

```
# ESP32 App Foundation — sdkconfig.defaults.esp32s3nano
# Inherits base defaults; S3-specific overrides below.
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y

# S3 main task stack — larger due to PSRAM availability
CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384

CONFIG_LITTLEFS_FOR_IDF_3_2=y

CONFIG_BT_ENABLED=n

# S3 PSRAM — QSPI OPI for UM NanoS3
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_MEMTEST=y

CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384
CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN=8192

CONFIG_FREERTOS_HZ=1000
```

---

### Phase 5 — Build Verification & Commit

- [ ] **5.1** Verify the project config is readable via MCP:
  ```
  mcp_platformio_get_project_config(projectDir="<absolute-path-to-repo>")
  ```

- [ ] **5.2** Attempt a clean of the `native` env to confirm PlatformIO accepts the config:
  ```
  mcp_platformio_clean_project(
    projectDir="<absolute-path-to-repo>",
    environment="native",
    background=true
  )
  ```
  Poll with `mcp_platformio_check_task_status` until complete.

- [ ] **5.3** Attempt a build of `esp32dev` to confirm the dual-framework config is valid:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="esp32dev",
    background=true
  )
  ```
  Poll until done. This is expected to succeed (empty main.cpp stub is acceptable for now).
  If it fails because `src/main.cpp` is missing, write a minimal stub:
  ```cpp
  // Temporary scaffold stub — replaced by WO4
  extern "C" void app_main() {}
  ```

- [ ] **5.4** Stage and commit all WO1 work to the `wo1-scaffold` branch:
  ```bash
  git add .
  git commit -m "feat(scaffold): PlatformIO config, partition tables, sdkconfig.defaults

  - platformio.ini: dual-env esp32dev + esp32s3nano + native
  - partitions.esp32dev.csv: 2x960KB OTA + 1.5MB LittleFS
  - partitions.esp32s3nano.csv: 2x2MB OTA + 3.75MB LittleFS
  - sdkconfig.defaults: rollback enabled, BT disabled, FreeRTOS 1kHz
  - sdkconfig.defaults.esp32s3nano: S3 PSRAM QSPI OPI config
  - Directory skeleton with .gitkeep sentinels"
  ```

---

## Verification Requirement

Before considering this WO complete, confirm:
1. `git log --oneline` shows your commit on `pr-01-kernel-interfaces/wo1-scaffold`
2. `mcp_platformio_get_project_config` returns the three environments (esp32dev, esp32s3nano, native)
3. `esp32dev` build produces no fatal errors (linker errors due to missing src are acceptable)
4. The directory skeleton matches the layout in the PROGRAM-PLAN.md

**STOP and ask the user to confirm the work before committing or invoking the distillery.**  
Once approved, invoke the `@distillery` skill, passing `.agents/skills/principal-architect-workspace/pr-01-kernel-interfaces/wo1-scaffold/` as the target directory. Then append a `## Status` section to this WORKORDER.md with the outcome and drop control.

---

## Status

*Awaiting execution.*
