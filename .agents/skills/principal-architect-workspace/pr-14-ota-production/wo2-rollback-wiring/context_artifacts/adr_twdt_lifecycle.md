# ADR: TWDT Lifecycle Management in OTA Task

## Context

The OTA download/flash pipeline runs in a dedicated FreeRTOS task. A hung download (network timeout, corrupted stream, infinite redirect loop) could leave the device stuck indefinitely, preventing it from rebooting and rolling back to a known-good firmware.

## Decision

1. **Subscribe** the OTA task to the Task Watchdog Timer (TWDT) at the start of `performUpdateTask()` via `esp_task_wdt_add(NULL)`.
2. **Feed** the watchdog after each successful chunk write in the download loop via `esp_task_wdt_reset()`.
3. **Unsubscribe** the task via `esp_task_wdt_delete(NULL)` on **every** exit path — both the success path (before `ESP.restart()`) and all 8 early error returns.
4. **Enable panic mode** via `CONFIG_ESP_TASK_WDT_PANIC=y` in both `sdkconfig.defaults` files, so the watchdog triggers a reboot instead of just logging.

## Rationale

- **Every exit path must unsubscribe**: If a return statement exits `performUpdateTask()` without calling `esp_task_wdt_delete(NULL)`, the TWDT subscription remains active on a task that is no longer feeding. The next time the watchdog fires, it would panic+reboot the device even though the OTA was cleanly aborted.
- **TWDT feeds in the download loop only**: The download loop is the long-running section; all other steps (GitHub API query, signature fetch, verifier init) complete quickly enough to not trigger the default 5-second watchdog timeout.
- **Panic mode is intentional**: Combined with `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`, a TWDT panic during OTA will reboot the device, and if the firmware hasn't been blessed by `markAppValid()`, the bootloader automatically reverts.

## Consequences

- The OTA task must not block for longer than the TWDT timeout (~5s default) without feeding.
- Any future additions to `performUpdateTask()` that add early return paths must include `esp_task_wdt_delete(NULL)`.
