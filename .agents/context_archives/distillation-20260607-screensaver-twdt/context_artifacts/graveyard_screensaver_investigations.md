# Graveyard: Screensaver Crash Investigations — Dead Ends

## Dead End 1: "setPowerSave(1) caused an MSPI bus fault"

**Hypothesis:** `setPowerSave(1)` sends `0xAE` over SPI. On the UM NanoS3, the display SPI and the
octal PSRAM both share the MSPI bus. The power-save command was thought to cause MSPI bus contention.

**Evidence used:** A prior session decoded a backtrace and pointed to SPI operations.

**Why it was wrong:** The backtrace decode had an ELF mismatch — the binary on the device was from
a different build than the ELF file used to decode symbols. The addresses were meaningless.

**Proof it was wrong:** After replacing `setPowerSave(1)` with `setContrast(0)`, the crash continued
identically at 313s. After removing ALL hardware calls from `_blank()`, the crash still continued at
313s. The SPI command was irrelevant.

---

## Dead End 2: "setContrast(0) caused MSPI cache contention"

**Hypothesis:** `setContrast(0)` holds the MSPI bus, pausing the cache on both cores. `async_tcp`
on CPU 1 can only run from IRAM during cache pause, and was thought to spin-wait, starving IDLE1.

**Evidence:** `setContrast(0)` crash was at the same timestamp as `setPowerSave(1)` crash.

**Why it was wrong:** The cache pause from a contrast command takes microseconds. IDLE1 was starved
for 5 full seconds. The SPI transaction was never the cause of the sustained starvation.

**Proof:** Removing all hardware calls → crash still at 313s.

---

## Dead End 3: "async_tcp was causing the starvation independently"

**Hypothesis:** Something in `async_tcp`'s internal state machine causes it to spin for 5+ seconds
at ~300s. A connection pool buildup from HTTP polling, a TCP timeout check, or OTA retry was suspected.

**Evidence:** `task_wdt: CPU 1: async_tcp` — async_tcp was the running task when IDLE1 timed out.

**Why it was wrong:** async_tcp was the SYMPTOM not the CAUSE. On CPU 1, both `loopTask` and
`async_tcp` compete. When `loopTask` ran tight (no rendering delay), it alternated with `async_tcp`
at such high frequency that `IDLE1` (lowest priority) never got scheduled. It wasn't async_tcp
running continuously — it was both tasks alternating so fast that IDLE1 was squeezed out.

**Proof:** Adding `vTaskDelay(33ms)` — with no change to async_tcp — fixed the crash entirely.

---

## Dead End 4: "clearBuffer/sendBuffer is the correct final solution"

**Status:** Worked (484s stable), but not optimal.

**Why abandoned:** `clearBuffer/sendBuffer` pushes all-zero pixels to GDDRAM. Display controller
stays fully powered — oscillator running, row/column drivers scanning, DC-DC boost converter on.
Visually dark but drawing ~10mA vs <1mA for genuine hardware sleep.

**Replaced by:** `setPowerSave(1)` which was re-tested with the vTaskDelay fix in place and confirmed
working at 379s. The hardware sleep command is the correct final implementation.

---

## Dead End 5: "The USB console was on /dev/cu.usbserial-* or debug-console"

**Hypothesis:** Prior sessions tried multiple monitor ports to capture crash output.

**Why it was wrong:** The UM NanoS3 uses USB-JTAG (not UART) for the primary console. The correct
device node is `/dev/cu.usbmodem101`. Standard `/dev/cu.usbserial-*` ports are NOT the JTAG port
and will show nothing or garbage.

**Additionally:** `sdkconfig.defaults.esp32s3nano` had `CONFIG_ESP_CONSOLE_NONE=y` (from PR-19),
which disabled all console output entirely. Both the wrong port AND the disabled console made crash
diagnosis impossible for multiple sessions.

---

## The Actual Crash (for reference)

```
I (307896) ScreensaverCtrl: Display blanked (entering power save)
E (313872) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (313872) task_wdt:  - IDLE1 (CPU 1)
E (313872) task_wdt: Tasks currently running:
E (313872) task_wdt: CPU 0: IDLE0
E (313872) task_wdt: CPU 1: async_tcp
E (313872) task_wdt: Aborting.
Backtrace: 0x42006cda:0x3fcb94d0
Rebooting...
```

Timeline: blank at 307.9s → IDLE1 starved from ~308.9s → TWDT fires at 313.9s (5s starvation).
CPU 0 was IDLE (main task had yielded there), CPU 1 was occupied by async_tcp with no IDLE1 slot.
