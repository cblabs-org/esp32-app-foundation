# PR-16: Backup System Overhaul

**Branch:** `pr-16-backup-overhaul`
**Status:** 🔄 WIP — WO1 in progress
**Created:** 2026-06-03

## Summary

The `/api/config/backup` and `/api/config/restore` routes have never fired on
device. ESPAsyncWebServer prefix-matches the earlier-registered `/api/config`
route, swallowing all sub-path requests. The backup download returns only the
namespace list instead of actual config data.

## Work Orders

| ID | Topic | Branch | Status |
|----|-------|--------|--------|
| **[wo1-firmware](./wo1-firmware/WORKORDER.md)** | Route reorder, streaming response, `_meta` envelope, OTAUpdateManager IConfigurable | `pr-16-backup-overhaul-wo1-firmware` | 🔄 IN PROGRESS |
| **[wo2-frontend](./wo2-frontend/WORKORDER.md)** | 16KB guard, metadata preview, restore summary UX | `pr-16-backup-overhaul-wo2-frontend` | ⏳ QUEUED (awaits WO1) |
| **[wo3-tests](./wo3-tests/WORKORDER.md)** | Mock server realistic data, new unit tests | `pr-16-backup-overhaul-wo3-tests` | ⏳ QUEUED (awaits WO1) |

## Key Decisions

- **Root cause**: Route registration order in `ConfigManager::registerRoutes()` — specific paths must be registered before generic parent
- **ArduinoJSON v7**: Dynamic heap allocation — no fixed capacity needed
- **`_meta` envelope**: `backup_version`, `firmware_version`, `device_id` (MAC), `device_type`, `datetime`, `timezone`, `module_count`
- **OTAUpdateManager**: Extend to `IConfigurable` with `"ota"` namespace, fields: `channel`, `autoCheck`
- **WiFi passwords**: Confirmed stored in KeyManager (NVS) — not in config files
- **Streaming**: Use `AsyncResponseStream` — no intermediate `String` buffer
