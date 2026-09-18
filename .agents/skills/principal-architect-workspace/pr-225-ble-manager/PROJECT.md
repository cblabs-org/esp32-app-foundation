# BLEManager — Managed BLE Optional Module (RFC #225 rev 2)

**Project:** `pr-225-ble-manager`
**Base branch:** `main`
**Branch prefix:** `ble-manager/` (linear stack, see [Branch Plan](#branch-plan))
**Status:** 🔨 WO-3 IN PROGRESS (autonomous worker, `pro`). WO-2 complete 2026-09-18T22:31:11+01:00.
**Parent issue:** [#225](https://github.com/matthewmcneill/esp32-app-foundation/issues/225)
**Agreed design (source of truth):** [RFC #225 revision 2](https://github.com/matthewmcneill/esp32-app-foundation/issues/225#issuecomment-5729939169), plus the amendments in the [framework acknowledgement](https://github.com/matthewmcneill/esp32-app-foundation/issues/225#issuecomment-5735199501) (acceptance item 6 is split into 6a/6b/6c)
**Consumer:** Echenon ([SPEC.md](https://github.com/cblabs-org/echenon/blob/main/SPEC.md) §7–§8), labels `upstream-issue`, `found-by:echenon`

---

## Approval Record

> ✅ **Approved — all WOs (WO-0 through WO-6).**
>
> - **Approver's exact words:** "approved all WOs. Happy for you to do the spike here in-session and then delegate for the following."
> - **Timestamp:** 2026-09-18T21:02:01+01:00
> - **Scope:** All WOs (WO-0 through WO-6)
> - **Spawn mode:** WO-0 PA-driven (in-session); WO-1…WO-5 autonomous workers; WO-6 interactive bridge.
>
> Workers verify provenance against this section (`.agents/rules/00-operating-model.md`). This approval authorises execution of all WOs within their respective Strict Boundary Boxes.

---

## Architectural Shift (summary)

Add `modules/BLEManager/`, a **WITH_BLE-gated optional module** that owns the NimBLE stack for both roles:
- **Peripheral:** a shared GATT server, advertising aggregated from `IBLEServiceProvider`s and packed by a NimBLE-free `BLEAdvertisingBuilder`.
- **Central:** user-initiated discovery, a manager-owned stored identity with opportunistic bonding, direct-connect reconnect (fast backoff, then slow background retry), and declarative subscriptions delivered on the main loop to `IBLEClientProvider`s.

It also contributes a portal tab, REST routes, MCP tools, EventBus `ble.*` events, preconditions and config.

**No kernel changes.** Nothing changes in `Application`, `AppFoundation.h`, `include/interfaces/` or `registerModule()`. The module is wired explicitly in `main.cpp` like HomeAssistant/OTA. The interfaces are module-local, per the Interface Placement ADR.

**v1 targets ESP32-S3 only**, through a new `esp32s3nano_ble` env built from a `[ble]` mix-in plus the `hardware/_common/sdkconfig.ble.defaults` fragment. Non-BLE envs must be unchanged in behaviour, with ≈ 0 flash/RAM delta.

**Threading contract (non-negotiable):** every provider callback runs on the **main loop**, drained from a bounded FreeRTOS queue in `BLEManager::tick()` (the `ButtonModule` pattern). The only exceptions are `registerServices()` (at start) and the opt-in `onClientReadyWorker()`. `ble.*` events are published from the main loop using **ring-buffer payloads** (see [#227](https://github.com/matthewmcneill/esp32-app-foundation/issues/227)). Nothing publishes to EventBus from the NimBLE host task.

---

## Decisions

| # | Decision | Ruling |
|---|---|---|
| **D1** | Module kind | Optional module; explicit `main.cpp` composition; `app.registerModule(&bleManager)` covers its own framework interfaces only. (RFC §3, review A3) |
| **D2** | Interface placement | `IBLEServiceProvider`, `IBLEClientProvider` and `BLEAdvertisingBuilder` live in `modules/BLEManager/`. (ADR, review A1/A2) |
| **D3** | NimBLE library | **`h2zero/esp-nimble-cpp@^2.1.0`** — confirmed by WO-0 spike (2026-09-18). Clean build under dual `arduino, espidf` framework (IDF 5.5.5), no duplicate-symbol or duplicate-host errors. IDF-native NimBLE host, sdkconfig-driven. All RFC §5 APIs present (server/client/scan/advertise/bond/subscribe). Flash Δ = +251 KB (37.7% of 4 MB), RAM Δ = +2.1 KB. Controller budget: 6/6 activities (3P+1C+adv+scan). Candidate B (`NimBLE-Arduino@^2`) was not tested — A passed all gates and is the pre-agreed tie-break winner. See [`wo0-spike/FINDINGS.md`](wo0-spike/FINDINGS.md). |
| **D4** | Targets | v1 = ESP32-S3. Env `esp32s3nano_ble` in the **root** `platformio.ini` (compile/CI) **and** in `sample-app/platformio.ini` (proves the out-of-tree consumer path). `esp32dev` is out of scope. |
| **D5** | Framework-side validation | **Reference providers in `sample-app/src/ble/`**, under `WITH_BLE`: `SampleFtmsProvider` (synthetic indoor-bike data, so real FTMS apps can discover it), `SampleHrRepeater` (0x180D peripheral), `SampleHrClient` (0x180D central). Echenon validates its own providers in its own programme (its PR-07). |
| **D6** | Pure-logic isolation | `BLETypes`, `BLEAdvertisingBuilder`, `BLEDeviceRegistry`, `BLEReconnectPolicy` are NimBLE-free and unit-tested under `env:native`. **Only `BLEStack.*` includes NimBLE headers**, wrapped in `#if defined(ESP_PLATFORM) && defined(WITH_BLE)`. `BLEManager.cpp` has no NimBLE includes. |
| **D7** | Stack start trigger | `tick()` polls `Application::getState()` and starts on the main loop. The stack is **not** started from an EventBus callback: the esp_event task has a ~4 KB stack and the wrong thread context. `ota.progress` subscriptions only set an atomic flag, which `tick()` acts on. |
| **D8** | [#227](https://github.com/matthewmcneill/esp32-app-foundation/issues/227) dependency | WO-5 consumes `ota.progress`, which is currently a single-slot payload that can be torn. **WO-5 is gated on #227's OTA fix being on `main`**, or on the human explicitly choosing the fallback: BLEManager reacts only to the *first* `ota.progress` event (pause) and to `complete && !success` (resume), and copies fields immediately. The PA must raise this with the human at the WO-5 gate. |
| **D9** | [#226](https://github.com/matthewmcneill/esp32-app-foundation/issues/226) | Independent. Not a dependency of any WO. |
| **D10** | Docs and skills | Each WO documents its own surface in the module README. WO-5 writes the consolidated developer guide, framework-expert skill material and CHANGELOG. |
| **D11** | Portal disconnect-peer action | Deferred to v1.1 (RFC §19 item 7). |

---

## Work Order Ledger

| WO | Title | Complexity | Status | Branch | Commit |
|----|-------|-----------|--------|--------|--------|
| **WO-0** | [Build spike: library choice, sdkconfig fragment, budgets](wo0-spike/WORKORDER.md) | `standard` (build-dominated → PA-driven verification) | ✅ Complete | `ble-manager/wo0-spike` | `e499f010` |
| **WO-1** | [Module skeleton, types, pure logic, env plumbing, lifecycle](wo1-skeleton/WORKORDER.md) | `standard` | ✅ Complete | `ble-manager/wo1-skeleton` | `01be46a0` |
| **WO-2** | [Peripheral role: GATT registration, advertising packer, callbacks](wo2-peripheral/WORKORDER.md) | `standard` | ✅ Complete | `ble-manager/wo2-peripheral` | `1c048c72` |
| **WO-3** | [Central role: discovery, registry, bonding, reconnect, worker task](wo3-central/WORKORDER.md) | `architect` | ⬜ Not started | `ble-manager/wo3-central` | — |
| **WO-4** | [Portal tab, REST routes, MCP tools](wo4-portal-mcp/WORKORDER.md) | `standard` | ⬜ Not started | `ble-manager/wo4-portal-mcp` | — |
| **WO-5** | [Coexistence, OTA pause, docs and skill](wo5-coexistence-ota/WORKORDER.md) | `standard` | ⬜ Not started (gated by D8) | `ble-manager/wo5-coexistence-ota` | — |
| **WO-6** | [Hardware acceptance run (RFC §17.2, amended)](wo6-validation/WORKORDER.md) | `interactive` | ⬜ Not started | `ble-manager/wo6-validation` | — |

### Gates between WOs
- **After WO-0 (hard gate):** the PA reviews `wo0-spike/FINDINGS.md`, records D3 and any sdkconfig changes here, and **presents the budget numbers to the human**. If the S3 budget or the controller activity budget can't meet 3P+1C, **stop** and return to the RFC. Don't continue on assumptions.
- **Before WO-3 hardware checks:** the consumer's §19 item 1 (Beat address type / SMP support) is *useful* but not blocking. Opportunistic bonding handles either answer.
- **Before WO-5:** D8 (#227).
- **WO-6** is `interactive`. The PA issues a bridge prompt to the human (debug-handoff protocol) instead of spawning a worker.

---

## Branch Plan

Linear stack, bases expressed as **branch names** (STACKING_REFERENCE §1.2):

```
main
 └─ ble-manager/wo0-spike
     └─ ble-manager/wo1-skeleton
         └─ ble-manager/wo2-peripheral
             └─ ble-manager/wo3-central
                 └─ ble-manager/wo4-portal-mcp
                     └─ ble-manager/wo5-coexistence-ota
                         └─ ble-manager/wo6-validation   ← PR head
```

**File overlap:** WO-2 through WO-5 all touch `modules/BLEManager/BLEManager.{hpp,cpp}` and `BLEStack.{hpp,cpp}`. **They must be serialized (Option A), not run in parallel.** WO-4 (portal/MCP) touches the fewest shared files. It *could* branch from WO-3 in parallel with WO-5 only if the human accepts manual conflict resolution in `BLEManager.cpp`. The default is serial.

---

## Modules & Components

| Module/Area | WOs | Notes |
|---|---|---|
| `modules/BLEManager/` (new) | WO-1…WO-5 | The module; its layout is fixed by RFC §2 |
| `hardware/_common/sdkconfig.ble.defaults` (new) | WO-0, WO-1 | BT/NimBLE enable fragment |
| `platformio.ini` (root) | WO-0, WO-1 | `[ble]` mix-in, `esp32s3nano_ble` env; native env exclusion |
| `sample-app/platformio.ini`, `sample-app/src/ble/`, `sample-app/src/main.cpp` | WO-1…WO-3 | Out-of-tree consumer env + reference providers (D5) |
| `src/main.cpp` (root in-tree app) | WO-1 | `#ifdef WITH_BLE` wiring so the module compiles in-tree |
| `scripts/sync_sdkconfig.py` | WO-1 | Watch the BLE fragment; extend the supported-env docstring |
| `test/test_unit/test_bleManager/` (new) | WO-1…WO-3 | Native tests for the pure-logic classes |
| `docs/developer/bluetooth.md` (new), `using-optional-modules.md`, `event-bus.md` (reference only) | WO-5 | Developer guide |
| `.agents/skills/framework-expert/systems/ble/` (new), `SKILL.md` triggers | WO-5 | Skill material; repo-owned, not a submodule |
| `CHANGELOG.md` | WO-5 | Consumer notice |

---

## Framework Facts Workers Need (verified 2026-09-18 against `main` @ `69d9c813`)

- **Registration:** `Application::registerModule<T>()` (`modules/Application/Application.hpp:252`) dispatches `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IDataSource`, `IStatusReporter`, `IBootReporter`, `IPreconditionContributor`. Vectors lock at the end of `app.begin()`.
- **Interfaces:** `ITickable::tick(uint32_t)` (10 ms default budget, ADR-17); `IConfigurable` (`getConfigNamespace/getSchemaVersion/applyConfig/serializeConfig/migrateConfig/resetConfig`); `IWebContributor` (`registerRoutes(PsychicHttpServer&)`, `registerTabs(ITabRegistry&)`, `getApiPrefix()`, `getMaxPostBody()`); `IMCPContributor::registerMCPTools(MCPToolRegistry&)` with `addTool(name, desc, [schema,] handler, ctx, mutatesState)`; `IPreconditionContributor::registerPreconditions(IPreconditionRegistry&)` with `add(name, role, evalFn, describeFn, edges, count, damping, retry)` and `Cause`/`Severity`/`PreconditionRole` in `include/interfaces/IPrecondition.hpp`.
- **EventBus** (`modules/EventBus/EventBus.hpp`): `subscribe(topic, cb, ctx)` and `publish(topic, payload)`. **On target, publish posts the payload POINTER asynchronously and blocks (`portMAX_DELAY`) if the queue is full.** Use the ring-buffer pattern (`ButtonModule.cpp:~300–315`). Never publish from the NimBLE host task.
- **App lifecycle:** `AppState::{Boot, Configuring, Running}` (`include/types/AppState.hpp`). `ApplicationEvents::OTA_PROGRESS` carries `OtaProgressPayload{progress, status, complete, success}` under `WITH_OTA`.
- **Optional-module pattern:** `docs/developer/using-optional-modules.md`, HomeAssistant (`modules/HomeAssistant/`: `library.json`, `web/`) and OTA (`otaManager.begin(app)` after `app.begin()` in `sample-app/src/main.cpp`).
- **Portal assets:** `modules/<Name>/web/<name>.js` is auto-discovered by `scripts/portalBuilder.py` into `include/portal_assets/<name>_js.hpp` and served with `include/utils/serveGzipAsset.hpp`. Tab lifecycle: `docs/developer/extending-the-web-portal.md`; design system: `framework-design-system` skill.
- **sdkconfig:** `sdkconfig.defaults:26` sets `CONFIG_BT_ENABLED=n` (it must stay that way for non-BLE envs). The S3 env layers `SDKCONFIG_DEFAULTS` through `board_build.cmake_extra_args`.
- **Build tooling:** use the `platformio` MCP (`build_project`, `run_tests`, `check_task_status`, `get_lock_status`) per `.agents/rules/04-mcp-tooling.md`. First ESP-IDF builds take 2–8 min. Judge success by clean link + `firmware.bin`; ignore the known `pre_build_test` Playwright false-`[FAILED]`.
- **House style:** module/file headers, method docs and inline comments follow the `house-style-docs` skill.

---

## Session Log

### 2026-09-18 — Project scoped (drafting PA session)
**Summary:** RFC #225 went through a critical review (A–F findings, G1–G17 consumer questions), two consumer answer rounds and revision 2. The consumer assessed rev 2 as approvable, and the framework acknowledgement marked it the agreed shape (issue body now carries a "superseded" banner pointing at rev 2). This PROJECT.md and WO-0…WO-6 were drafted from rev 2 §18 with the decisions above.
**Related issues filed during review:** #226 (P1, OTA `checkOnBoot`/deferral; Echenon runs `autoCheck=false` meanwhile), #227 (P2, EventBus payload-lifetime contract wrong on target; single-slot `OTA_PROGRESS`/`DATA_SOURCE_HEALTH` publishers race → D8).
**Consumer items outstanding (non-blocking):** Beat strap address type / SMP support (RFC §19 item 1), Appearance value for Indoor Bike (item 5; the sample FTMS provider uses its own value, so this doesn't block framework WOs).
**Next:** the implementing PA obtains and records human approval, then starts WO-0.

### 2026-09-18 — WO-0 complete (PA-driven build spike)
**Summary:** Human approved all WOs (verbatim: "approved all WOs. Happy for you to do the spike here in-session and then delegate for the following."). WO-0 executed PA-driven on branch `ble-manager/wo0-spike`. Created `hardware/_common/sdkconfig.ble.defaults` (RFC §16.2), `[ble]` mix-in and `[env:esp32s3nano_ble]` in root `platformio.ini`. Built baseline (`esp32s3nano`) and spike (`esp32s3nano_ble` with full API harness). Candidate A (`h2zero/esp-nimble-cpp@^2.1.0`) passed all gates — clean build, no duplicate symbols, IDF-native host, all RFC §5 APIs present. D3 recorded. Controller activity budget: 6/6 exact fit (3P+1C+adv+scan). Flash Δ = +251 KB (37.7%), RAM Δ = +2.1 KB. Candidate B not tested (A won). Spike harness committed then removed in separate commit. `FINDINGS.md` written. 5 esp-nimble-cpp v2 API differences documented (all straightforward).
**Commits:** `0f666cb5` (approval), `2b02db51` (spike harness), `e499f010` (harness removal).
**Next:** Present budgets to human for confirmation. If approved, create `ble-manager/wo1-skeleton` and spawn WO-1 autonomous worker.
