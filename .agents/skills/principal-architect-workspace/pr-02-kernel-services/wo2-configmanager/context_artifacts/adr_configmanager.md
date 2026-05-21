# Architecture Decision Record: Config Manager

## Structured JSON Configuration on LittleFS
The `ConfigManager` handles writing and reading JSON configs from `/etc/config.json` via LittleFS. This centralizes framework-wide properties (like Wi-Fi settings or server credentials) in one place.

## Semi-Automatic Version Migration
To prevent crashes when firmware upgrades modify the expected configuration schema, `ConfigManager` tracks a schema version integer. During load, it performs incremental migration operations to map older keys to the new structure, falling back to safe defaults if migration is impossible.

## Hot-Reloading Support via EventBus
`ConfigManager` supports `requestReload()` to notify registered modules to reload their configs from the filesystem without requiring a full device reboot. This is dispatched as an event.
