# OTA Config Lifecycle

The `ConfigManifest` library manages configuration versioning across OTA updates using a three-tier snapshot model that protects against configuration loss during firmware transitions.

## Three-Tier Model

| Tier | Description | Snapshot Trigger |
|---|---|---|
| **Active** | Currently running config schema version per module | Updated by `ConfigManager` during `beginAll()` |
| **Previous** | Config state before the last OTA download | `snapshotActiveToPrevious()` — called before OTA download |
| **Golden** | Last-known-good config (firmware blessed) | `snapshotActiveToGolden()` — called from `markAppValid()` |

## OTA Integration Points

```
  OTA Download Triggered
         │
         ▼
  snapshotActiveToPrevious() + save()
         │
         ▼
  Download → Flash → Reboot
         │
         ▼
  Boot Sequence (Steps 1-17)
         │
         ▼
  Step 17b: markAppValid()
         │
         ▼
  snapshotActiveToGolden() + save()
```

### Snapshot Timing

- **Pre-download snapshot:** `snapshotActiveToPrevious()` captures the current module config schema versions before the firmware download begins. This ensures that if the new firmware has schema changes, the old config state is preserved for potential rollback.
- **Post-blessing snapshot:** `snapshotActiveToGolden()` captures the config state after the new firmware has been blessed, establishing a new known-good baseline.

## Config Persistence

| Path | Purpose |
|---|---|
| `/config_manifest.json` | The ConfigManifest — schema versions per tier |
| `/mod/<namespace>.json` | Per-module configuration files |

## Migration During OTA

When new firmware ships a higher config schema version for a module:

1. `ConfigManager::beginAll()` loads the existing `/mod/<namespace>.json`
2. The module's `applyConfig()` receives the old-schema JSON
3. The module is responsible for migrating fields (adding defaults for new fields, transforming deprecated fields)
4. On next `saveModuleConfig()`, the updated config is persisted with the new schema version

## Rollback Behaviour

If new firmware crashes before `markAppValid()`:

1. The bootloader reverts to the previous firmware partition
2. The previous firmware loads `ConfigManager` with the **Active** config (which may have been partially migrated)
3. The **Previous** snapshot in `ConfigManifest` records the pre-OTA schema versions, allowing the framework to detect a rollback scenario
4. The **Golden** snapshot remains the last successfully blessed configuration baseline

## Cross-References

- **Human docs:** [`docs/developer/ota-update-system.md`](../../../../../docs/developer/ota-update-system.md) § Config Lifecycle
- **Config system overview:** [`systems/config/overview.md`](../config/overview.md)
- **Config persistence:** [`systems/config/persistence.md`](../config/persistence.md)
- **OTA overview:** [`systems/ota/overview.md`](overview.md)
