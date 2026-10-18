# ADR: OO Expert Module Compliance Findings

## Context

A full architectural review of all 11 modules in `modules/*` was conducted against:
- **Hierarchical Object Model (HOM)** — Domain Meaning Test, Law of Demeter, sub-object API design
- **Service Module Pattern (SMP)** — Orchestrator Anti-Pattern, hub coupling, tick() purity
- **SOLID** — SRP, DIP, encapsulation

## Key Architectural Decision: DI Strategy

**Question raised by user:** Should modules simply receive `Application&` rather than specific sub-dependencies?

**Decision: NO.**

`Application` is the hub/god-object that the SMP explicitly warns against. `U8G2DisplayModule` (before refactor) proved this: it held `Application* _app` and its `tick()` then chained through to `_app->getState()`, `_app->getWifiManager()`, `_app->getConfigGates()` — all Orchestrator Anti-Pattern violations, all enabled by the wide hub seam.

The rule is: **`begin()` may accept `Application&` as a bootstrap scaffold for one-time wiring, but MUST NOT store it as a member.** After `begin()`, only narrow typed members survive. `tick()` must only touch those stored members.

MCPServer is the positive example: `begin()` receives several Application-sourced deps but stores only `ConfigManager*`, `const FrameworkConfig*`. `tick()` never touches Application.

## Per-Module Final Grades

| Module | SMP | HOM | Overall |
|---|---|---|---|
| Application | ✅ | ✅ | A |
| ConfigManager | ✅ | ✅ | A (post-refactor) |
| DataManager | ✅ | ✅ | A |
| EventBus | ✅ | ✅ | A |
| KeyManager | ✅ | ✅ | A− (DIP minor, not addressed) |
| MCPServer | ✅ | ✅ | A− (begin() param count, not addressed) |
| OTAUpdateManager | ⚠️ | ✅ | B+ (hub coupling — DEFERRED) |
| TimeManager | ✅ | ✅ | A− (DIP minor, not addressed) |
| U8G2DisplayModule | ✅ | ✅ | A (post display refactor) |
| WebServerManager | ✅ | ✅ | A |
| WifiManager | ✅ | ✅ | A− (DIP minor, not addressed) |

## OTAUpdateManager — The Remaining Violation

`OTAUpdateManager` holds `Application* app_` (stored from `begin(Application& app)`), then accesses:
- `app_->getConfig()` — for OTA GitHub config fields
- `app_->getEventBus().publish(...)` — for OTA progress events

**Correct fix:** Replace with `begin(EventBus& eventBus, const FrameworkConfig& config)` — or a thin `OTAConfig` struct. This removes the hub reference entirely. **User has a deeper OTA refactor planned; this is deferred.**

## What Was Intentionally NOT Done

1. **DIP interfaces** (`ICryptography`, `IConfigPersistence`): User confirmed concrete DI is acceptable given stable implementations. Introducing interfaces adds overhead with no practical benefit at current scale.
2. **MCPServer begin() restructure**: Cosmetic — 4 params, 3 optional. Not worth the churn.
3. **DataManager comment fix**: Cosmetic.
4. **EventBus #define → constexpr**: Cosmetic.
