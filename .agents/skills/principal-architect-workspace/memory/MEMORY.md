# Principal Architect Memory
<!-- Last updated: 2026-09-22T21:15:00+01:00 -->

## Active Programs
| Program | Plan | Status |
|---|---|---|
| `esp32-app-foundation` | [PROGRAM-PLAN.md](../esp32-app-foundation/PROGRAM-PLAN.md) | PR-01 through PR-40 delivered |

## Active Projects
| Project | Status | Modules | Context |
|---|---|---|---|
| pr-225-ble-manager | WO6/7 — **BLOCKED** on BLE WDT crash during `NimBLEDevice::init()`. WO-0–5 ✅, all thrash reverted. Non-BLE baseline stable at `v0.0.12-dev.60`. 4 hypotheses documented, systematic debugging required. | BLEManager (new), platformio.ini, sdkconfig fragment, sample-app, sync_sdkconfig.py, docs, framework-expert skill | [PROJECT.md](../pr-225-ble-manager/PROJECT.md) |
| sample-app-remediation | NEW — weather station sample app needs review: boot sequence, dependency mapping, config flow, DataManager usage vs post-Drayton framework APIs | sample-app/src/main.cpp, WeatherModule, DataManager, ConfigManager, web portal | (pending PROJECT.md creation) |
| pr-drayton-consumer-fixes | WO5/5 — COMPLETE, soak-tested | DataManager, IDataSource, FetchContext, AppFoundation.h, WeatherModule, framework-expert skill | [PROJECT.md](../pr-drayton-consumer-fixes/PROJECT.md) |
| pr-128-datamanager-resilience | WO7/7 (WO6 re-plan gated) | DataManager, IDataSource, FetchContext, Application, MCPServer, Display, Portal | [PROJECT.md](../pr-128-datamanager-resilience/PROJECT.md) |

## Recently Completed (last 5)
| Project | Completed | Modules | Key Outcome |
|---|---|---|---|
| pr-spec-v2 | 2026-09-07 | docs/design_specification.md, include/interfaces/, include/AppFoundation.h, modules/MCPServer/, CHANGELOG.md, scripts/, docs/developer/, docs/archive/, .agents/skills/framework-expert/ | Design specification rebuilt v1.1→v2.0 against delivered architecture. All 18 sections rewritten. 6 WOs, 6 workers, 14 commits. `_mcpReadOnly` P1 fix (#214), `ILVGLHardwareAdapter` relocation (ADR), CHANGELOG.md (first in repo), doc-rot lint gate (`scripts/doc_lint.py`), NATIVE_BUILD→ESP_PLATFORM sweep (45+ sites), 3 design docs archived, 15 new decisions (D40–D54). PR [#224](https://github.com/matthewmcneill/esp32-app-foundation/pull/224) staged. [PROJECT.md](../pr-spec-v2/PROJECT.md) |
| pr-212-remediation | 2026-09-02 | Redactor, LogPipeline, StructuralScrubber, ISanitizer, CompositeSanitizer, AuditModule, KeyManager, WifiManager | Chief Architect review remediation of PR #212. 6 blockers fixed (append-only registry restored, sliding-window dumpPayload, heap under lock, fail-closed defaults, full native test coverage). 888 tests, 49 files. PR [#212](https://github.com/matthewmcneill/esp32-app-foundation/pull/212) merged. [PROJECT.md](../pr-212-remediation/PROJECT.md) |
| pr-211-logging | 2026-09-02 | Redactor, LogPipeline (new), ISanitizer (new), Logger (delete), Application, AuditModule, DeviceCrypto, WifiManager, KeyManager, docs, framework-expert skill | Logging subsystem unification: global secret redaction via esp_log_set_vprintf() hook. Replaced decorative Logger wrapper bypassed by 538/545 sites. 4 WOs + 5 WO remediation. Closes #211. PR [#212](https://github.com/matthewmcneill/esp32-app-foundation/pull/212) merged. [PROJECT.md](../pr-211-logging/PROJECT.md) |
| pr-sls-to-lvglpro | 2026-09-01 | LVGLDisplay, DiagCarouselController, uidesign, Logger, docs, framework-expert skill | SLS→LVGL Pro migration: 23 `sys_*` subjects replace `ILVGLSystemScreens` shim, design-folder-as-PIO-library, 4 WOs, hardware soak-tested (1h48m, heap flat), SLS archived under `sls-archive` tag. Logger `Arduino.h` dep removed. Logger consolidation surfaced as P1 (#211). PR [#208](https://github.com/matthewmcneill/esp32-app-foundation/pull/208) merged. [PROJECT.md](../pr-sls-to-lvglpro/PROJECT.md) |
| pr-194-audit-module | 2026-08-30 | lib/Redactor, lib/Logger, AuditModule, board.h, MCPBuiltinTools, ConfigManager, Audit.h, framework-expert skill | Generalized flash-persisted AuditModule with two-tier API (AUDIT() macro + direct API sentinel), JSONL rotation, 4 framework interfaces, `get_logs`→`get_debug_logs` rename, factory reset clears audit. Post-merge: comprehensive doc audit (thread safety, limitations, cross-refs), FR bug fix. 38+ files, 872 tests. PR [#203](https://github.com/matthewmcneill/esp32-app-foundation/pull/203) merged. [PROJECT.md](../pr-194-audit-module/PROJECT.md) |

## Older Projects
96 completed projects.
Full project history: [PROJECT_INDEX.md](PROJECT_INDEX.md)
Cross-cutting module index: [MODULE_INDEX.md](MODULE_INDEX.md)

