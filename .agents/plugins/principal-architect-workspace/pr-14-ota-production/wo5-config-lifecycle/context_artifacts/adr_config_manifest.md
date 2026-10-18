# ADR: Three-Tier Config Lifecycle Architecture

## Decision

Implement a Three-Tier config lifecycle (Active + Previous + Golden) as a **separate manifest file** (`/config_manifest.json`) operating alongside the existing per-module config files at `/mod/<namespace>.json`. The manifest tracks schema versions — not config file contents — for each module namespace.

## Context

The OTA system needs rollback safety for configuration. When firmware is updated, config schemas may change. By tracking which schema versions were "known good" (Golden), "pre-update" (Previous), and "current" (Active), the system can detect and recover from config corruption after a failed OTA.

## Design Choices

### Fixed-size arrays over `std::vector`
- `ModuleSlot _slots[kMaxModules]` with `kMaxModules = 16`
- Avoids heap fragmentation on embedded targets
- 16 modules is well above the current 7-module registry

### Schema versions tracked, not file hashes
- The manifest stores `int activeVersion / previousVersion / goldenVersion`
- These map directly to `IConfigurable::getSchemaVersion()` — the existing schema version system
- No additional file I/O needed beyond the manifest itself

### Integration points chosen for minimal coupling
1. **`ConfigManager::beginAll()`** — natural place to set Active versions (after loading all modules)
2. **`OTAUpdateManager::performUpdateTask()`** — snapshot Active→Previous before download
3. **`OTAUpdateManager::markAppValid()`** — snapshot Active→Golden after firmware blessing

### `getManifest()` accessor pattern
- Exposes `ConfigManifest&` directly from ConfigManager
- OTAUpdateManager accesses it via `app_->getConfigManager().getManifest()`
- Avoids adding ConfigManifest as a direct dependency of OTAUpdateManager's header
