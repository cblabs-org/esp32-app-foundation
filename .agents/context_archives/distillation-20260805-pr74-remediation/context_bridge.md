---
title: "PR #74 — Post-Merge Remediation + EventBus Boot-Order + Configuring Gate Fixes"
distilled_at: "2026-08-05T16:22:00+01:00"
original_plan_id: "pr-68-remediation"
artifacts:
  - context_artifacts/adr_eventbus_boot_order.md
  - context_artifacts/adr_gate_dependency_ordering.md
  - context_artifacts/graveyard_dangling_pointer_ip.md
  - context_artifacts/state_pr74_final.md
---

# Context Bridge — PR #74 Remediation Session

## Executive Summary

Completed full post-merge remediation of PR #68 (CONFIGURING orchestration). Discovered and fixed a critical EventBus boot-order race condition that silently dropped display subscriptions, added WiFi gate dependency to Weather module to enforce correct gate ordering, and fixed a dangling pointer in IP change detection. All work is on branch `pr-68-remediation`, pushed as PR #74, rebased on main (which now includes encoder PR #75). All 417 native tests pass; framework and sample-app builds green; hardware-verified on device.

## Next Steps

1. **Merge PR #74** — review and merge https://github.com/matthewmcneill/esp32-app-foundation/pull/74 to main.
2. **Screen flicker investigation** — user reported dim/bright flashing on the OLED during RUNNING state. Not caused by our changes (no screensaver/contrast cycling in logs). Likely hardware-level SPI timing or OLED controller issue. Investigate separately.
3. **Issue #39 (brandable screens)** — partially addressed by `IU8G2SystemScreens` extension but the full branding story (boot splash, system boards) remains open.
4. **Issue #67 (web portal CSS)** — untouched, future enhancement.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_eventbus_boot_order.md` — Why EventBus subscriptions silently failed pre-begin() and the lazy event loop solution
- `context_artifacts/adr_gate_dependency_ordering.md` — Why Weather showed before WiFi and the getDependencies() fix
- `context_artifacts/graveyard_dangling_pointer_ip.md` — The temporary String dangling pointer that caused re-publish spam
- `context_artifacts/state_pr74_final.md` — Final branch state, commit log, file manifest, and validation results
