# State: OTAUpdateManager Schema v2

## Config File: `/mod/ota.json`

```json
{
  "_meta": { "schema": 2 },
  "channel": "stable",
  "autoCheck": true,
  "lastCheckedChannel": ""
}
```

## Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | `char[16]` | `"stable"` | Active OTA release channel: `"stable"`, `"beta"`, or `"dev"` |
| `autoCheck` | `bool` | `true` | Auto-check for updates on boot |
| `lastCheckedChannel` | `char[16]` | `""` | Last channel used for update check — enables implicit downgrade detection |

## Migration: v1 → v2

```cpp
if (fromVersion < 2) {
    section["lastCheckedChannel"] = "";
}
```

## Implicit Downgrade State Machine

```
checkForUpdate() flow:
  1. Query GitHub via getLatestReleaseForChannel(_channel)
  2. SemVer compare: remote > local? → update available, save _lastCheckedChannel
  3. Channel changed? (_channel != _lastCheckedChannel && _lastCheckedChannel != "")
     → YES: signal update available, persist channel change, return true
     → NO: update _lastCheckedChannel, return false (no update)
```

The `_lastCheckedChannel` is persisted via `saveModuleConfig()` on channel change to prevent repeated downgrade triggers across reboots.
