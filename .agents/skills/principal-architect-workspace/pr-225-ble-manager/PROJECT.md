# BLEManager — Managed BLE Optional Module (RFC #225 rev 2)

**Project:** `pr-225-ble-manager`
**Base branch:** `main`
**Branch prefix:** `ble-manager/` (linear stack, see [Branch Plan](#branch-plan))
**Status:** 🎯 WO-6 READY (interactive hardware acceptance). WO-0–5 complete.
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

> ✅ **Approved: WO-8 and WO-9 (PR #235 third-round review fixes).**
>
> - **Approver's exact words:** "Wait - I just want you to fix the current PR, don;t start a new one!" This was given in response to the PA's proposed WO-8…WO-11 decomposition, and narrows it to the PR #235 fixes only (WO-8, WO-9). No new PR; the WO-7 branch is not touched.
> - **Timestamp:** 2026-09-25
> - **Scope:** WO-8 (central client lifecycle and discovery) and WO-9 (manager state safety and persistence). Both are stacked on `ble-manager/wo6-validation` (the PR #235 head). After review, the PA fast-forwards `ble-manager/wo6-validation` to the WO-9 tip and pushes.
> - **Spawn mode:** autonomous workers (`standard` → sonnet), serialized (both touch `BLEManager.cpp`).
> - **Source of findings:** Chief Architect third-round review of PR #235 at `665b847`.
>
> ✅ **Approved: WO-10 (fix issue #236 in this PR).**
> - **Approver's exact words:** "Fix it fully now. and close #236 out". This was the answer to the PA's question about the WifiManager STA timeout / #236.
> - **Timestamp:** 2026-09-25
> - **Scope:** WO-10, WifiManager STA resilience and no unauthenticated fallback AP. Branch `ble-manager/wo10-wifi-sta-resilience`, run in parallel with WO-8/WO-9 (disjoint files), then rebased onto the WO-9 tip before the PR head fast-forward. Spawn mode: autonomous (`architect` → opus).
> - **Recorded for later (not in this PR):** the TX-power answer "Configurable, default +20" applies to the WO-7 branch (`BLEConfig.txPowerDbm`, default +20 dBm, documented regulatory responsibility).
>
> Workers verify provenance against this section (`.agents/rules/00-operating-model.md`). This approval authorises execution of WO-8 and WO-9 within their Strict Boundary Boxes.

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
| **D8** | [#227](https://github.com/matthewmcneill/esp32-app-foundation/issues/227) dependency | **DECIDED 2026-09-19:** Human chose **fallback** — react to first `ota.progress` only (pause), `complete && !success` (resume), copy fields immediately. No progress-percentage tracking. Code markers: `// FALLBACK(#227)`. Detailed follow-up comment posted to [#227](https://github.com/matthewmcneill/esp32-app-foundation/issues/227#issuecomment-5739961669) specifying exact files and changes needed once the fix lands. |
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
| **WO-3** | [Central role: discovery, registry, bonding, reconnect, worker task](wo3-central/WORKORDER.md) | `architect` | ✅ Complete | `ble-manager/wo3-central` | `539488c7` |
| **WO-4** | [Portal tab, REST routes, MCP tools](wo4-portal-mcp/WORKORDER.md) | `standard` | ✅ Complete | `ble-manager/wo4-portal-mcp` | `c133d59b` |
| **WO-5** | [Coexistence, OTA pause, docs and skill](wo5-coexistence-ota/WORKORDER.md) | `standard` | ✅ Complete (D8 fallback) | `ble-manager/wo5-coexistence-ota` | `b568ec5f` |
| **WO-6** | [Hardware acceptance run (RFC §17.2, amended)](wo6-validation/WORKORDER.md) | `interactive` | ✅ Complete | `ble-manager/wo6-validation` | `18618a43` (ACCEPTANCE.md), fix `c697ff55` (DIS MAC). Tag `v0.0.12-dev.67`. |
| **WO-8** | [PR #235 R3: central client lifecycle and discovery](wo8-central-lifecycle/WORKORDER.md) | `standard` | ✅ Complete (`e206b12f`) | `ble-manager/wo8-central-lifecycle` (base `ble-manager/wo6-validation`) | — |
| **WO-10** | [Fix #236: WifiManager STA resilience](wo10-wifi-sta-resilience/WORKORDER.md) | `architect` | ✅ Complete (`295f2c7a`) | `ble-manager/wo10-wifi-sta-resilience` | — |
| **WO-9** | [PR #235 R3: manager state safety and persistence](wo9-state-safety/WORKORDER.md) | `standard` | ✅ Complete (`9d833913`) | `ble-manager/wo9-state-safety` (base `ble-manager/wo8-central-lifecycle`) | — |

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

### 2026-09-19 — WO-1 through WO-5 complete (autonomous workers)
**Summary:** WO-1 through WO-5 were executed by autonomous workers in sequence per the stacked branch plan. All completed successfully.
- **WO-1** (`01be46a0`): Module skeleton, BLETypes, BLEAdvertisingBuilder, env plumbing, lifecycle. Branch `ble-manager/wo1-skeleton`.
- **WO-2** (`1c048c72`): Peripheral role — GATT server, handle routing, advertising packer, NimBLE callbacks, sample providers (`SampleFtmsProvider`, `SampleHrRepeater`). Branch `ble-manager/wo2-peripheral`.
- **WO-3** (`539488c7`): Central role — discovery, BLEDeviceRegistry, bonding, BLEReconnectPolicy, worker task, `SampleHrClient`. Branch `ble-manager/wo3-central`.
- **WO-4** (`c133d59b`): Portal tab (`ble.js`), REST routes, MCP tools (`ble_status`, `ble_scan`, `ble_pair`, `ble_forget`). D8 gate: human chose fallback (option 2) for #227 EventBus payload lifetime. Branch `ble-manager/wo4-portal-mcp`.
- **WO-5** (`b568ec5f`): OTA pause/resume (D8 fallback pattern with `// FALLBACK(#227)` markers), WiFi/BLE coexistence hardening, `ScanInhibit`, consolidated docs (`docs/developer/bluetooth.md`), framework-expert skill BLE material, CHANGELOG entry. Branch `ble-manager/wo5-coexistence-ota`. Tagged `v0.0.12-dev.58`.
**Cross-cutting decisions:** D8 fallback chosen and posted to [#227](https://github.com/matthewmcneill/esp32-app-foundation/issues/227#issuecomment-5739961669). Template app issue filed as [#229](https://github.com/matthewmcneill/esp32-app-foundation/issues/229).
**Build numbers (post WO-5):** `esp32s3nano_ble` — Flash 1,761,971 B (52.5%), RAM 62,180 B (18.9%).

### 2026-09-22 — WO-6 interactive session: BLE WDT crash blocker + baseline recovery
**Summary:** Attempted WO-6 interactive hardware acceptance. **BLE init crashes on boot — BLOCKER.** 7 fix attempts all failed, systematic debugging invoked, all thrash reverted, non-BLE baseline confirmed stable at `v0.0.12-dev.60`. WO-6 acceptance protocol did NOT execute.

**BLE WDT Crash — Root Cause Analysis (CRITICAL — do not retry these):**
The BLE controller's `NimBLEDevice::init()` on ESP32-S3 acquires cross-core heap spinlocks that disable interrupts on BOTH CPUs for >5 seconds, causing watchdog resets. The controller also dumps 80+ lines of debug output (`EM_BASE_REG_LUT` table) during init, which is not normal production behaviour.

**Failed fix attempts (DO NOT RETRY):**

| # | Fix | WDT Type | Why it failed |
|---|-----|----------|---------------|
| 1 | `vTaskDelay(10ms)` before init | INT_WDT | Contention is DURING init, not before |
| 2 | `vTaskDelay(100ms)` + `advertiseInConfiguring=true` | INT_WDT | Stale config overrides default, still blocks |
| 3 | One-shot task on CPU1 (semaphore) | INT_WDT | Cross-core spinlock blocks CPU0 regardless |
| 4 | `CONFIG_ESP_INT_WDT_TIMEOUT_MS=800` | INT_WDT | Interrupts masked at level 11 — no timeout helps |
| 5 | `CONFIG_ESP_INT_WDT_TIMEOUT_MS=2000` | INT_WDT | Same |
| 6 | `CONFIG_ESP_INT_WDT_CHECK_CPU1=n` | INT_WDT | CPU0 still hit by cross-core spinlock |
| 7 | `CONFIG_ESP_INT_WDT=n` (disable entirely) | **TG0WDT** | Task WDT fires instead (5s default, init >5s) |

**Systematic debugging hypotheses (ranked, untested):**
- **H1:** BLE controller debug logging enabled — `EM_BASE_REG_LUT` dump shouldn't appear; check for `CONFIG_BT_LE_CONTROLLER_LOG_*` or similar
- **H2:** `CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL=y` (PSRAM) causing slow init — controller needs internal DRAM
- **H3:** `esp-nimble-cpp@2.5.0` vs IDF 5.5.5 compatibility issue
- **H4:** Task WDT needs deregistration during init (`esp_task_wdt_delete/add` around semaphore wait) + INT_WDT=n combined

**Key technical facts for next session:**
- NimBLE version: `esp-nimble-cpp@2.5.0` (h2zero)
- Task WDT config: `CONFIG_ESP_TASK_WDT_TIMEOUT_S=5`, `CONFIG_ESP_TASK_WDT_PANIC=1`
- `CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL=1` (PSRAM alloc)
- `CONFIG_BT_NIMBLE_LOG_LEVEL=1` (INFO)
- `CONFIG_BT_CTRL_MODE_EFF=1`
- Crash timeline: BLE_INIT starts ~7900ms, MAC printed, EM_BASE_REG_LUT dump (51 entries), reset at ~13000ms
- Waveshare board has 1 button (BOOT/GPIO0). Download mode: unplug USB, hold button, plug in, wait 2s, release
- Port: `/dev/cu.usbmodem101`

**Other work completed this session:**
- **PlatformIO MCP `extends` bug fix:** Root cause found in `parseTargetEnvironments()` — doesn't resolve `extends` directive. Fixed in `platformio-mcp` fork, 3 tests added, upstream PR [jl-codes/platformio-mcp#28](https://github.com/jl-codes/platformio-mcp/pull/28) submitted. Issue [#27](https://github.com/jl-codes/platformio-mcp/issues/27) filed.
- **WeatherModule WiFi guard:** Committed `3b20a443` — `fetchUpdate()` now checks WiFi connectivity via EventBus before attempting HTTP.
- **LittleFS re-uploaded** (fresh filesystem after factory flash).
- **Non-BLE baseline confirmed:** `esp32s3nano` env, `v0.0.12-dev.60`, 10-minute soak test PASS, zero crashes. Weather config saved via portal confirmed.

**All BLE thrash changes REVERTED.** Working tree is clean except `MEMORY.md` status update. The code on `ble-manager/wo6-validation` is identical to `ble-manager/wo5-coexistence-ota` plus the WeatherModule WiFi guard commit.

**Sample app remediation needed:** The weather station sample app is not a canonical best-in-class consumer of the framework. Boot sequence, dependency mapping, configuration flow, and DataManager usage need review against the post-Drayton-Wiser framework APIs. This should be a separate work stream before BLE acceptance.

**Next:** (1) Fix BLE WDT crash using systematic debugging (start with H1/H2). (2) Remediate sample app. (3) Then retry WO-6 acceptance protocol.

### 2026-09-23 — BLE crash resolved, DRAM optimisation, DIS implementation approved
**Summary:** BLE WDT crash root-caused (H-G: Arduino `initArduino()` releases BT controller memory when `bleInUse()` returns false), fixed with strong symbol override. DRAM optimised via sdkconfig right-sizing (+27 KB recovered). Full DRAM audit completed. BLE library alternatives researched (NimBLE confirmed as optimal). LOW_SPEED_MODE and RUN_IN_FLASH_ONLY investigated and rejected. Built-in DIS service designed and approved.

**Commits:** `v0.0.12-dev.64` (crash fix), `v0.0.12-dev.65` (DRAM optimisation), `4b263a7a` (docs gap closure).

**Key decisions:**
- D9: NimBLE is the optimal BLE library for ESP32-S3. Bluedroid costs ~40 KB more, BTstack has GPL licensing issues, Zephyr is infeasible.
- D10: `CONFIG_BT_NIMBLE_LOW_SPEED_MODE` saves <1 KB — not useful. `CONFIG_BT_CTRL_RUN_IN_FLASH_ONLY` saves ~25 KB but is EXPERIMENTAL and causes BLE failures during flash erase — not suitable as framework default.
- D11: Built-in DIS (0x180A) approved with field mapping: Manufacturer=app author (`setManufacturer()`, default "CB Labs"), Model=`getAppName()`, Serial=MAC, FirmwareRev=`BUILD_VERSION`, HardwareRev=`ESP.getChipModel()`, SoftwareRev=`"App Foundation " + FRAMEWORK_VERSION`.
- D12: NimBLE built-in GATT services (HR, BAS, DIS, etc.) are demo cruft — framework implements DIS via `IBLEServiceProvider` for dynamic values.

**Issues filed:**
- [#230](https://github.com/matthewmcneill/esp32-app-foundation/issues/230): RFC — OTA memory headroom via temporary module unloading
- [#232](https://github.com/matthewmcneill/esp32-app-foundation/issues/232): P2 — Device name inconsistency across WiFi AP, BLE, boot screen, portal, HA, TLS certs

> ✅ **DIS Work Order approved.**
> - **Approver's exact words:** "OK please write up the work order and proceed with implementation."
> - **Timestamp:** 2026-09-23T22:53:04+01:00
> - **Scope:** [wo6-dis-service/WORKORDER.md](wo6-dis-service/WORKORDER.md)

**WO6-DIS executed by worker `32b05cf5`:** DIS implementation delivered, hardware-validated at `v0.0.12-dev.66`. Worker Completion Report in [wo6-dis-service/WORKORDER.md](wo6-dis-service/WORKORDER.md). 933 unit tests pass. DRAM=70,739 B (negligible regression from 6 GATT entries). Serial confirmed `Device Information Service (0x180A) registered`.

**Additional work this session:**
- Credits screen (`web/portal/credits.js`) updated — 5 missing libraries added (esp-nimble-cpp, NimBLE, LVGL, LovyanGFX, JsonStreamingParser). Commit `a148b938`.
- Documentation gaps closed by worker `191242a3`: bluetooth.md §13 Memory & DRAM Budget, bleInUse() troubleshooting, SKILL.md BLE quick answers, DRAM preamble for memory profiles.
- Full device naming audit (17+ surfaces). Filed as [#232](https://github.com/matthewmcneill/esp32-app-foundation/issues/232) with extended findings (TLS cert CN bug, LVGL boot binding bug, mDNS unimplemented).

**Cross-cutting decisions (project-wide implications):**
- D13: `FRAMEWORK_VERSION` constant created (`include/framework_version.h`). Must be manually bumped on framework releases. Used by DIS Software Revision. Future: could be auto-injected by `inject_version.py`.
- D14: `Application::setManufacturer()`/`getManufacturer()` API added. Defaults to `"CB Labs"`. Third-party consumers override. This feeds into #232 identity unification.
- D15: Portal credits must list ALL third-party dependencies with correct licences. 12 libraries now credited.

**Dead ends (project-wide):**
- `CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL=y` (PSRAM for NimBLE): Causes spinlock panics (IDFGH-7560). BLE controller DMA physically cannot access PSRAM. This is a hardware constraint, not software — applies to ALL ESP32-S3 projects.
- `CONFIG_BT_CTRL_RUN_IN_FLASH_ONLY`: Saves ~28 KB IRAM but is EXPERIMENTAL, breaks during flash erase (OTA, LittleFS). Espressif's own CI disables it. Not viable as framework default.

**Soak test results:**
- `v0.0.12-dev.65`: 12/12 checks over 56m33s, DRAM dead flat at 73,527 B, zero crashes.
- `v0.0.12-dev.64`: 12/12 checks over 3h24m, stable.

### Session Wrap-Up — 2026-09-23T23:12:00+01:00

**Architectural outcome:** BLE module is now feature-complete for framework v0.0.12. The module provides:
- Dual-role BLE (Peripheral + Central) via `IBLEServiceProvider` / `IBLEClientProvider`
- Built-in Device Information Service (DIS 0x180A) with dynamic identity from `Application` APIs
- Optimised DRAM footprint (~69 KB, down from ~97 KB)
- Documented memory profiles (balanced, peripheral-only, gateway, ultra-minimal)
- Full developer documentation (`bluetooth.md`) and agent skill coverage

**Branch status:** `ble-manager/wo6-validation` — 8 commits ahead of `ble-manager/wo5-coexistence-ota`:

| Tag | Commit | Description |
|-----|--------|-------------|
| `v0.0.12-dev.62/63` | `4e047185` | WDT scaffolding (reverted in dev.64) |
| `v0.0.12-dev.64` | `d59ce16f` | BLE crash fix (bleInUse + WDT revert) |
| `v0.0.12-dev.65` | `f517e790` | DRAM optimisation |
| — | `0afff5ec` | Skill: BLE memory profiles |
| — | `4b263a7a` | Docs: memory, troubleshooting, skill gaps |
| `v0.0.12-dev.66` | `344ee6df` | DIS implementation |
| — | `162ebf30` | WO6-DIS completion report |
| — | `a148b938` | Credits: 5 missing libraries |
| `v0.0.12-dev.67` | `c697ff55` | fix(ble): DIS Serial → BLE MAC |
| — | `18618a43` | docs(wo6): hardware acceptance report |

**WO-6 complete.** Interactive hardware acceptance testing passed on `v0.0.12-dev.67`:
- DIS 6/6 characteristics verified via Mac BLE (bleak)
- BLE advertising, connect/disconnect cycling, DRAM stability all pass
- WiFi+BLE coexistence 80/80 HTTP requests with BLE active
- MCP read-only enforcement verified
- One fix applied: DIS Serial Number changed from WiFi MAC to BLE MAC
- Full report: [`ACCEPTANCE.md`](wo6-validation/ACCEPTANCE.md)

**Unresolved items:**
- ~~[#232](https://github.com/matthewmcneill/esp32-app-foundation/issues/232): Device name unification (P2, deferred)~~ BLE surface fixed (dev.71)
- [#232](https://github.com/matthewmcneill/esp32-app-foundation/issues/232): Remaining surfaces (TLS cert CN, LVGL boot title, MCP server name, OTA UA, mDNS)
- [#230](https://github.com/matthewmcneill/esp32-app-foundation/issues/230): OTA module unloading RFC (future)
- Sample app remediation (separate project)
- `FRAMEWORK_VERSION` auto-injection from git tags (future)
- BLE portal page UX redesign (bridge prompt prepared for next session)
- Hooks investigation: `.agents/hooks.json` flash-tag-gate hook did not fire on dev.70 flash — root cause undetermined

### Session — 2026-09-24T22:35:00+01:00 — PR #235 Re-Review + Issue #232 BLE Fix

**Summary:** Addressed all PR #235 re-review comments (Round 2), added `_stateMutex` thread safety (O2), and fixed issue #232's BLE surface (advertised name auto-wires from app identity).

**Commits (this session):**

| Tag | Commit | Description |
|-----|--------|-------------|
| `v0.0.12-dev.69` | `71ef9480` | fix(ble): PR #235 re-review — N1-N3 blockers, N4-N6, O6-O7, O9, R5, R9, Y1, Y8 |
| `v0.0.12-dev.70` | `476d252e` | fix(ble): O2 — add _stateMutex for REST/MCP thread safety |
| `v0.0.12-dev.71` | `cee0bb30` | fix(ble): issue #232 — auto-wire advertised name from app identity |

**PR #235 re-review items resolved:**
- **N1 (BLOCKER):** ChainingCharCallbacks — NimBLE 2.5.0 never has null callbacks; must chain
- **N2 (BLOCKER):** setClientCallbacks leak — moved inside create-only branch
- **N3 (BLOCKER):** Dangling providerId in event payloads — use getProviderId() (static lifetime)
- **N4:** 6-arg setConnectionParams with scan interval/window
- **N5:** millis() replaces xTaskGetTickCount (different time bases with PSRAM memtest)
- **N6:** Lambda subscribe captures providerId by value; s_clientProviderMap removed
- **O2:** `mutable std::recursive_mutex _stateMutex` with `lockState()` RAII accessor. All REST/MCP handlers + inhibitScanning/releaseScanInhibit + dispatchQueueEvents now hold the lock
- **O6:** Removed hasFault() check from PeerDisconnected advertising restart
- **O7:** Bond result consumed, stored device updated and persisted
- **O9:** executeCentralForget takes BLEPeerAddress for address-based bond deletion
- **R5:** setMaxResults(0) for callbacks-only discovery
- **R9:** forget() synchronously erases state before queuing worker
- **Y1:** .pyc files removed from tracking
- **Y8:** FTMS Feature bits 0x4486→0x4482, Appearance removed from SampleFtmsProvider

**Issue #232 fix (BLE surface):**
- Added `setDefaultAdvertisedName()` mirroring `WifiManager::setDefaultHostname()`
- `BLEManager::begin()` auto-pulls `app.getAppName()` as factory default
- Persisted user config still overrides
- Hardware verified: `adv.local_name = 'ESP32 App Foundation-8E92'`

**Cross-cutting decisions:**
- O2 mutex is `recursive` because REST handlers call `pair()`/`forget()` which are also called during `dispatchQueueEvents()` (same lock path)
- Issue #237 (O2 thread safety) created and closed — fixed directly in PR #235 per user directive ("no technical debt, no splitting")
- SampleFtmsProvider Appearance codes removed — consumer-specific, not framework

**Dead ends (project-wide):**
- `NimBLECharacteristic::getCallbacks() == nullptr` guard: NimBLE 2.5.0 constructor always sets a static default callback — never returns nullptr. Must CHAIN callbacks via wrapper class.
- `BLEDevice.rssi` attribute in bleak 3.x: doesn't exist. Must use `BleakScanner.discover(return_adv=True)` and read `AdvertisementData.rssi`.
- `.agents/hooks.json` flash-tag-gate: correctly written, valid location per official docs, but did NOT fire when flashing dev.70 without a tag. Root cause unknown — possibly an Antigravity hooks loading issue.

**Hardware verification (v0.0.12-dev.71):**
- BLE advertising: active, name = "ESP32 App Foundation-8E92" ✅
- Scan response: 27/31 bytes used ✅
- DRAM heap: 67,439 B free ✅
- droppedEvents: 0 ✅
- Mac BLE scan confirms `adv.local_name` correctly shows full app name ✅
- Firmware version: v0.0.12-dev.71 ✅

**Pending for next session (bridge prompt prepared):**
- BLE portal page UX redesign: 3 cards (Status, Central Clients, Connections) + diagnostics drawer
- Bottom nav ordering: WiFi → Bluetooth → Security → System (add data-order attributes)
- Add `bleAddress` field to status API


### Session — 2026-09-25 — PR #235 third-round fixes (WO-8, WO-9, WO-10) and #236
**Summary:** The Chief Architect's third-round review of `665b847` found one 🔴 (a use-after-free after a failed first central connect) and three 🟠 in-PR items (O2 lock scope, flash writes under the lock, the R9 race). The human approved narrowing the fixes to "fix the current PR" (no new PR) and asked for #236 to be fixed fully now. WO-8 (sonnet) and WO-10 (opus) ran in parallel in separate worktrees; WO-9 (sonnet) was stacked on WO-8. WO-10 was then rebased onto the WO-9 tip (disjoint files, clean). The PA reviewed every diff.
**Outcomes:**
- **WO-8:** one client per provider for its lifetime, deleted only in forget; every failure path reports back; discovery UUID filter and RSSI.
- **WO-9:** whole-`tick()`, config-hook and precondition locking; `_configDirty` deferred persistence (the single `saveModuleConfig` call runs outside the lock); synchronous registry removal in `forget()`; `resetConfig()` default name.
- **WO-10:** STA-first backoff; fallback AP in AP+STA mode after more than 3 min with auth enforced (`isSetupMode()`); Wi-Fi never switches itself off with credentials; modem sleep on BLE builds.

**Verification (integrated head):** root `esp32s3nano_ble` RAM 61,380 / Flash 1,635,307; root `esp32s3nano` 57,940 / 1,332,831; sample-app BLE 62,484 / 1,767,839. No warnings in the BLE or Wi-Fi sources. Native: 943 cases, 942 pass (`test_lvgl_stubs` pre-existing); `test_bleManager` and `test_wifiManager` pass.
**Residual:**
- The public `pair()`, `forget()` and `startDiscovery()` rely on the caller holding the lock.
- `_discovery.results.clear()` and `_pauseReason.clear()` are unlocked on some paths.
- `docs/design_specification.md` still describes the old AP bypass.
- Native tests need glob filters (`-f "*test_x"`).
- Central-role hardware acceptance (RFC §17.2 items 4–9) and the #236 router-drop hardware checks are still to be run (interactive).

**Recorded for WO-7 (not in this PR):** TX power becomes `BLEConfig.txPowerDbm`, default +20 dBm (owner decision). The WO-7 branch needs a rebase onto the new head.
**Next:** fast-forward `ble-manager/wo6-validation` (PR #235 head) and push; post the PR update; add "Fixes #236" to the PR body.
