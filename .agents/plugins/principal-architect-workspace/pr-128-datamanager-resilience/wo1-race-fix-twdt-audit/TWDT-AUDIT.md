# Framework Task Watchdog Audit

> Prerequisite for WO4's supervisor rework. Catalogues every task, its TWDT subscription
> state, its execution budget, and every `CONFIG_ESP_TASK_WDT_*` setting — so the watchdog
> architecture is changed against measured fact rather than assumption.
>
> State captured at `main` @ `1bbfdb98` (post-#136).

## 1. Configuration

`sdkconfig.defaults` sets **only** `CONFIG_ESP_TASK_WDT_PANIC=y`. Every generated per-board
sdkconfig (`esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`, `esp32dev_ha`) carries an
identical block:

| Setting | Value |
|---|---|
| `CONFIG_ESP_TASK_WDT_EN` | `y` |
| `CONFIG_ESP_TASK_WDT_INIT` | `y` |
| `CONFIG_ESP_TASK_WDT_PANIC` | `y` |
| `CONFIG_ESP_TASK_WDT_TIMEOUT_S` | **5** |
| `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0` | `y` |
| `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1` | `y` |

`PANIC=y` was set deliberately in PR-14 so a hung OTA download triggers bootloader rollback.

## 2. Task inventory

| Task | Created at | Stack | Prio | TWDT | Budget |
|---|---|---|---|---|---|
| `loopTask` (Arduino main loop) | Arduino core `main.cpp:113` | 8192 (`CONFIG_ARDUINO_LOOP_STACK_SIZE`) | 1 | **No** — see §3 | 10 ms cooperative tick budget (framework policy, not enforced by TWDT) |
| `DataManager` worker | `DataManager.cpp:93` | 12288 DRAM | 5 | **Yes**, but *deleted for the duration of every fetch/job* | unbounded — the subject of #128 |
| OTA worker | `OTAUpdateManager.cpp:243` (`xTaskCreateWithCaps`, internal DRAM) | — | — | **Yes** — own subscription, `add` at :669, `delete` at :671 | fed every ~4 KB from the download progress callback (`:615-625`) |
| `cert_gen` | `AuthManager.cpp:683` and `:753` | 12288 DRAM | 1 | **No** — deliberate; short-lived and self-deleting (comment at `:749`) | mbedTLS EC keygen; seconds |
| `DeepStackExecutor` worker | `DeepStackExecutor.cpp:59` (`xTaskCreateWithCaps`, PSRAM) | `_stackBytes` (16 KB placeholder — see #133) | `tskIDLE_PRIORITY+1` | **No** | tdefl compression, ~2–3 s for 240×240 |
| `httpd` (×2) | created by `esp_http_server` | `BOARD_HTTP_STACK_SIZE` (8 KB) | — | **No** | per-request; deep work offloaded to DeepStackExecutor |
| Idle × 2 | FreeRTOS | — | 0 | **Yes** — via `CHECK_IDLE_TASK_CPU0/1` | starvation detector |

Two tasks are named `httpd` (HTTP and HTTPS servers), so `xTaskGetHandle("httpd")` returns
whichever was registered first — documented in `WebServerManager.cpp` during #136.

## 3. Finding — the main loop has no watchdog supervision at all

Arduino-ESP32 initialises `loopTaskWDTEnabled = false` (core `main.cpp:111`) and only
subscribes `loopTask` if the application calls `enableLoopWDT()`. **This framework never calls
it.** The main loop is therefore not a TWDT subscriber on any board.

Two consequences:

1. **`WebServerManager.cpp:660` is a silent no-op.** The restart path busy-waits up to 10 s for
   certificate generation, calling `esp_task_wdt_reset()` each 100 ms to "feed the watchdog".
   That call runs on `loopTask`, which has no subscription, so it returns `ESP_ERR_NOT_FOUND`
   (unchecked) and protects nothing. The wait is harmless *because* nothing is watching — but
   the code reads as though supervision exists.

2. **It moots a design objection raised in review of #128.** The concern that raising
   `CONFIG_ESP_TASK_WDT_TIMEOUT_S` from 5 → 20 would "weaken main-loop supervision" does not
   hold: there is no main-loop supervision to weaken. A global raise would still degrade
   **idle-task** starvation detection on both cores, which remains a real objection — but the
   main-loop half of the argument is void.

   It also strengthens the case for an application-level heartbeat as the correct instrument
   for the 10 ms tick budget. That is not a relaxation of existing protection; it is the first
   protection the main loop would have.

## 4. Finding — the supervised window is inverted from intent

The `DataManager` worker holds a TWDT subscription during the cheap, bounded part of its loop
(scheduling, registry walk) and **drops it for the expensive, unbounded part** (the fetch).
Supervision is applied exactly where it is not needed and removed exactly where it is.

This is the structural observation behind #128 §2.2, recorded here because WO4 reverses it.

## 5. Finding — the race this WO fixes

`esp_timer_stop()` does not wait for an already-dispatched callback. Both supervised windows
close with `stopFetchWatchdog()` before `_fetchStartMs.store(0)`, leaving a window in which an
in-flight `fetchWatchdogCb` can observe a stale non-zero start stamp and `abort()` a fetch that
already returned. Fixed in this WO by inverting the two statements at both call sites, which
brings the callback's existing `if (start == 0) return;` guard over the window.

## 6. Implications carried into WO4

- Only **two** framework tasks ever subscribe, and OTA's subscription is independent — so
  changing `DataManager`'s policy cannot affect OTA rollback.
- A global `CONFIG_ESP_TASK_WDT_TIMEOUT_S` raise costs **idle-task** detection only. That is
  still a real cost, so the narrower mechanisms (bounded handshake, or
  `esp_task_wdt_reconfigure()` across the handshake alone) remain preferred.
- Feeding must happen **on the worker task**. `esp_task_wdt_reset()` acts on the calling task,
  so a supervising timer cannot feed on the worker's behalf. The OTA progress callback at
  `OTAUpdateManager.cpp:615-625` is the in-repo precedent for doing it correctly.
