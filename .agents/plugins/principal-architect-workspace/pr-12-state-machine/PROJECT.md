# PR-12: State Machine Redesign — Boot → Configure ↔ Running

## Objective

Simplify the application lifecycle state machine from 4 monotonic states (`BOOTING → APP_SETUP → CONFIGURING → RUNNING`) to 3 bidirectional states (`BOOT → CONFIGURING ↔ RUNNING`). Enables runtime state regression with application-configurable tolerance, adds a ConfiguringStatusBoard for real-time gate progress UX, and simplifies DataManager gating.

## Architectural Context

**Analysis session:** Conversation `7c085adb-b44a-4f27-9b7b-f46345330ef8`
**Root cause analysis:** 9 tangles identified via systematic debugging + embedded systems review.
**User design decisions:** Per-gate regression tolerance, simple DataManager gating (all fetches deferred until RUNNING), ConfiguringStatusBoard as framework system board.

### Key Decisions

| Decision | Resolution |
|----------|------------|
| State count | 3: `BOOT`, `CONFIGURING`, `RUNNING` (remove `BOOTING` + `APP_SETUP`) |
| State regression | Bidirectional `CONFIGURING ↔ RUNNING` with per-gate tolerance |
| Regression tolerance | Each `IConfigGate` declares `getRegressionToleranceMs()`. Default 30s. `UINT32_MAX` = never regress. |
| Data fetching | ALL fetches deferred until RUNNING. No `iDataSource` interface changes. Simple atomic flag in DataManager. |
| CONFIGURING UX | New `ConfiguringStatusBoard` shows dynamic gate progress with status icons. Yields to helper boards (WiFi wizard) when user action needed. |
| WiFi password | Blocking prerequisite — must be wired through KeyManager before state machine can be tested. |
| Gate dependencies | Deferred to fast-follow. Initial implementation uses flat gate ordering. |
| Dead code | Remove `ConfigManager::requestReload()`/`checkAndClearReload()` (zero callers). |

## Dependency Graph

```
WO0 (WiFi pwd) ──┐
                  ├──► WO1 (State Machine Core) ──┬──► WO2 (DataManager Gating)
WO4 (Cleanup) ───┘                                ├──► WO3 (ConfiguringStatusBoard)
                                                   └──► WO5 (Integration Tests)
```

**Parallelism advisory:**
- WO0 + WO4: Safe to parallelize (zero file overlap)
- WO0/WO4 → WO1: Serial dependency (WO1 depends on enum changes compiling against clean base)
- WO1 → WO2, WO3: WO2 and WO3 can parallelize after WO1 (no mutual file conflicts)
- WO5: Must follow all others (integration across all changes)

## Work Order Ledger

- **[ ] [wo0-wifi-password-persistence](wo0-wifi-password-persistence/WORKORDER.md):** Wire WiFi password storage through KeyManager. `serializeConfig()` excludes password → `updateWiFi()` stores via `KeyManager::addKey()`, `applyConfig()` retrieves via `KeyManager::getKeyById()`. Add `KeyManager*` DI to WifiManager. Blocking prerequisite for hardware testing.
- **[ ] [wo1-state-machine-core](wo1-state-machine-core/WORKORDER.md):** Collapse `BOOTING`+`APP_SETUP` into `BOOT`. Remove `APP_SETUP` from enum and all references. Add `getRegressionToleranceMs()` to `IConfigGate`. Add per-gate regression timer logic to `Application::tick()`. Add `isRegression` field to `AppConfiguringPayload`. Update `Application::begin()` to run entirely under `BOOT` state. Update `setState()` name switch.
- **[ ] [wo2-datamanager-gating](wo2-datamanager-gating/WORKORDER.md):** Add `std::atomic<bool> _appRunning` to DataManager. Add `setAppRunning(bool)` public method. Guard worker task loop with flag check. Wire `Application` to call `setAppRunning(true/false)` on RUNNING entry / CONFIGURING regression.
- **[ ] [wo3-configuring-status-board](wo3-configuring-status-board/WORKORDER.md):** New `ConfiguringStatusBoard` system board in U8g2DisplayModule. Dynamic gate list with status icons (✓/⟳/○). Progress bar. Wire into `app.configuring` / `app.running` events. Shows during CONFIGURING, yields to helper boards, auto-dismisses on RUNNING.
- **[ ] [wo4-dead-code-cleanup](wo4-dead-code-cleanup/WORKORDER.md):** Remove `ConfigManager::requestReload()`, `checkAndClearReload()`, `_reloadPending`. Remove associated test `RequestReload_SetsAndClearsFlag`. Update module header doc block.
- **[ ] [wo5-integration-tests](wo5-integration-tests/WORKORDER.md):** Update all `BOOTING` → `BOOT` assertions in `test_application.cpp`. Add regression tests: tolerance exceeded → CONFIGURING, re-satisfied within tolerance → stays RUNNING, UINT32_MAX → never regresses, regression event has `isRegression = true`. Verify clean native build across all test suites.
- **[ ] [wo6-gate-dependency-ordering](wo6-gate-dependency-ordering/WORKORDER.md):** Add `getDependencies()` to `IConfigGate`. Update `_publishConfiguringEvent()` to resolve first actionable gate. Update ConfiguringStatusBoard to render gates grouped by dependency state: satisfied / actionable / blocked.

## Branch State

| Branch | Base | Status |
|--------|------|--------|
| `pr-12-state-machine` | `pr-11-config-gateway` | Feature PR branch — not yet created |
| `pr-12-state-machine/wo0-wifi-pwd` | `pr-12-state-machine` | Planned |
| `pr-12-state-machine/wo1-core` | `pr-12-state-machine` | Planned |
| `pr-12-state-machine/wo2-datamanager` | `pr-12-state-machine` | Planned |
| `pr-12-state-machine/wo3-configuring-board` | `pr-12-state-machine` | Planned |
| `pr-12-state-machine/wo4-cleanup` | `pr-12-state-machine` | Planned |
| `pr-12-state-machine/wo5-tests` | `pr-12-state-machine` | Planned |
| `pr-12-state-machine/wo6-gate-deps` | `pr-12-state-machine` | Planned |

---

*Created by Principal Architect — 2026-05-31*
