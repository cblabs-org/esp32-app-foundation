# PROJECT: pr-66-configuring-orchestration

> **RFC:** [#66](https://github.com/matthewmcneill/esp32-app-foundation/issues/66)
> **Resolves:** [#64](https://github.com/matthewmcneill/esp32-app-foundation/issues/64)
> **Base:** `main` @ `4185297`
> **Repo:** `matthewmcneill/esp32-app-foundation` (framework)

## Architectural Objective

Centralise the CONFIGURING lifecycle display orchestration. Application (Ring-1 kernel) owns the flow — which gate is blocking, its action status, when to re-publish. Display modules (Ring-2, optional) are pure event-driven reactors. This eliminates the fragile consumer `main.cpp` catch-up pattern, resolves the wrong-modal bug (#64), and adds the `GateActionStatus` tri-state enum to `IConfigGate`.

## Human Approval

- **Decomposition approved by human:** ✅ 2026-08-04 — User explicitly requested PA setup after approving the implementation plan.
- **RFC approved by consumers:** ✅ Three reviews on #66, all supportive with targeted amendments incorporated.

## Work Order Ledger

| WO | Topic | Complexity | Dependencies | Status |
|:---|:------|:-----------|:-------------|:-------|
| **[x] [wo1-interface-kernel](wo1-interface-kernel/WORKORDER.md)** | `GateActionStatus` enum, `getActionStatus()` on `IConfigGate`, enriched `AppConfiguringPayload`, gate name constants, `getActionableGate()` helper, gate-transition re-publishing in `Application::tick()` | `standard` | None (base: `main`) | ✅ COMPLETE |
| **[x] [wo2-u8g2-display](wo2-u8g2-display/WORKORDER.md)** | `ConfiguringStatusBoard` upgrade, `GenericGateHelperBoard`, three-tier modal dispatch in `_onAppConfiguring()`, `IU8G2SystemScreens::gateHelperBoard()` | `standard` | wo1 | ✅ COMPLETE |
| **[x] [wo3-lvgl-parity](wo3-lvgl-parity/WORKORDER.md)** | `ILVGLSystemScreens` interface additions, `LVGLDisplayModule::_onAppConfiguring()` stub update | `mechanical` | wo2 | ✅ COMPLETE |
| **[x] [wo4-consumer-cleanup](wo4-consumer-cleanup/WORKORDER.md)** | Delete catch-up blocks + AP-mode subscriptions from all three `main.cpp` files, add `BoardRegistry` WiFi dependency (departures-board consumer) | `mechanical` | wo3 | ✅ COMPLETE |

## Concurrency Assessment

All four WOs are **strictly serial** — each depends on the prior's interface changes. No parallelism possible, no merge conflict risk.

## Key Design Decisions (Locked)

1. `GateActionStatus` is a tri-state enum: `Satisfied`, `InProgress`, `NeedsUserAction`
2. `gateHelperBoard()` receives full `AppConfiguringPayload*`, not just `gateName`
3. LVGL parity: interface additions included, implementation deferred to stub
4. `deviceIP` field added to payload — always show IP, never hostname-based URLs
5. 500ms `Periodic` throttle on gate-transition re-publishing
6. Explicit `clearModal()` at top of every `_onAppConfiguring` re-publish
7. Gate names are public API — `GateNames::WIFI_STATION`, `GateNames::TIMEZONE` constants
8. `getActionableGate()` extracted as shared helper on Application
9. Consumer `WIFI_AP_STARTED` subscriptions deleted (superseded by re-publishing)
