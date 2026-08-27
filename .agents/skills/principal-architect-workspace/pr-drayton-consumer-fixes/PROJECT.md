---
name: drayton-consumer-fixes
description: Resolve 9 consumer-filed issues (#160-#168) from Drayton Wiser Controller pre-flight verification.
---

# Project: drayton-consumer-fixes

> Resolve 9 issues (#160–#168) filed by `program-wiser-controller` (Drayton Wiser
> Controller, PR-21) during pre-flight verification of a framework submodule bump.
> All issues verified accurate. Fixes span packaging, API coherence, transport
> coverage, health semantics, and backoff proportionality.

## Human Approval

- [x] Decomposition approved by human: 2026-08-27 — "Yes please - go ahead. Then I give you autonomous approval to proceed as defined."
- [x] Sequencing approved: WO1 → WO3 → WO4 → WO2 → WO5. One PR, no fast-tracking.
- [x] Design decisions approved:
  - #166: per-source virtual `getMaxBackoffMs()` (user directive)
  - #164: coverage gap reframing (not TWDT regression)
  - F2: quarantine threshold decoupled from backoff cap
  - F4: supervisory backstop for unsupervised-path sources

## Architectural Position

**Extend the four-layer bounded transport architecture to cover both TLS and plain-HTTP
sources, fix health semantics for unconfigured sources, and make backoff proportional
to priority tier with per-source override.**

Key design decisions:
- **D1:** `FetchContext::client` retypes from `WiFiClientSecure*` to `WiFiClient*` (same underlying type via typedef). Framework owns both clients.
- **D2:** `IDataSource::requiresTls()` (default `true`) selects which client the framework provides.
- **D3:** `UpdateStatus::Skipped = 6` — explicit arm in `recordFetchOutcome()`, guards `lastAttemptMs` only.
- **D4:** `IDataSource::getMaxBackoffMs()` — tier-derived defaults, per-source override, floor enforced in DataManager.
- **D5:** Quarantine on `consecutiveFailures >= QUARANTINE_FAILURE_THRESHOLD` (≈6), decoupled from backoff cap.
- **D6:** `FrameworkSecureClient::applyDeadline()` — fix `setTimeout()` units bug (#168), use `setConnectionTimeout(ms)`.
- **D7:** Supervisory backstop — fetch overrun detector for sources that decline `ctx.client`.
- **D8:** `TwdtFetchGuard` remains unconditional for both transports (WO8 design decision preserved).

## Prerequisites & Dependencies

| Dependency | Status | Impact |
|---|---|---|
| PR-128 DataManager resilience | ✅ Merged (PR #159) | Four-layer architecture, health state machine |
| PreconditionEngine (PR #149) | ✅ Merged | Precondition infrastructure |
| ITabRegistry (PR #150) | ✅ Merged | Portal tab pattern |
| IPreconditionRegistry (PR #151) | ✅ Merged | Interface naming |

## Modules & Components

| Module / Area | WOs | Notes |
|---|---|---|
| `include/types/FrameworkSecureClient.hpp` | WO1, WO3 | Untrack fix, then Layer 1 units fix |
| `modules/DataManager/DataManager.cpp` | WO1, WO3, WO4 | resolveHostBounded, FetchGuard, Skipped, backoff |
| `modules/DataManager/DataManager.hpp` | WO3, WO4 | Plain client member, quarantine threshold |
| `include/interfaces/IDataSource.hpp` | WO3, WO4 | requiresTls(), Skipped, getMaxBackoffMs() |
| `include/types/FetchContext.hpp` | WO3 | client type change |
| `include/AppFoundation.h` | WO2 | Façade event payload includes |
| `sample-app/src/WeatherModule.cpp` | WO3 | Skipped reference example |
| `docs/developer/` | WO2, WO3 | Migration guide, framework-expert skill |
| `.agents/skills/framework-expert/` | WO2, WO4 | Skill docs |
| `test/test_unit/test_dataManager/` | WO3, WO4 | Native test surface |

## Work Order Ledger

- **[ ] [wo1-unbreak-main](wo1-unbreak-main/WORKORDER.md):** Track missing `FrameworkSecureClient.hpp`, suppress `resolveHostBounded` warning. Closes #160.
- **[ ] [wo3-transport-health](wo3-transport-health/WORKORDER.md):** Dual-transport support (#164), `Skipped` status (#165), `resolveHostBounded` wiring (#161), FetchGuard extraction, supervisory backstop. Closes #161, #164, #165, #168.
- **[ ] [wo4-backoff-proportionality](wo4-backoff-proportionality/WORKORDER.md):** Per-source `getMaxBackoffMs()`, decoupled quarantine threshold. Closes #166.
- **[ ] [wo2-docs-facade](wo2-docs-facade/WORKORDER.md):** Façade includes (#163), stale docs (#162), reserved prefixes (#167). Closes #162, #163, #167.
- **[ ] [wo5-device-verification](wo5-device-verification/WORKORDER.md):** Hardware build, flash, MCP probe, 30-min soak test with serial + MCP monitoring.

## Worker Model Mapping

| WO | Complexity | Model |
|---|---|---|
| WO1 | `mechanical` | `flash_lite` |
| WO3 | `architect` | `pro` |
| WO4 | `standard` | `flash` |
| WO2 | `standard` | `flash` |
| WO5 | `standard` | `flash` |

## Concurrency Analysis

All WOs are **strictly serialised** — DataManager.cpp is touched by WO1, WO3, and WO4.

| WO | Depends on | Shared files with next |
|---|---|---|
| WO1 | `main` | WO3 edits DataManager.cpp, FrameworkSecureClient.hpp |
| WO3 | WO1 | WO4 edits DataManager.cpp/.hpp, IDataSource.hpp |
| WO4 | WO3 | WO2 edits docs that reference WO3/WO4 changes |
| WO2 | WO4 | WO5 reads everything |
| WO5 | WO2 | — |

## Risk Register

| Risk | Severity | Mitigation |
|---|---|---|
| `ctx.client` type change breaks consumers | Low | G2 confirmed: only `sample-app` uses it, compiles unchanged |
| `Skipped` fallthrough in `default:` arm | High | F1: explicit `case Skipped: return;` before `default:` |
| `High` tier quarantines after 2 failures | High | F2: quarantine decoupled from cap, failure-count-driven |
| `applyDeadline()` units bug (#168) | Medium | F3: fix via `setConnectionTimeout(ms)` in WO3 |
| FetchGuard timer lifetime | Medium | G4: explicit acceptance criterion — timer deleted before client |
| Unsupervised-path sources | Medium | F4: supervisory backstop in DataManager |

## Session Log

### 2026-08-27 — Session 1 (Analysis, Review, Decomposition)
- Triage analysis verified all 8 issues accurate against `1e16923f`.
- TWDT unsub/resub correctly reframed as deliberate WO8 design decision, not regression.
- Chief architect review identified 4 must-fix findings (F1-F4), 3 corrections (C1-C3), 4 gaps (G1-G4).
- Drayton consumer architect review aligned on all major points, filed #168 (`applyDeadline()` units bug).
- User decisions: one PR (no fast-tracking), Drayton sequencing (WO1→WO3→WO4→WO2), #166 via virtual function.
- 5-WO decomposition approved. WO5 added for device verification with 30-min soak test.
- **Base commit for stack:** current `main` tip.

---

*Created: 2026-08-27. Status: **IN PROGRESS** — WO1 pending.*
