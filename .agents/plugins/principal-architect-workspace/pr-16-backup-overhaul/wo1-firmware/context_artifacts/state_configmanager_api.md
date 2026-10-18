# State: ConfigManager API — Live Shapes After WO1

## Live Endpoint: GET /api/config/backup

Device IP: `192.168.20.186` (hostname: `esp32-foundation`)

```json
{
  "_meta": {
    "backup_version": 1,
    "firmware_version": "unknown",
    "device_id": "80:65:99:A1:8E:90",
    "device_type": "ESP32-S3",
    "datetime": "2026-06-03T22:17:11Z",
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
    "timezone": "",
    "posix": "GMT0",
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

## Live Endpoint: GET /api/config

```json
{
  "namespaces": ["display", "wifi", "mcp", "time", "ota"],
  "count": 5
}
```

## Boot Log Sequence (Module Registration)

```
I (4081) ConfigManager: Registered module 'display' (slot 1/8)
I (5390) ConfigManager: Registered module 'wifi' (slot 2/8)
I (5390) ConfigManager: Registered module 'mcp' (slot 3/8)
I (5390) ConfigManager: Registered module 'time' (slot 4/8)
I (5391) ConfigManager: Registered module 'ota' (slot 5/8)    ← NEW in WO1
I (5391) ConfigManager: beginAll() — loading config for 5 module(s)
W (5605) ConfigManager: Config file /mod/ota.json not found — applying defaults for 'ota'
I (5605) OTAUpdateManager: Config reset to defaults — channel='stable' autoCheck=true
I (5605) ConfigManager: Config saved for 'ota'
I (5644) ConfigManager: beginAll() complete
I (7247) OTAUpdateManager: OTA manager initialized with Application reference
```

## Schema Reference: IConfigurable Fields per Module

| Module | Namespace | Schema _v | Persisted Fields |
|--------|-----------|-----------|-----------------|
| U8G2DisplayModule | `display` | 2 | brightness, flip, sleepTimeout, rotationInterval, targetFps, clockFontSize |
| WifiManager | `wifi` | 1 | ssid, hostname, password (via KeyManager) |
| MCPServer | `mcp` | 1 | enabled, readOnly |
| TimeManager | `time` | 1 | timezone, posix, ntpServer1/2/3 |
| OTAUpdateManager | `ota` | 1 | channel, autoCheck |

## Important Notes for WO2 (Web Portal)

1. **`_meta` is not a module namespace** — the restore handler skips it silently. Do NOT send `_meta` as a module section in POST /api/config/restore.
2. **`wifi.password` is not in backup JSON** — it is stored encrypted via KeyManager and excluded from `serializeConfig()` by design.
3. **`setMetaTimezone()` is wired but not called** — `timezone` will always show `"UTC"` until TimeManager is updated to call it in `applyConfig()`.
