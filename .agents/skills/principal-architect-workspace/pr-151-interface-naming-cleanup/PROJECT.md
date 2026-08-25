# PROJECT: PR #151 — Interface Naming Convention Cleanup

> PR: `151-interface-naming-cleanup`
> Base: `148-web-tab-registry/wo2-clean-break` (PR #150)
> Origin: [OO Architecture Assessment](conversation://44040a4c-3c9f-48c1-865d-854f0d0ce086) — R1 & R2 recommendations
> **Human approval: 2026-08-25 — approved, WO1 upgraded to flash per user request.**

## Architectural Overview

The OO architecture assessment (conversation [44040a4c](conversation://44040a4c-3c9f-48c1-865d-854f0d0ce086))
identified two abstract interfaces misplaced in `include/utils/` that lack the
project's `I`-prefix naming convention:

| Current | Target | Nature |
|---|---|---|
| `PreconditionRegistry` in `include/utils/` | `IPreconditionRegistry` in `include/interfaces/` | Pure abstract: virtual destructor + single virtual `add()` |
| `TabRegistry` in `include/utils/` | `ITabRegistry` in `include/interfaces/` | Pure abstract: virtual destructor + single virtual `add()` |

Both are architecturally identical to `ITickable`, `IConfigurable`, etc. The
missing `I`-prefix makes them look like concrete implementations when they are
actually interfaces. Moving them to `include/interfaces/` with the correct
prefix unifies the naming convention and cleans `include/utils/` to contain only
genuine utilities (`Periodic.hpp`, `BufferStream.h`).

This is a **breaking change** for downstream consumers — any code referencing
`PreconditionRegistry` or `TabRegistry` must update include paths and type names.

## Prerequisites & Dependencies

| Dependency | Status | Impact |
|---|---|---|
| PR #150 (Web Tab Registry, WO2) | ✅ Complete on branch | Base branch — must merge first or stack on top |
| PR #149 (PreconditionEngine) | ✅ Complete on branch | Introduced `PreconditionRegistry` — stacked below #150 |

## Modules & Components

| Module | WO | Role |
|---|---|---|
| `include/interfaces/IPreconditionRegistry.hpp` (new) | WO1 | Renamed + relocated from `include/utils/PreconditionRegistry.hpp` |
| `include/interfaces/ITabRegistry.hpp` (new) | WO1 | Renamed + relocated from `include/utils/TabRegistry.hpp` |
| `include/utils/PreconditionRegistry.hpp` (delete) | WO1 | Old location — deleted |
| `include/utils/TabRegistry.hpp` (delete) | WO1 | Old location — deleted |
| `IPreconditionContributor.hpp` | WO1 | Include path update |
| `IWebContributor.hpp` | WO1 | Include path + type update |
| `PreconditionEngine` | WO1 | Include path + base class name |
| `WebServerManager` | WO1 | Include path + type references |
| `Application` | WO1 | Type references |
| `AuthManager` | WO1 | Type references |
| `ConfigManager` | WO1 | Type references |
| `KeyManager` | WO1 | Type references |
| `WifiManager` | WO1 | Include path + type references |
| `TimeManager` | WO1 | Include path + type references |
| `OTAUpdateManager` | WO1 | Type references |
| `HomeAssistantModule` | WO1 | Type references |
| `MCPServer` | WO1 | Type references |
| `LVGLDisplayModule` | WO1 | Type references |
| `U8G2DisplayModule` | WO1 | Type references |
| `WeatherModule` (sample-app) | WO1 | Include path + type references |
| `MyApp` (template-app) | WO1 | Include path + type references |
| Test mocks/stubs | WO1 | Type reference updates |
| `AppFoundation.h` | WO1 | Include path update if applicable |
| `framework-expert` skill docs | WO1 | Name references in documentation |

## Work Order Ledger

- **[ ] [wo1-rename-move](wo1-rename-move/WORKORDER.md):** Rename `PreconditionRegistry` → `IPreconditionRegistry` and `TabRegistry` → `ITabRegistry`, relocate both from `include/utils/` to `include/interfaces/`, update all includers (30+ files), update framework-expert skill documentation, verify zero old-name references remain, run full test + build verification.

## Worker Model Mapping

| WO | Complexity | Model |
|---|---|---|
| WO1 | `standard` | `flash` |

## Concurrency Analysis

Single WO — no concurrency concerns.

## Risk Register

| Risk | Mitigation |
|---|---|
| Missed includer — old name reference survives | WO1 verification: `grep -r` for both old names across all source files — must return zero hits (except `IPreconditionRegistry` / `ITabRegistry` / `TabEntry`) |
| `TabEntry` struct accidentally renamed | Explicitly preserved — `TabEntry` is a data struct, not an interface |
| Downstream consumer apps break | Breaking change is intentional. Template-app and sample-app ship as migration examples |
| `AppFoundation.h` includes stale path | WO1 checklist includes `AppFoundation.h` verification |

## Session Log

### 2026-08-25 — Project initialisation
- Source: OO architecture assessment from [conversation 44040a4c](conversation://44040a4c-3c9f-48c1-865d-854f0d0ce086) — recommendations R1 & R2.
- Bridge prompt from that session provides exhaustive file-by-file impact analysis.
- Decomposition: single WO at `mechanical` complexity — this is pure find/replace/move with zero architectural ambiguity.
- Stacks on `148-web-tab-registry/wo2-clean-break` (PR #150).
- Branch name: `151-interface-naming-cleanup/wo1-rename-move`.

---

*Created: 2026-08-25. Status: Awaiting human approval of decomposition.*
