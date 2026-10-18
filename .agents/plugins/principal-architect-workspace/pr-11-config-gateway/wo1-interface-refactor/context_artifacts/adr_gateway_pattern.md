# ADR: ConfigManager Gateway Pattern

## Context
The original ConfigManager was a monolithic class owning a `SystemConfig` struct with `loadConfig()` / `saveConfig()` methods that serialized all system fields into a single `system_config.json` file. Each module that needed config (WiFi, Display) also implemented its own independent `loadConfig()` / `saveConfig()` with direct LittleFS access.

## Decision
Refactor ConfigManager into a **centralised configuration gateway** using a fixed-size registry pattern:

- **Registry Model**: ConfigManager maintains a `iConfigurable* _modules[MAX_CONFIGURABLES]` array (MAX=8) and a `_moduleCount` counter. No heap allocation for the registry itself.
- **Namespace Isolation**: Each module declares a unique namespace via `getConfigNamespace()`. Config is persisted at `mod_<namespace>.json` — one file per module.
- **Schema Versioning**: Each config file embeds a `_v` field matching `getSchemaVersion()`. On load, if the file version is older, `migrateConfig()` is called for in-place migration.
- **Gateway API**: `beginAll()` iterates the registry and loads each module's config. `saveModuleConfig()` persists a single module. `exportAllConfig()` / `importAllConfig()` handle bulk operations for backup/restore.

## Rationale
1. **Decoupling**: Modules no longer need direct LittleFS access — the gateway owns all I/O.
2. **Schema Evolution**: The `_v` + `migrateConfig()` pattern allows non-breaking schema changes without manual migration scripts.
3. **Fixed-size Registry**: Avoids heap fragmentation on embedded targets. 8 slots is sufficient for the current module count with room to grow.
4. **Bulk Operations**: `exportAllConfig()` / `importAllConfig()` provide a clean foundation for the backup/restore REST API (WO2).

## Consequences
- The old `SystemConfig` struct and `SYSTEM_CONFIG_PATH` / `SYSTEM_CONFIG_VERSION` constants are fully removed.
- MCP builtin tools (`get_config`, `set_config`) now use the gateway's export/import API instead of direct struct access.
- Modules must implement the 6-method `iConfigurable` interface to participate in centralised config management.
