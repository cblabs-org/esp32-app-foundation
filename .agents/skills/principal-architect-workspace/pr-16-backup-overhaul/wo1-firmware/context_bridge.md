---
title: WO1 Firmware — Backup System Overhaul
distilled_at: 2026-06-03T22:19:00Z
original_plan_id: pr-16-backup-overhaul-wo1-firmware
artifacts:
  - context_artifacts/adr_backup_overhaul.md
  - context_artifacts/adr_ota_iconfigurable.md
  - context_artifacts/graveyard_route_ordering.md
  - context_artifacts/state_configmanager_api.md
---

## Executive Summary

WO1 is **COMPLETE** and committed (`4c67b43` on `pr-16-backup-overhaul`). The ESP32 backup system is now fully functional: the route-ordering bug is fixed, the `/api/config/backup` endpoint returns a `_meta` envelope with 5 module sections, and `OTAUpdateManager` implements `IConfigurable` so OTA config participates in backup/restore.

## Next Steps

WO2 (web portal) and WO3 (native tests) are next. If picking up WO2:
1. Read `WORKORDER.md` in the WO2 directory for exact scope
2. The backup endpoint shape is now fixed — the JS client can safely `GET /api/config/backup` and `POST /api/config/restore`
3. The `_meta` object is always first in the backup JSON; restore logic must skip it
4. `OTAUpdateManager` registers at slot 5/8 — the portal should display all 5 namespaces: display, wifi, mcp, time, ota

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_backup_overhaul.md` — Why route ordering was fixed, AsyncResponseStream choice, _meta envelope design
- `context_artifacts/adr_ota_iconfigurable.md` — Why OTA allocation was moved to Step 7, namespace/schema choices, timezone coupling decision
- `context_artifacts/graveyard_route_ordering.md` — The ESPAsyncWebServer prefix-greedy bug, what was tried, confirmed fix
- `context_artifacts/state_configmanager_api.md` — Live API shapes, backup JSON schema, boot log sequence
