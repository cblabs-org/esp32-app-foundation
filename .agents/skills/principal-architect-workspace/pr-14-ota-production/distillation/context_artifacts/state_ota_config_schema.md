# State: OTA Config Schema v3

## Namespace: `ota`
## Schema version: 3

### Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `channel` | `char[16]` | `"stable"` | Release channel: `"stable"`, `"beta"`, or `"dev"` |
| `autoCheck` | `bool` | `true` | Enable boot auto-check and daily scheduled check |
| `checkHour` | `uint8_t` | `3` | Hour of day (0–23) for daily scheduled check |
| `lastCheckedChannel` | `char[16]` | `""` | Last channel used; tracks implicit channel-change downgrade |

### Migration history

| From | To | Change |
|------|----|--------|
| 1 | 2 | Added `lastCheckedChannel` |
| 2 | 3 | Added `checkHour = 3` |

### API actions (POST /api/ota)

| Action | Payload | Effect |
|--------|---------|--------|
| `status` | — | Returns `state`, `version`, `channel`, `boardId`, `autoCheck`, `checkHour`, `lastError` |
| `checkGithub` | — | Spawns performUpdate() FreeRTOS task |
| `setChannel` | `{ channel: string }` | Saves channel, persists |
| `setAutoCheck` | `{ autoCheck: bool }` | Saves autoCheck, persists |
| `setCheckHour` | `{ checkHour: int (0-23) }` | Saves checkHour, persists |
| `markValid` | — | Calls esp_ota_mark_app_valid_cancel_rollback() |
| `rollback` | `{ confirm: "ROLLBACK" }` | Triggers rollback to previous OTA slot |

### Scheduling state (NOT persisted — reset each boot)

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `_lastCheckDay` | `uint32_t` | `0` | Unix day of last scheduled check (`time()/86400`) |
| `_hasBootChecked` | `bool` | `false` | Set true after first boot-check tick fires |
