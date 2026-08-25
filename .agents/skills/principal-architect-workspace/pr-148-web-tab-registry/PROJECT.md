# PROJECT: Issues #147 + #148 — Web Tab Registry

> PR: `148-web-tab-registry`
> Base: `146-config-gate/wo7-consumer-cleanup` (PR #149)
> Issues: [#147](https://github.com/matthewmcneill/esp32-app-foundation/issues/147), [#148](https://github.com/matthewmcneill/esp32-app-foundation/issues/148)
> **Human approval: 2026-08-23 — full autonomous execution approved.**

## Architectural Overview

Replace the scalar-getter tab metadata pattern on `IWebContributor` with a
registry/visitor pattern (`registerTabs(TabRegistry&)`) — the same approach
already established by `IMCPContributor` and `IPreconditionContributor`.

The current design forces multi-tab modules (`AuthManager`, `LVGLDisplayModule`,
`U8G2DisplayModule`) to create inner stub structs that implement `IWebContributor`
purely to contribute tab metadata, each requiring a dummy `registerRoutes(){}`,
an accessor method, and a separate `registerModule()` call. This produces:

- **Silent omission risk** — forget a `registerModule(display.settingsTabStub())`
  and the tab vanishes with no compile-time error.
- **Boilerplate tax** — 8 inner stub structs across 3 modules.
- **Architectural asymmetry** — `registerRoutes()` already uses a registry
  (one call, many routes), but tab metadata uses scalar getters forcing 1:1.

The fix is a clean break: `registerTabs(TabRegistry&)` becomes pure virtual,
all scalar tab getters are removed, and every `IWebContributor` implementor
migrates. `WebServerManager` itself becomes an `IWebContributor` for its
hardcoded "Status" tab — framework eats its own dog food.

**Issue #147** is closed here: the helpText borrowed-pointer defect was
**fully resolved** by PR #149's `DescribeFn` pattern. The `_helpTextBuf` is
already a member variable in `Application.hpp:505` — no additional work needed.
Issue #147 will be closed with a comment noting PR #149 resolved it.

### Interface Audit

A full audit of all 9 registration interfaces confirmed `IWebContributor` is the
**sole interface** suffering from the inner-stub / multiple-`registerModule()`
pattern. All others are 1:1 singletons, already use registry patterns, or handle
1:N at the subsystem level.

## Prerequisites & Dependencies

| Dependency | Status | Impact |
|---|---|---|
| PR #149 (PreconditionEngine) | ⏳ Open — stacking on top | Base branch; must merge first |
| `IPreconditionContributor` pattern | ✅ Delivered in PR #149 | Reference implementation for registry pattern |
| `IMCPContributor` pattern | ✅ Exists on `main` | Second reference implementation |

## Modules & Components

| Module | WOs | Role |
|---|---|---|
| `TabRegistry.hpp` (new) | WO1 | Registry type + `TabEntry` struct |
| `IWebContributor.hpp` | WO1, WO2 | Interface extension → clean break |
| `Application.hpp/.cpp` | WO1, WO2 | Stub registration removal, internal contributor migration |
| `AuthManager.hpp` | WO1 | Delete 2 stubs, add `registerTabs()` |
| `LVGLDisplayModule.hpp` | WO1 | Delete 3 stubs, add `registerTabs()` |
| `U8G2DisplayModule.hpp` | WO1 | Delete 3 stubs, add `registerTabs()` |
| `WebServerManager.hpp/.cpp` | WO2 | Implement `IWebContributor`, refactor tab collection |
| `ConfigManager.hpp` | WO2 | Add `registerTabs()` |
| `KeyManager.hpp` | WO2 | Add `registerTabs()` |
| `WifiManager.hpp` | WO2 | Add `registerTabs()` |
| `OTAUpdateManager.hpp` | WO2 | Add `registerTabs()` |
| `TimeManager.hpp` | WO2 | Add `registerTabs()` |
| `HomeAssistantModule.hpp` | WO2 | Add `registerTabs()` |
| `MCPServer.hpp` | WO2 | Add `registerTabs()` |
| `WeatherModule.hpp` (sample-app) | WO2 | Add `registerTabs()` — best-practice example |
| `MyApp.hpp` (template-app) | WO2 | Add `registerTabs()` — best-practice example |
| Test mocks & stubs | WO1, WO2 | Update for interface change |

## Work Order Ledger

- **[x] [wo1-types-multi-tab](wo1-types-multi-tab/WORKORDER.md):** New `TabRegistry.hpp`, `IWebContributor` extension with backward-compat default, multi-tab module migrations (AuthManager, LVGLDisplayModule, U8G2DisplayModule — deletes all 8 inner stubs), test stub updates. Build verification. ✅ 828/828 tests, all targets build.
- **[x] [wo2-clean-break](wo2-clean-break/WORKORDER.md):** Remove all scalar tab getters from `IWebContributor` (make `registerTabs()` pure virtual). Migrate all 9 remaining single-tab modules + 2 Application internal contributors. `WebServerManager` implements `IWebContributor` for "Status" tab. Refactor tab collection loop. Consumer app migrations (WeatherModule, MyApp). Full test updates. Final build + test verification. ✅ 824/824 tests, all targets build.

## Worker Model Mapping

| WO | Complexity | Model |
|---|---|---|
| WO1 | `standard` | `flash` |
| WO2 | `standard` | `flash` |

## Concurrency Analysis

Both WOs are **strictly serial** — WO2 depends on WO1's interface changes.

| WO | Depends on | Shared files with next |
|---|---|---|
| WO1 | — | WO2 modifies `IWebContributor.hpp` (removes scalar getters WO1 kept) |
| WO2 | WO1 | — |

## Risk Register

| Risk | Mitigation |
|---|---|
| Tab JSON output differs after refactor | WO2 verification: diff `/api/tabs` response before/after. Tab names, parents, orders must be byte-identical. |
| `WebServerManager` implementing `IWebContributor` creates circular registration | Already registered via `registerModule()` for `ITickable`. Adding `IWebContributor` just auto-detects and adds to `_webContributors`. Self-reference is safe: `begin()` collects before calling. |
| `getApiPrefix()` / `getMaxPostBody()` wrongly removed | Explicitly retained — they serve body-limit middleware, not tab metadata. 1:1 per contributor. |
| Downstream consumer apps break | Breaking change approved by user. Template-app and sample-app ship as migration examples. |
| Display module consumer `main.cpp` still registers stubs | WO1 removes stub accessors — downstream code that calls them will fail at compile time (desired). |

## Downstream Notification Checklist

- [ ] Notify downstream consumers: `IWebContributor` breaking change — scalar tab getters removed, all implementors must add `registerTabs()` override.
- [ ] Notify downstream consumers: delete any `registerModule(display.xxxTabStub())` calls from `main.cpp` — tabs are now automatic.

## Session Log

### 2026-08-23 — Project initialisation
- Interface audit complete: `IWebContributor` confirmed as sole interface with inner-stub problem (8 stubs across 3 modules).
- Issue #147 confirmed structurally resolved by PR #149; minor `helpBuf` cleanup scoped.
- User decisions: Option B (clean break, no deprecation), WebServerManager migrates to `IWebContributor`, zero technical debt in release.
- Decomposition: 2 WOs, strictly serial.
- Stacks on PR #149 (`146-config-gate/wo7-consumer-cleanup`).

### 2026-08-23 — All WOs complete
- WO1: `dd06d109` — TabRegistry + multi-tab migrations. 828/828 tests. Deleted 8 inner stubs across AuthManager (2), LVGLDisplayModule (3), U8G2DisplayModule (3). Also cleaned `src/main.cpp` display stub registrations (discovered during build — outside original WO scope but correct).
- WO2: `b5c67270` — Complete clean break. 824/824 tests (4 fewer: scalar getter tests consolidated into registry-based tests). 28 files changed, 468 insertions, 383 deletions.
- Dead end (WO2): initial test sweep missed `test_wifiManager.cpp` and `test_configManager.cpp` which called removed scalar getters. Caught during PA review of test results (828→722 drop). Fixed by replacing with `registerTabs()` collector tests.
- `getApiPrefix()` and `getMaxPostBody()` confirmed retained — body-limit middleware is orthogonal to tab metadata.

---

*Created: 2026-08-23. Status: All WOs complete — awaiting PR submission.*
