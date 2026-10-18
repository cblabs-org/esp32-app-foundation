# ADR: Config Root Path Abstraction (`ConfigRoot`)

## Context & Problem
Prior to PR-MK07 WO1, system config paths (`/etc`, `/etc/server.key`, `/etc/server.crt`, `/etc/keys.json`) and module config paths (`/mod`, `/mod/<ns>.json`) were directly hardcoded as string literals across `AuthManager`, `KeyManager`, `ConfigManager`, and `WebServerManager`.

To prepare for dual-partition and dynamic mount layouts in WO2 (where configuration files may be stored on a dedicated `config` LittleFS partition or fallback to default mount), all modules must route path queries through a centralized, layout-agnostic accessor.

## Decisions Made
1. **Static `ConfigRoot` Class**: Placed in `modules/ConfigManager/ConfigRoot.hpp` and `ConfigRoot.cpp`.
2. **Accessors**:
   - `ConfigRoot::getEtcPath()` returns current system root (`"/etc"`).
   - `ConfigRoot::getModPath()` returns current module root (`"/mod"`).
   - `ConfigRoot::getEtcFilePath(filename, buf, size)` / `getEtcFilePath(filename)` helper constructs system config file paths.
   - `ConfigRoot::getModFilePath(ns, buf, size)` / `getModFilePath(ns)` helper constructs module config file paths (`/mod/<ns>.json`).
3. **Module Refactoring**:
   - `ConfigManager::getConfigPath` delegates directly to `ConfigRoot::getModFilePath`.
   - `KeyManager` updates `KEY_MANAGER_PATH` macro in `KeyManager.hpp` to evaluate `ConfigRoot::getEtcFilePath("keys.json").c_str()`.
   - `AuthManager` replaces hardcoded `/etc` checks and `/etc/server.key` / `/etc/server.crt` strings with `ConfigRoot` methods.
   - `WebServerManager::on("/api/system/factory-reset")` erases `ConfigRoot::getModPath()` and `ConfigRoot::getEtcPath()`.

## Consequences
- Single point of control for storage path resolution across all framework modules.
- Complete readiness for WO2 dynamic mount probing without further changes in `AuthManager`, `KeyManager`, or `WebServerManager`.
