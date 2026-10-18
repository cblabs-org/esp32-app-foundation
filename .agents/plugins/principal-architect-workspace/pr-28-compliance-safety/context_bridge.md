---
title: PR-28 Compliance Safety — Full Remediation + Architecture Review
distilled_at: 2026-06-18T10:22:00Z
original_plan_id: pr-28-compliance-safety
session_id: bd5ee383-afe0-4879-978c-6da0e69d96fa
artifacts:
  - context_artifacts/adr_embedded_memory_strategy.md
  - context_artifacts/adr_string_migration_scope.md
  - context_artifacts/graveyard_bootloop_recovery.md
  - context_artifacts/state_pr28_endstate.md
---

# PR-28 Compliance Safety — Context Bridge

## Executive Summary

PR-28 delivered 11 Work Orders across 55 files (+1,367 / −869 lines), systematically remediating every actionable finding from the framework compliance audit plus an embedded systems architecture review. The device is running `v0.0.10-dev.8` with 562/562 native tests passing.

## What Was Accomplished

| Phase | WOs | Key Deliverables |
|-------|-----|-----------------|
| Safety foundations | WO1–4 | `std::nothrow` everywhere, TWDT compliance, TAG constexpr, facade completeness |
| API-level refactors | WO5–6 | `std::function` eradication, Arduino `String` → `std::string` in modules |
| Architectural cleanup | WO7–9 | ADR-39 Transient Service Locator, naming normalisation, manifest TLS upgrade |
| Runtime hotfixes | — | TWDT bootloop fix, MCP input buffer cap (8KB), `otadata` partition recovery |
| Architecture review follow-up | WO10–11 | WiFiClientSecure heap alloc, OTA stack hardening, `String` → `std::string` in all authored libs |

## Next Steps

1. **No immediate code work required.** The codebase is stable and all review findings are resolved.
2. **GitHub CI** — The failing GitHub Actions workflows on `main` remain disabled/broken (investigated in session `624526fd`). This is a separate concern.
3. **GitHub API JSON parsing** — The `IncompleteInput` error on API responses is pre-existing and handled gracefully. Fix requires larger JSON buffer or streaming parser — tracked but low priority.
4. **When going to production** — Enable NVS encryption, Secure Boot, Flash Encryption, eFuse anti-rollback. Research captured in `sdkconfig.defaults` lines 155-199.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_embedded_memory_strategy.md` - PSRAM vs DRAM allocation rules for OTA, TLS stack pressure analysis, WiFiClientSecure heap allocation rationale
- `context_artifacts/adr_string_migration_scope.md` - Why modules/ migrated but framework API interop left alone, Logger overload design decision
- `context_artifacts/graveyard_bootloop_recovery.md` - TWDT bootloop root cause, MCP abort crash, otadata partition recovery procedure
- `context_artifacts/state_pr28_endstate.md` - Final file manifest, memory baselines, device telemetry snapshot
