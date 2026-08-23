# PROJECT: RFC #146 — Configuration Gate Refactor

> PR: `146-config-gate`
> Base: `main`
> RFC: [#146](https://github.com/matthewmcneill/esp32-app-foundation/issues/146)
> Design: [`docs/config-gate-design.md`](../../docs/config-gate-design.md)
> **Human approval: 2026-08-23 — full autonomous execution approved.**

## Architectural Overview

Replace the `IConfigGate` interface — a boolean predicate that can only block boot
— with a rich `IPrecondition` system that separates *may we boot*, *is this met
right now*, and *is this broken enough to tear down*. The new system introduces:

- **`IPreconditionContributor`** + **`PreconditionRegistry`** — registry/visitor
  pattern (CG-2b) replacing the 1:1 module-to-gate coupling
- **`PreconditionEngine`** — damping, breaker state machine, edge graph with
  `Wants`/`Requires`/`BindsTo`, severity derivation from cause + trust history
- **`Cause` vocabulary** (11 members) replacing the 3-value `GateActionStatus`
- **Operational preconditions** that degrade features without blocking boot (CG-1)
- **Portal + display surfacing** via `precondition.health` EventBus topic (CG-18)

This is a **breaking interface change** with no honest shim. The old default
(`NeedsUserAction`) is the behaviour being fixed.

## Prerequisites & Dependencies

| Dependency | Status | Impact |
|---|---|---|
| `SourceHealth` model (#128) | ✅ Merged — PR #145 landed on `main` (2026-08-23) | CG-15 reads it; CG-4a enriches from it |
| `fetchesWhileConfiguring()` | ✅ Does not exist | CG-17 has nothing to retire — moot |
| `DataManager._appRunning` gate | ⚠️ Exists, must be removed | CG-17 condition: removal is part of WO3 |
| `IWebContributor` tab registry (#148) | ❌ Separate PR | Not entangled with this work |

## Modules & Components

| Module | WOs | Role |
|---|---|---|
| `IPrecondition.hpp` (new) | WO1 | Types, enums, structs |
| `IPreconditionContributor.hpp` (new) | WO1 | Registry/visitor interface |
| `PreconditionRegistry` (new) | WO1, WO2 | Registration target |
| `PreconditionEngine` (new) | WO2 | Core evaluation, damping, breaker, edges |
| `Application` | WO3 | Wiring, state machine, projection |
| `DataManager` | WO3 | `_appRunning` gate removal |
| `AppConfiguringPayload` | WO3 | New precondition health payload |
| `WifiConfigGate` → `WifiPrecondition` | WO4 | Framework gate migration |
| `TimeConfigGate` → `TimePrecondition` | WO4 | Framework gate migration (1→2 preconditions) |
| `WebServerManager` | WO5 | `/api/preconditions` route |
| Portal JS | WO5 | Precondition health UI |
| `ConfiguringStatusBoard` | WO6 | Rewrite for new model |
| `U8G2DisplayModule` | WO6 | Subscribe to `precondition.health` |
| `LVGLDisplayModule` | WO6 | Subscribe to `precondition.health` |
| SLS project (`squareline/crowpanel/`) | WO6 | System UI screen updates |
| `WeatherModule` (sample app) | WO7 | Consumer migration + Indeterminate fix |
| `MyApp` (template) | WO7 | Consumer migration |
| `IConfigGate.hpp` (delete) | WO7 | Interface removal |
| `ConfigGateResolver.hpp` (delete) | WO7 | Utility removal |

## Work Order Ledger

- **[x] [wo1-types-interfaces](wo1-types-interfaces/WORKORDER.md):** New `IPrecondition.hpp`, `IPreconditionContributor.hpp`, `PreconditionRegistry.hpp` — all the types, enums, and interfaces. No behavioral change. Build verification only. ✅ 789/789 tests, all targets build.
- **[x] [wo2-precondition-engine](wo2-precondition-engine/WORKORDER.md):** `PreconditionEngine` — core evaluation loop, edge resolution with cycle detection, damping accumulator, breaker state machine, severity derivation (reads `SourceHealth` for enrichment), trust history. Comprehensive native test suite. ✅ 800/800 tests, 11 new.
- **[x] [wo3-application-wiring](wo3-application-wiring/WORKORDER.md):** Wire `PreconditionEngine` into `Application`. Auto-detect `IPreconditionContributor` in `registerModule<T>`. New state-machine projection (Startup-role gates boot). `DataManager._appRunning` gate removal (CG-17). New `precondition.health` event topic. Update `AppConfiguringPayload`. Existing Application tests migrated. ✅ 797/797 tests.
- **[x] [wo4-framework-migration](wo4-framework-migration/WORKORDER.md):** Migrate `WifiConfigGate` → `WifiPreconditionContributor`. Migrate `TimeConfigGate` → `TimePreconditionContributor` (two preconditions: Timezone + Time Sync). Update built-in registrations in Application. Port existing gate tests. ✅ 801/801 tests, all targets.
- **[x] [wo5-portal-api](wo5-portal-api/WORKORDER.md):** New `/api/preconditions` REST route. Portal JavaScript: prominent precondition health display on System page. Wire into `WebServerManager`. ✅ 801/801 native, 71/71 web.
- **[x] [wo6-display-updates](wo6-display-updates/WORKORDER.md):** Rewrite `ConfiguringStatusBoard` — Startup-only progress bar, overflow handling, rich cause/severity rendering. Both display modules subscribe to `precondition.health`. SLS widget repurposing (round display safe zone). Port display tests. ✅ 803/803 tests, all targets.
- **[x] [wo7-consumer-cleanup](wo7-consumer-cleanup/WORKORDER.md):** Migrate `WeatherModule` gate → precondition with `Unavailable`/`Indeterminate` fix (§2.4). Migrate `MyApp` template. Delete `IConfigGate.hpp`, `ConfigGateResolver.hpp`, and all dead references. Final build + test verification. ✅ 828/828 tests.

## Worker Model Mapping

| WO | Complexity | Model |
|---|---|---|
| WO1 | `standard` | `flash` (Gemini Flash 3.7 High) |
| WO2 | `architect` | `pro` (Gemini 3.1 Pro High) |
| WO3 | `architect` | `pro` (Gemini 3.1 Pro High) |
| WO4 | `standard` | `flash` (Gemini Flash 3.7 High) |
| WO5 | `standard` | `flash` (Gemini Flash 3.7 High) |
| WO6 | `standard` | `flash` (Gemini Flash 3.7 High) |
| WO7 | `standard` | `flash` (Gemini Flash 3.7 High) |

## Concurrency Analysis

All WOs are **strictly serial** — each builds on the last. No parallel execution.

| WO | Depends on | Shared files with next |
|---|---|---|
| WO1 | — | WO2 uses types from WO1 |
| WO2 | WO1 | WO3 uses engine from WO2 |
| WO3 | WO2 | WO4 touches Application.cpp (registration) |
| WO4 | WO3 | WO5 reads precondition state via API |
| WO5 | WO4 | WO6 references same event topics |
| WO6 | WO5 | WO7 deletes old interfaces used until WO6 |
| WO7 | WO6 | — |

## Risk Register

| Risk | Mitigation |
|---|---|
| `DataManager._appRunning` removal (CG-17) re-opens circular dependency | WO3 must verify: any Startup-role precondition backed by an IDataSource can be fetched during CONFIGURING after the gate is removed |
| `std::function` in PreconditionRegistry may be too heavy for constrained targets | Profile in WO2. Fallback: virtual interface on lightweight objects |
| Display modules have two separate implementations (U8G2 + LVGL) | WO6 is explicit about both; tests cover both paths |
| SLS project files copied by downstream consumers | WO6 must notify downstream: review system UI screen changes in `squareline/crowpanel/` |
| Breaking change with no shim means every consumer breaks at once | WO7 is the single cleanup WO; all consumers migrate in one pass. Framework + sample-app + template are in-repo |

## Downstream Notification Checklist

- [ ] Notify downstream consumers that SLS system UI files (`squareline/crowpanel/`) have changed (WO6) — they may need to re-merge into their forked SLS projects.
- [ ] Notify downstream consumers of the `IConfigGate` → `IPreconditionContributor` migration (WO7) — any consumer implementing `IConfigGate` must migrate.

## Session Log

### 2026-08-23 — Project initialisation
- Decomposition drafted: 8 WOs (including WO1b for SourceHealth)
- PR #145 merged: SourceHealth now on main. **WO1b eliminated.**
- Final plan: 7 WOs, strictly serial
- **Human approval received** for full autonomous execution

### 2026-08-23 — Hardware validation & PR submission
- **Hardware crash found & fixed**: `LoadProhibited` on CrowPanel after CONFIGURING→RUNNING transition. Two root causes:
  1. Dangling pointer: `PreconditionHealthPayload` stack-allocated in `tick()` but EventBus dispatch is async on real hardware (sync in native tests — invisible to unit tests)
  2. LVGL thread safety: display callbacks called `lv_label_set_text()` from `esp_event` task. Fix: deferred flag pattern with atomic booleans (matching `_onOtaProgress` pattern)
- All 4 environments build clean: `native` (828/828), `esp32dev`, `esp32s3nano`, `crowpanel-128-rotary`
- CrowPanel physical device: 60s+ stable uptime, 8.4MB free heap, NTP synced, no crash on multiple resets
- **PR [#149](https://github.com/matthewmcneill/esp32-app-foundation/pull/149) created** — `feat(precondition)!: replace IConfigGate with PreconditionEngine (#146)`
- 77 files changed, 7044 insertions, 1931 deletions
- **Status: Awaiting downstream consumer review before merge**

### Dead-ends & lessons
- **MCP monitor spooler**: consistently fails to capture serial data from ESP32-S3 USB JTAG CDC on `/dev/cu.usbmodem101`. The spooler opens the port but writes 0 bytes. Had to use direct pyserial reads as workaround. The MCP tool defaults to `/dev/cu.debug-console` for upload/monitor which is a macOS virtual console, not the CrowPanel. Always specify `port` explicitly.
- **ESP32-S3 USB CDC after flash**: `esptool` reports "Device not configured" on post-flash port reconfiguration — this is normal, the USB CDC drops during reset. The firmware IS flashed correctly. Manual RESET button press required after flash to boot.
- **EventBus sync vs async**: Native tests use synchronous EventBus dispatch (callback runs inline). Real hardware uses `esp_event_loop_run` on a separate task. This means **any EventBus subscriber that touches thread-unsafe APIs (LVGL, SPI, etc.) will crash on hardware but pass native tests**. This is a systemic testing gap worth addressing in a future PR.
- **WO7 flash model quota exhaustion**: Hit 429 rate limit on `flash` model mid-session. Re-spawned on `pro` model which had separate quota. Worked fine.

### Unresolved items (out of scope)
- **Issue [#148](https://github.com/matthewmcneill/esp32-app-foundation/issues/148)**: IWebContributor multi-contribution retrofit — separate PR
- **Module [1] tick budget warnings**: LVGL display module consistently exceeds its 33ms tick budget (~50ms renders). Pre-existing, not caused by our changes. Needs separate investigation (LVGL render batching or tick budget tuning).
- **SLS `.spj` downstream notification**: WO6 determined SLS files did NOT need modification (round display safe zone, dynamic widget repurposing instead). But the downstream notification checklist item should be reviewed once consumers test the new display behaviour.

---

*Created: 2026-08-23. Status: **Implementation complete — PR [#149](https://github.com/matthewmcneill/esp32-app-foundation/pull/149) awaiting review.***

