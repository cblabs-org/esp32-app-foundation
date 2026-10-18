# State — PR-16 Backup API Shapes

## Live Backup Response — `GET /api/config/backup`

Validated on device `192.168.20.186` (MAC `80:65:99:A1:8E:90`) after merge commit `4d48d36`:

```json
{
  "_meta": {
    "backup_version": 1,
    "firmware_version": "0.1.0",
    "device_id": "80:65:99:A1:8E:90",
    "device_type": "ESP32-S3",
    "datetime": "2026-06-03T22:30:01Z",
    "timezone": "UTC",
    "module_count": 5
  },
  "display": {
    "brightness": 255,
    "flip": false,
    "sleepTimeout": 300000,
    "rotationInterval": 8000,
    "targetFps": 30,
    "clockFontSize": 2,
    "_v": 2
  },
  "wifi": {
    "ssid": "cb-iot",
    "hostname": "esp32-foundation",
    "_v": 1
  },
  "mcp": {
    "enabled": true,
    "readOnly": true,
    "_v": 1
  },
  "time": {
    "timezone": "Europe/London",
    "posix": "GMT0BST,M3.5.0/1,M10.5.0",
    "ntpServer1": "pool.ntp.org",
    "ntpServer2": "time.nist.gov",
    "ntpServer3": "time.windows.com",
    "_v": 1
  },
  "ota": {
    "channel": "stable",
    "autoCheck": true,
    "_v": 1
  }
}
```

**Note on `timezone`:** Shows `"UTC"` even though `time.timezone` is `"Europe/London"`. `setMetaTimezone()` is available on `ConfigManager` but not yet called from `TimeManager::applyConfig()`. See ADR-4 and follow-up items.

**Note on `_v` fields:** Each module serializes its own schema version via `serializeConfig()`. The `display` module is at schema version 2 (has `clockFontSize`); all others are at version 1.

---

## Namespace List Response — `GET /api/config`

Returns the registered module list (NOT the config data):

```json
{
  "namespaces": ["display", "wifi", "mcp", "time", "ota"],
  "count": 5
}
```

**Critical:** This endpoint previously swallowed all requests to `/api/config/*` due to prefix matching. After the route reorder fix, it now only fires for exact `/api/config` requests.

---

## OTAUpdateManager Config Schema

Namespace: `"ota"` | Schema version: `1` | File: `/mod/ota.json`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | string (16 char max) | `"stable"` | OTA release channel (`"stable"` or `"beta"`) |
| `autoCheck` | bool | `true` | Auto-check for updates on boot |

**Note:** OTA channel is persisted but not yet consumed by `checkForUpdate()` / `performUpdate()`. PR-14 (OTA Production) will wire channel filtering into the GitHub release query.

---

## backup.js UX Behaviour (post-PR-16)

### Download flow
1. `apiFetch('/api/config/backup')` → parsed JSON object
2. `JSON.stringify` → `Blob` → `<a>` download trigger → file saved as `config-backup.json`
3. If `_meta` present in response: display summary in `#backup-download-status`:
   `"Downloaded: 5 modules — firmware v0.1.0, ESP32-S3 (80:65:99:A1:8E:90) — 2026-06-03T22:30Z"`

### Restore flow
1. File selected → `FileReader` reads immediately → parse JSON
2. If parse fails → inline error shown, button stays disabled
3. If parse succeeds → list non-`_meta` keys as `"Will restore: display, wifi, mcp, time, ota"`
4. File size guard: reject if > 16384 bytes (`"File too large (max 16 KB)."`)
5. Click "Upload & Restore": strip `_meta`, `JSON.stringify`, POST to `/api/config/restore`
6. On success: show confirmation message, device reboots

---

## Build Stats (merge commit `4d48d36`)

| Metric | Value |
|--------|-------|
| Flash | 1,189,183 bytes / 2,031,616 (58.6%) |
| RAM | 50,748 bytes / 327,680 (15.5%) |
| Native test cases | 404 / 404 passed |
| Test suites | 17 / 17 passed |
