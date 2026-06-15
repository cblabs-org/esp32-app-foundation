# PR-25 — Framework Developer Docs + Extensibility Refinements

**Branch:** `pr-25-framework-docs-skill`
**Base:** `main`
**Status:** 📋 PLANNING — Work Orders written, awaiting user approval to execute
**Design Document:** [pr25_design.md](../../../../.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/pr25_design.md)

---

## Objective

Two goals:
1. **Developer onboarding documentation** — any human or agent who clones this
   repo gets clear instructions for extending, modifying, and building on the framework.
2. **Architectural fitness** — close the gap between what the framework promises
   and what a developer actually gets. Code refinements where documentation alone
   cannot fix friction.

---

## Work Order Ledger

| WO | Topic | Complexity | Status | Dependencies |
|---|---|---|---|---|
| **[ ] [wo1-arch-refinements](wo1-arch-refinements/WORKORDER.md)** | `registerModule<T>()` + `ITickable::tick(uint32_t)` + `begin()` migration | `architect` | QUEUED | none |
| **[ ] [wo2-eventbus-upgrade](wo2-eventbus-upgrade/WORKORDER.md)** | EventBus `std::vector` migration + module-owned topic constants | `standard` | QUEUED | WO1 |
| **[ ] [wo3-template-expansion](wo3-template-expansion/WORKORDER.md)** | Template expansion (all extension points) + CONTRIBUTING.md fix | `standard` | QUEUED | WO1 + WO2 |
| **[ ] [wo4-developer-docs](wo4-developer-docs/WORKORDER.md)** | Developer documentation (5 MVP guides) | `architect` | QUEUED | WO3 |
| **[ ] [wo5-framework-expert-skill](wo5-framework-expert-skill/WORKORDER.md)** | `.agents/skills/framework-expert/` AI skill | `standard` | QUEUED | WO4 |
| **[ ] [wo6-phase1b-guides](wo6-phase1b-guides/WORKORDER.md)** | Deferred guides (optional modules, data fetches, build system) | `standard` | GATED | WO5 + user decision |

### Sequencing

```
WO1 (arch) → WO2 (EventBus) → WO3 (template)
                                     │
                                     └→ WO4 (docs) → WO5 (skill) → WO6 (Phase 1b)
```

All WOs are serialised. Each builds on the previous.

---

## Key Design Decisions

1. **D1 — `registerModule<T>()`**: Compile-time `if constexpr` auto-detects all
   framework interfaces. Framework's own `begin()` migrates to use it (eat own dog food).
2. **D2 — `ITickable::tick(uint32_t currentMillis)`**: Breaking signature change.
   Application captures `millis()` once per loop and injects it.
3. **D3 — EventBus vector migration**: Replace static `Subscription[32]` with `std::vector`.
4. **D4 — Module-owned event topics**: No central `FrameworkEvents.hpp`. Each module
   declares its own topic constants.
5. **D5 — `FrameworkConfig`**: Document as "OTA secrets only". Elimination deferred.
6. **D6 — Template expansion**: Show ALL primary extension points.

---

*Created: 2026-06-15*
