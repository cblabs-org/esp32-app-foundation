# ADR: sdkconfig Base+Delta Layering Architecture

**Decision:** Promote `sdkconfig.defaults` to a shared canonical base for all board environments,
and reduce `sdkconfig.defaults.esp32s3nano` to a true delta containing only the 4 categories of
genuinely-different settings.

**Context:** The two `.defaults` files were previously independent copies with ~90% duplicated content.
ESP-IDF's `kconfig.cmake` does NOT watch `.defaults` files as CMake dependencies — only the generated
`sdkconfig.<env>` file is a tracked CMake input. This means changes to one `.defaults` file are never
reflected in another board's build unless its generated `sdkconfig.*` is explicitly deleted first.

**The PR-14 WO9 Regression (Root Cause):**
The TCP window fix (`CONFIG_LWIP_TCP_WND_DEFAULT=32768`) was added to `sdkconfig.defaults` (esp32dev base)
only. Because `sdkconfig.defaults.esp32s3nano` was an independent copy and was NOT updated, the S3 Nano's
generated `sdkconfig.esp32s3nano` still had `# CONFIG_SPIRAM is not set` and was missing the TCP window
settings. This produced silent PSRAM failures and degraded OTA performance on the S3 Nano.

**Decision Made:**
Use the official PlatformIO/ESP-IDF mechanism for multi-file SDKCONFIG_DEFAULTS:
```ini
board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3nano"
```
ESP-IDF's `project.cmake` accepts a semicolon-separated list. `kconfgen` merges them left-to-right;
later files win on conflicts. Base is loaded first, delta overrides it.

**Why NOT keep `custom_sdkconfig`?**
The `custom_sdkconfig` PlatformIO key was a single-file mechanism — it replaced the default rather than
layering. It could not load two files simultaneously. The `board_build.cmake_extra_args` approach passes
`-DSDKCONFIG_DEFAULTS` directly to CMake which is the upstream ESP-IDF supported mechanism.

**Why NOT create `sdkconfig.defaults.esp32` for the esp32dev env?**
The base `sdkconfig.defaults` already covers esp32dev. Adding a redundant `sdkconfig.defaults.esp32`
would add indirection with no benefit. Only boards that genuinely diverge from the base need a delta file.

**Delta Contents (4 categories, 7 config keys):**
1. `CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384` — S3 has PSRAM; larger stack than base (8192)
2. `CONFIG_SPIRAM=y`, `CONFIG_SPIRAM_MODE_OCT=y`, `CONFIG_SPIRAM_SPEED_80M=y`, `CONFIG_SPIRAM_MEMTEST=y` — OPI PSRAM hardware present on Waveshare S3-Nano
3. `CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN=8192` — S3 has more RAM; larger TLS out buffer than base (4096)
4. `CONFIG_ESP_CONSOLE_NONE=y`, `CONFIG_ESP_CONSOLE_UART=n` — S3 uses USB-JTAG console, not UART

**WO2 Dependency:**
WO2 will add `scripts/sync_sdkconfig.py` — a drift detection script that diffs the base against all delta
files to alert when settings diverge unexpectedly. This closes the feedback loop that allowed the PR-14
regression to go undetected.
