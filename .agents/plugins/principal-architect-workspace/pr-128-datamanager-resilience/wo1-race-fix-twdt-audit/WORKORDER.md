---
complexity: standard
parent_pr: pr-128-datamanager-resilience
branch: 128-datamanager-resilience/wo1-race-fix-twdt-audit
base: main
---

# WO1 — Abort-on-success race fix + framework TWDT audit

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Your task is strictly limited to executing the checklist below.

## Objective
Fix the live race in which a *successful* fetch can still reboot the device, and produce the
framework-wide Task Watchdog audit that de-risks WO4's supervisor rework.

## Background

`DataManager::drainJobs()` and `::executeLoop()` both end a supervised window like this:

```cpp
stopFetchWatchdog();        // calls esp_timer_stop()
_fetchStartMs.store(0);
```

`esp_timer_stop()` does not wait for a callback that has already been dispatched. A
`fetchWatchdogCb` already running on the esp_timer task can read `_fetchStartMs` in the window
after `esp_timer_stop()` returns but before `store(0)` lands — and if the elapsed time exceeds
30 s it calls `abort()` on a fetch **that already returned successfully**.

The case it hits is precisely the long-but-recoverable fetch the timer exists to adjudicate:
the device reboots, and the log claims the fetch hung when in fact it completed.

Reported by the consumer that filed #128. Confirmed at both call sites.

## Strict Boundary Box

- **Do NOT** begin the health model, `FetchContext`, cancellation, or removal of `abort()` —
  those are WO2/WO3/WO4. This WO leaves the existing mechanism in place and makes it correct.
- **Do NOT** change `CONFIG_ESP_TASK_WDT_*` in any sdkconfig. The audit informs WO4; it does
  not act.
- **Do NOT** touch `modules/OTAUpdateManager/` behaviour. It is read-only input to the audit.
- **Do NOT** file GitHub issues. Surface findings in the Completion Report.

## Tooling Constraints

Use the `platformio` MCP tools (`build_project`, `run_tests`) — never raw `pio` CLI.

## Line-Item Instructions

- [ ] `modules/DataManager/DataManager.cpp` — in `drainJobs()`, order `_fetchStartMs.store(0)`
      **before** `stopFetchWatchdog()` so the callback's existing `if (start == 0) return;`
      guard covers the dispatch window. Comment why the order matters.
- [ ] `modules/DataManager/DataManager.cpp` — same inversion at the `executeLoop()` call site.
- [ ] `modules/WebServer/WebServerManager.cpp` — the `esp_task_wdt_reset()` in the
      restart/cert-generation wait runs on the Arduino `loopTask`, which is not a TWDT
      subscriber (`loopTaskWDTEnabled = false` unless `enableLoopWDT()` is called; the
      framework never calls it). The call is a silent no-op returning `ESP_ERR_NOT_FOUND`.
      Replace with an honest comment rather than leaving code that implies protection.
- [ ] `test/test_unit/test_dataManager/test_main.cpp` — add coverage asserting `_fetchStartMs`
      is cleared before the supervisor is torn down, via the existing native seam.
- [ ] `.agents/skills/principal-architect-workspace/pr-128-datamanager-resilience/wo1-race-fix-twdt-audit/TWDT-AUDIT.md`
      — the audit artifact: every task, its subscription state, budget, and stack.

## Verification Requirement

Native suite green via `run_tests`; at least one board builds via `build_project`.

## Issue Surfacing
Surface out-of-scope findings in the Completion Report. Do not file them.

## Completion Report
_To be filled in on completion._
