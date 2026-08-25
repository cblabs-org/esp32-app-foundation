# Project: issue-sweep-aug

> Close out 11 actionable open issues in a single stacked PR. Covers P1 compilation
> blockers, TWDT safety, portal DRAM tuning, LVGL/portal branding fixes, skill/doc
> updates, SLS naming, test fixes, and a screensaver policy UI feature.

## Human Approval

- [x] Decomposition approved by human: 2026-08-19T20:39+01:00

## Work Order Ledger

| WO | Branch | Status | Closes | Summary |
|----|--------|--------|--------|---------|
| **[x] [wo1-ota-compilation-fixes](wo1-ota-compilation-fixes/WORKORDER.md)** | `issue-sweep-aug/wo1-ota-compilation-fixes` | ✅ Complete | #120 | Fix 2 compilation defects blocking `WITH_OTA` on consumer apps |
| **[x] [wo2-datamanager-twdt-safety](wo2-datamanager-twdt-safety/WORKORDER.md)** | `issue-sweep-aug/wo2-datamanager-twdt-safety` | ✅ Complete | #128 | Harden DataManager fetch loop against TWDT boot-loop |
| **[x] [wo3-portal-dram-tuning](wo3-portal-dram-tuning/WORKORDER.md)** | `issue-sweep-aug/wo3-portal-dram-tuning` | ✅ Complete | #114, #119 | Reduce httpd stack/sockets for constrained boards + HWM logging |
| **[x] [wo4-branding-fouc](wo4-branding-fouc/WORKORDER.md)** | `issue-sweep-aug/wo4-branding-fouc` | ✅ Complete | #129, #80 | LVGL boot title + portal branding FOUC elimination |
| **[x] [wo5-docs-sls-tests](wo5-docs-sls-tests/WORKORDER.md)** | `issue-sweep-aug/wo5-docs-sls-tests` | ✅ Complete | #127, #130, #97, #113 | Skill/doc updates, PSRAM audit, EventBus payload lifetime doc, SLS naming fix, sample-app test fix |
| **[x] [wo6-screensaver-policy-ui](wo6-screensaver-policy-ui/WORKORDER.md)** | `issue-sweep-aug/wo6-screensaver-policy-ui` | ✅ Complete | #110 | Screensaver policy toggle in portal Display settings |
| **[x] [wo7-lvgl-eventbus-race](wo7-lvgl-eventbus-race/WORKORDER.md)** | `issue-sweep-aug/wo7-lvgl-eventbus-race` | ✅ Complete | #131 | Fix LVGL race in EventBus subscriber + document threading rule |
| **[x] [wo8-queue-data-job](wo8-queue-data-job/WORKORDER.md)** | `issue-sweep-aug/wo8-queue-data-job` | ✅ Complete | #132 | Extend DataManager with queueDataJob() for off-loop data operations |

## Modules & Components

| Module / Area | WOs | Notes |
|---|---|---|
| `modules/OTAUpdateManager/` | WO1 | Lambda capture + static method fixes |
| `modules/LVGLDisplay/` | WO1, WO4, WO6, WO7 | Static call fix, boot title, screensaver policy, EventBus race |
| `modules/DataManager/` | WO2, WO8 | TWDT unsubscribe around fetch, IDataSource contract, queueDataJob primitive |
| `modules/WebServer/` | WO3, WO4 | Stack size, sockets, `/api/app` stylesheet |
| `modules/Application/` | WO4 | `setAppStylesheet()` API |
| `modules/EventBus/` | WO5, WO7 | Publisher-side payload lifetime doc (WO5), threading rule doc (WO7) |
| `web/portal/` | WO3, WO4, WO6 | HWM logging, FOUC fix in app.js, screensaver toggle |
| `.agents/skills/framework-expert/` | WO5 | LV_CONF_PATH consumer warning, PSRAM audit checklist |
| `squareline/crowpanel-128/` | WO5 | SLS naming consistency |
| `sample-app/` | WO5 | Native test link fix |

## Session Log

### Session 1 — 2026-08-19 (Triage & Decomposition)
- Reviewed all 15 open GitHub issues.
- Triaged into 11 actionable + 4 parked (#55 roadmap RFC, #103/#104/#105 release-ops chain).
- Recommended #102 (beta OTA) remain open as a standalone investigation.
- Decomposed 11 issues into 6 sequential Work Orders, serialised to avoid file conflicts.
- Prior art: `pr-p2-issue-remediation` project closed #91/#90/#84 in 4 WOs with 271 new tests.
- Pruned 8 stale local branches from graduated projects.
- Working tree clean on `main` at `446a1724`, tag `v0.0.12-dev.52`.

### Session 2 — 2026-08-19/20 (Execution — WO1 through WO8)
**Summary**: All 8 Work Orders executed serially by spawned workers (Tier 3). 14 issues addressed across 12 modules. 748/748 native tests on main framework, 11 new tests added.

**Cross-cutting decisions**:
- WO2 worker took an "alternative approach" to the TWDT fix — post-fetch `esp_task_wdt_reset()` + `_fetchStartMs` instrumentation only, dropping the `esp_timer` feeder. This turned out to be insufficient (see Session 3).
- WO7 (`_onOtaProgress` EventBus race) established the project-wide pattern for LVGL thread safety: set `std::atomic<bool>` in EventBus callback, act on it in `tick()`. This pattern is now documented in `EventBus.hpp`.
- WO8 (`queueDataJob`) used `ulTaskNotifyTake` instead of `vTaskDelay` to allow immediate job wake-up — a material improvement over polling.
- Mid-flight additions: Issues #130 (EventBus payload lifetime), #131 (LVGL data race), #132 (queueDataJob design) were triaged and added as WO5/WO7/WO8 during execution.

**Surfaced findings**:
- Worker-model instruction drift: Flash/Pro-Low tier workers consistently ignored MCP tooling rules and fell back to raw `pio test` shell commands. PA had to intervene to verify builds.
- WO1 required 4 spawn attempts due to 429/503 cloud capacity limits.
- `sample-app/platformio.ini` used `test_filter` with negative syntax (`-test_unit/test_weather`) which silently skipped all tests — not caught until PR review.

**Dead ends (project-wide)**:
- WO1: The `_pauseRendering.load()` OTA predicate swap (replacing `OTAUpdateManager::getState() != Idle`) is semantically narrower. A proper fix requires wiring an `OTAUpdateManager*` member into the display module — deferred as out of scope.
- WO2: The worker's "simpler approach" (no timer feeder) did NOT fix #128 for bounded hangs. This was caught by the downstream consumer review and fully remediated in Session 3.

### Session 3 — 2026-08-20 (PR #136 Review Remediation)
**Summary**: Downstream consumer review (DraytonWiserController perspective) identified 20 findings across 3 severity tiers: 1 blocker, 7 bugs, 5 design issues, 8 minor/house-style. All 20 addressed in commit `b961bb1c` (+802/−69, 14 files). 759/759 tests after remediation (11 net-new tests, including 2 from this session).

**Blocker resolution (TWDT #128)**:
The review correctly identified that the WO2 fix was incomplete — bounded fetches (e.g., 5s TLS handshake against `CONFIG_ESP_TASK_WDT_TIMEOUT_S=5`) still triggered TWDT abort. Fix: added `esp_timer`-based fetch watchdog. Worker is temporarily unsubscribed from TWDT during fetch/job; periodic timer (2s) monitors elapsed time and calls `abort()` at 30s. Also updated `IDataSource.hpp` contract from "aim to return within 30 seconds" to "MUST set client timeouts below 30s; 30s hard deadline enforced by framework."

**Key design fixes**:
- `drainJobs()` rewritten: scan past non-urgent entries to find urgent ones (fixes head-of-line blocking), bounded drain via `uxQueueMessagesWaiting()` snapshot, deferred re-queue eliminates `xQueueSendToFront` leak risk.
- `volatile uint32_t _fetchStartMs` → `std::atomic<uint32_t>` (volatile is not a C++ concurrency primitive).
- Destructor reordered: task → timer → queue → mutex (was use-after-free if destructor ran while worker was inside `drainJobs()`).
- Portal FOUC: stylesheet `<link>` now awaits `onload` with 3s timeout; boot sequence changed to `Promise.all` for parallel fetch.

**Dead ends**:
- Considered two-queue approach (separate urgent + normal queues) for F5 — rejected as over-engineering; deferred buffer scan is simpler and sufficient for queue depth ≤8.
- F6 (OTA predicate narrowing): confirmed that a proper fix requires adding an `OTAUpdateManager*` member to `LVGLDisplayModule` and wiring it through `Application::begin()` — left as a documented TODO with inline comment + U8G2 cross-reference.

### Project Wrap-Up — 2026-08-25

**Architectural outcome**: The August issue sweep addressed 14 issues across the framework, with the most significant additions being: (1) the `queueDataJob()` primitive for bounded off-loop writes on the DataManager worker task, (2) the TWDT fetch watchdog timer providing a 30s hard deadline for consumer fetches without boot-looping, and (3) the EventBus → LVGL thread-safety pattern (atomic flag in callback, act in `tick()`). PR #136 merged as `6890dc22`.

**Lessons for future projects**:
1. **Worker-generated TWDT fixes need explicit test coverage.** The WO2 worker's "simpler approach" passed CI but didn't actually fix the reported bug. The missing test (no assertion on watchdog behaviour during a blocking fetch) allowed this to ship. Always require a test that exercises the specific failure mode the fix targets.
2. **PlatformIO `test_filter` negative syntax doesn't work.** Use `test_ignore` instead. This silently skipped all sample-app tests and wasn't caught until the downstream review.
3. **`volatile` is not `std::atomic`.** Workers trained on older C/embedded patterns will reach for `volatile` for cross-task variables. The framework convention is `std::atomic` for all shared state — enforce this in the PSRAM audit checklist.
4. **Lower-tier worker models ignore MCP tooling rules.** Flash and Pro-Low models fall back to shell commands (`pio test`) despite explicit rules. PA must independently verify builds/tests rather than trusting worker completion reports at face value.
5. **Downstream consumer reviews catch what unit tests miss.** The DraytonWiserController perspective found the TWDT blocker, the urgent queue ordering bug, and the IDataSource contract contradiction — all of which passed 748 tests. Integration-perspective reviews are essential for framework PRs.

**Unresolved items**:
- #80 (portal FOUC) deliberately left open — reduced but not fully eliminated without server-side CSS embedding.
- #97 (SLS project naming) deliberately left open — naming fixed but broader SLS tooling consistency is ongoing.
- F6: `_pauseRendering` OTA predicate narrowing documented but not fully resolved — needs `OTAUpdateManager*` wired into display module.
- F14: Designated initializers (`DataJobOptions{ .tag = ... }`) compile as GCC/Clang extension under `-std=gnu++17` but would fail under `-Wpedantic` — acceptable for the framework but noted for consumer apps with stricter flags.
