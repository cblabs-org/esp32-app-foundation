---
title: OTA Close-Out — Boot Trigger, Quiet Hour Scheduler, Release Tooling
distilled_at: 2026-06-06T10:59:00Z
original_plan_id: pr-14-ota-production
artifacts:
  - context_artifacts/adr_ota_scheduler.md
  - context_artifacts/adr_ota_release_tooling.md
  - context_artifacts/graveyard_ota_github_auth.md
  - context_artifacts/state_ota_config_schema.md
---

## Executive Summary

This session closed out the final OTA production items: the `_autoCheck` boot trigger and daily quiet-hour scheduler are now fully implemented in `OTAUpdateManager::tick()`, the module is registered as an `ITickable`, and a configurable daily-check-hour (default 03:00) is persisted as config schema v3 and surfaced in the portal Settings card. Release tooling was overhauled with `--local` mode and key encryption.

## Next Steps

1. **Publish the release** — run from your own terminal (agent GITHUB_TOKEN blocks this):
   ```bash
   python3 scripts/deploy_release.py --tag v0.0.5 --local --private-key private_key.pem
   ```
2. **OTA e2e test** — after publish, press Check for Updates on portal; device should download, verify ECDSA sig, and reboot.
3. **Portal UI polish** — see `.agents/todo_list.md` for remaining items (Advanced section colour, screensaver toggles, Display tab).
4. **`FrameworkConfig` refactor** — slim it down to only app-specific compile-time constants; move module defaults into each module's `resetConfig()`.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_ota_scheduler.md` — why tick() not a FreeRTOS timer; boot-check vs daily-check design
- `context_artifacts/adr_ota_release_tooling.md` — why `--local` mode was added; key encryption approach
- `context_artifacts/graveyard_ota_github_auth.md` — GITHUB_TOKEN dummy env var blocking gh CLI from agent context
- `context_artifacts/state_ota_config_schema.md` — full OTA config schema v3 field reference
