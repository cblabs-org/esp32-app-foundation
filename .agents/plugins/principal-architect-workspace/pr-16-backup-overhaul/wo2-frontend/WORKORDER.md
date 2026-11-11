---
name: pr-16-wo2
description: Handoff instructions for Work Order 2 — Frontend backup.js improvements
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Improve the backup tab UI in `web/portal/backup.js` to: raise the restore file size guard, show a metadata summary after download, display which modules will be restored before the user commits, and strip `_meta` from the restore payload.

## Context

WO1 changed the firmware so that `GET /api/config/backup` now returns a JSON document with this shape:
```json
{
  "_meta": {
    "backup_version": 1,
    "firmware_version": "1.2.3",
    "device_id": "AA:BB:CC:DD:EE:FF",
    "device_type": "ESP32-S3",
    "datetime": "2026-06-03T22:00:00Z",
    "timezone": "Europe/London",
    "module_count": 5
  },
  "wifi":    { "ssid": "...", "_v": 1 },
  "mcp":     { "enabled": true, "_v": 1 },
  "time":    { "timezone": "...", "_v": 1 },
  "display": { "brightness": 80, "_v": 1 },
  "ota":     { "channel": "stable", "autoCheck": true, "_v": 1 }
}
```

`apiFetch('/api/config/backup')` returns the parsed JS object (ApiQueue calls `response.json()`).

## Strict Boundary Box

- **Only touch:** `web/portal/backup.js`
- Do NOT touch `app.js`, `index.html`, any CSS files, or any other portal scripts
- Do NOT touch any firmware or test files

## Checklist

### 1. Raise restore file size guard

- [ ] Change the client-side file size check from `8192` to `16384` bytes
- [ ] Update the error message to say "max 16 KB"

### 2. Download — metadata summary in status area

- [ ] After a successful download, extract `_meta` from the response object and display a human-readable summary in `backup-download-status`, e.g.:
  ```
  Downloaded: 5 modules — firmware v1.2.3, ESP32-S3 (AA:BB:CC:DD:EE:FF) — 2026-06-03T22:00Z
  ```
- [ ] If `_meta` is missing (older firmware), fall back gracefully: show only the filename

### 3. Restore — pre-upload module summary

- [ ] After the user selects a file (in the `change` event listener), parse the JSON and display which module namespaces it contains in the `backup-file-name` span or a new status element below it
- [ ] List the non-`_meta` keys, e.g.: `"Will restore: wifi, mcp, time, display, ota"`
- [ ] If the file fails to parse as JSON, show an inline error at this point (don't wait for the button click)

### 4. Strip `_meta` before restore POST

- [ ] In the restore button click handler, after parsing and validating the JSON, create a copy of the object with `_meta` deleted before posting:
  ```js
  const payload = { ...parsed };
  delete payload._meta;
  ```
- [ ] POST the cleaned payload (server already ignores `_meta`, but this is belt-and-braces)

## Verification Requirement

After implementing changes:

1. Open the mock server: `cd test/web && node server.js`
2. Open `http://localhost:3000` in a browser and navigate to System → Backup
3. Click "Download Config" — verify the status area shows the metadata summary
4. Select the downloaded file for restore — verify module list appears
5. Click "Upload & Restore" — verify it POSTs successfully

Once verified, **STOP and send a message back to the Principal Architect** with results. Do NOT distill or commit until the Principal Architect explicitly approves.

---

## Status: COMPLETE ✅

- **Approved by:** Principal Architect (f6e289ed-67c9-4dc9-bc37-efae8914e9ae)
- **Commit:** `2a5b9b6` on branch `pr-16-backup-overhaul-wo2-frontend`
- **Commit message:** `feat(backup): raise restore guard to 16KB, add _meta summary and module preview UX`
- **Completed at:** 2026-06-03T22:22:00Z
- **Distillation:** `.agents/plugins/principal-architect-workspace/pr-16-backup-overhaul/wo2-frontend/context_bridge.md`
