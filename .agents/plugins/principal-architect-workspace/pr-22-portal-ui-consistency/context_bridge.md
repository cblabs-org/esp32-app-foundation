---
title: PR-22 Portal UI Consistency — WO3 Drawer Unification + Beta Polish
distilled_at: 2026-06-14T13:36:00Z
original_plan_id: pr-22-portal-ui-consistency
artifacts:
  - context_artifacts/adr_drawer_unification.md
  - context_artifacts/graveyard_ota_beta_channel.md
  - context_artifacts/state_portal_drawer_components.md
---

# Executive Summary

PR-22 WO3 completed and merged to `main`: all portal drawers unified on native
`<details>`/`<summary>`. Six commits covering drawer unification, CSS chevron
upgrade, WiFi diagnostics migration, and a11y additions. Device running
`v0.0.7-beta.4`. OTA beta-channel bug deferred to backlog with full context.

# Next Steps

1. **Raise PR-22 GitHub PR** — branch `pr-22-portal-ui-consistency/wo2-advanced-drawers`
   merged to `main`. PR description in `.agents/plugins/principal-architect-workspace/pr-22-portal-ui-consistency/`.
2. **OTA beta-channel debug** — see `todo.md` item "Debug and harden the beta-channel
   OTA GitHub check". Root cause of `IncompleteInput` on `/releases` endpoint unknown;
   premature `Accept-Encoding: identity` fix was reverted.
3. **Deploy v0.0.7 release** — when ready, run:
   `python3 scripts/deploy_release.py --local --build --tag v0.0.7 --private-key private_key.pem`

# Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_drawer_unification.md` — Why CSS border chevron, info-drawer split, native `<details>` decision
- `context_artifacts/graveyard_ota_beta_channel.md` — OTA IncompleteInput failure, what was tried, what was reverted
- `context_artifacts/state_portal_drawer_components.md` — Final CSS definitions for both drawer components
