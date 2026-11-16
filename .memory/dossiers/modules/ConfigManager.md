# modules/ConfigManager

## Invariants & contracts

### [docseed-033af8d7] **`getConfigNamespace()`**: Returns a unique compile-time string literal (e.g. `"wifi"`, `"display"`, `"ota"`). Determines file location at `/mod/<namespace>.json`.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-06c6b490] **`section`:** A mutable reference to the parsed configuration. You modify it in-place.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-0e711c26] **Export (`exportAllConfig`):** Iterates all registered `IConfigurable` modules and compiles their serialized JSON objects into a single backup envelope:
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-154065b4] **`resetConfig()`**: Resets all internal settings to hardcoded factory defaults.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-1f71d475] **`/config/mod/<namespace>.json`**: Persisted configuration file for each registered `IConfigurable` module (e.g. `/config/mod/wifi.json`, `/config/mod/myapp.json`).
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-237a25fa] **Follow House-Style Documentation:** Add standard file header comment blocks and document your public methods using Doxygen-style markup.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-2b0be719] **`migrateConfig(JsonObject& section, int fromVersion)`**: Migrates an older schema in-place. Returning `true` persists the migrated format; `false` triggers a fallback to defaults.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-2e257b84] **Static Scope Only (ADR-16):** Modules MUST be declared at global or static scope, not stack-allocated in `setup()`. The ESP32 has no MMU — stack-allocated modules will silently corrupt memory or cause hard faults when the framework invokes them after `setup()` returns.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-30aee9e4] **Single Responsibility Principle (SRP)**: Every class, module, or function should have one, and only one, reason to change.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-30f865e4] **`migrateConfig(JsonObject& section, int fromVersion)`**: Invoked when stored `_v` is older than `getSchemaVersion()`. Modules modify the JSON object in-place (renaming keys, adding new defaults). Returning `true` persists the migrated format; `false` falls back to defaults.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-350addae] *Recommendation*: Move `globalStateMode` into `ConfigManager` and inject it where needed.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-397ed59a] **Dependency Injection & Framework Registrations:** Wire dependencies and register core modules (`WifiManager`, `AuthManager`, `ConfigManager`, `KeyManager`, `WebServerManager`, `PreconditionWebContributor`, `DataSourceWebContributor`, `AuditModule`, WiFi preconditions).
`invariant · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp, modules/AuthManager/AuthManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/KeyManager/KeyManager.hpp, modules/WebServer/WebServer.hpp, modules/AuditModule/AuditModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4364fa0f] **OQ-8 — Can `CG-12`'s runtime-settable damping actually reach a portal?** PowerMonitor reports that `ConfigManager` has no generic per-namespace write, so "runtime-configurable" may not be implementable as drafted without work in `ConfigManager` first.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-4384cb27] **Use Interfaces (`i`-prefixed abstract classes)**: Define contracts for behaviors. This decouples the consumer from the concrete implementation.
`contract · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-47d1f3c7] **`serializeConfig(JsonObject& section) const`**: Serializes current internal state into the JSON object. `ConfigManager` automatically injects the metadata field `_v`.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4b995d99] **Law of Demeter (Principle of Least Knowledge)**: A component should only talk to its immediate friends and avoid reaching through multiple objects.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-4c4c5e13] **Minimize Global State**: Avoid global variables (`extern` or globally scoped instances) whenever possible. Global state makes testing difficult and obscures dependencies. Use Dependency Injection (DI) or Singleton patterns selectively instead of floating global objects.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-4ef3ff97] **Dependency Injection**: Pass dependencies into constructors or `begin()` methods rather than having components instantiate their own dependencies.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-55b9d7ac] **Implement `#ifdef ESP_PLATFORM` Guards:** Ensure your module compiles on host CPUs by mocking out target-specific hardware headers (e.g. wire drivers, ESP32 specific registers).
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-5710428f] **Return Value:** Return `true` if migration succeeded. Return `false` to abort the migration, which forces a fallback to factory defaults (calls `resetConfig()`).
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-5b6a4a6e] **`getConfigNamespace()`**: Returns a unique, lowercase compile-time string literal (e.g. `"wifi"`, `"display"`, `"ota"`). Determines the filename at `/mod/<namespace>.json`.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-5f41805f] **`getSchemaVersion()`**: Returns an integer schema version (starting at 1). Incremented whenever configuration keys or types change.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-61dd9e3c] Manages its own configuration persistence via `ConfigManager` under `/mod/<namespace>.json` (never touching LittleFS directly)
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-6447b439] **File Read:** `ConfigManager` loads `/mod/<namespace>.json`. If missing, it invokes `resetConfig()`, serializes defaults, and writes the initial file.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-6a53eb00] **`importAllConfig()`**: Parses an incoming JSON document, routes the appropriate JSON object to each registered module's `applyConfig()` method, and immediately persists the new state via `saveModuleConfig()`. Unknown top-level keys (e.g. `_meta`) that do not match any registered module namespace are silently skipped.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-6ba95ac0] **Immutable State**: If an object does not need to change state after creation, initialize it via the constructor and do not expose setters.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-6df301de] **`applyConfig(const JsonObject& section)`**: Deserializes configuration into internal state during boot (`beginAll()`) or portal configuration imports. The JSON object is guaranteed to be complete.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-6e60f7c9] The ArduinoJson v7 `|` operator provides a safe fallback value when a key is missing or `null`. Always prefer `config["key"] | defaultValue` over `.as<T>()`, which can crash on missing keys.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-716da218] **Serializing Configuration**: When settings change, the application or web portal triggers a save. `ConfigManager` calls `serializeConfig(section)`, appends the schema version `_v`, and performs an atomic filesystem write.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-71ca1e47] **Implicit memory leaks in `tick()`:** Doing `JsonObject obj = doc.to<JsonObject>()` repeatedly inside `tick()` without clearing memory can lead to out-of-memory crashes.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-7270f43b] **Storage Location:** Each module receives a dedicated configuration file at `/mod/<namespace>.json` (where `<namespace>` is the unique string returned by `getConfigNamespace()`).
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-730c952c] **Schema Migration**: When firmware upgrades increase `getSchemaVersion()`, `ConfigManager` detects `stored_v < getSchemaVersion()` and calls `migrateConfig(section, stored_v)`, allowing the module to upgrade legacy fields in-place before `applyConfig()` is invoked.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-792dbf35] **`exportAllConfig()`**: Iterates through all registered `IConfigurable` modules, calling their `serializeConfig()` methods to build a complete JSON tree. Used by the Web Portal to generate a downloadable backup of the device state. Each module's config is nested under its namespace key, with a `_meta` envelope containing device metadata.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-79e4ee14] **Dependency Inversion Principle (DIP)**: High-level modules should not depend on low-level modules. Both should depend on abstractions (e.g., interfaces). Abstractions should not depend on details. Details should depend on abstractions.
`contract · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-853d88fa] **Clear Interfaces**: Public methods should define *what* the object does, not *how* it does it.
`contract · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-87bf075f] **`serializeConfig(JsonObject& section)`**: Serializes current configuration state into the JSON object. `ConfigManager` injects the metadata field `_v` automatically.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-88a39673] **Applying Configuration**: On boot and configuration imports, `ConfigManager` parses `/mod/<namespace>.json` into an ArduinoJson v7 `JsonObject` and calls `applyConfig(section)`.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-88ec11c8] **Data Hiding**: All member variables should be `private` or `protected`. State changes should only occur through clearly defined public methods.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-89caba26] *Recommendation*: Extract networking logic into a new `NetworkManager` class.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-89fa130f] **Pure Interfaces:** Polymorphic contracts (`ITickable`, `IConfigurable`, `IStatusReporter`, `IWebContributor`, `IMCPContributor`).
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IStatusReporter.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8e01df76] Implement `IDisplayModule` alongside standard framework interfaces (`ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`).
`contract · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp, include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-9209944c] *Issue*: `int globalStateMode` is exposed globally.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-932523b5] `IConfigurable` → `registerConfigurable(module)`
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9c94b1f4] `manifestVersion: 1` — integer schema version, consistent with `IConfigurable::getSchemaVersion()` convention
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-9d4553dd] Do NOT add module-specific settings to `FrameworkConfig`. That struct is reserved for core framework boot-time secrets. Modules manage their own runtime configuration via `IConfigurable`.
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-a320ff5a] **Adhere to the 10ms Budget:** Profile your `tick()` callback to ensure it returns within 10ms.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-beaa26d2] **File Structure:** The configuration files are stored in JSON format. The framework manages a metadata key `_v` inside the JSON object to track the schema version of the stored data.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-c1e67922] **`getSchemaVersion()`**: Returns the integer schema version (starting at 1). Incremented when configuration key structures change.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c5038d55] **5 Framework Interfaces**: Implements `ITickable`, `IConfigurable`, `IStatusReporter`, `IWebContributor`, and `IMCPContributor`.
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IStatusReporter.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/IMCPContributor.hpp, modules/MCPServer/MCPServer.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-c8a249cd] **Access Isolation:** Modules do NOT perform file system operations. ConfigManager is the sole manager of file streams, JSON serialization, and deserialization.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-ce998b48] **`resetConfig()`**: Resets all module settings to hardcoded factory defaults. Called before factory resets or when configuration files are missing or corrupt.
`invariant · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-cef278de] **`applyConfig(const JsonObject& section)`**: Deserializes JSON settings into internal state. Called at boot (`beginAll()`), after mutations (`saveModuleConfig()`), or upon backup restoration.
`contract · kind: semantic · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-d53cb8da] **Avoid Runtime Heap Allocation:** Avoid using `new`, `std::vector::push_back`, or Arduino `String` manipulation inside `tick()`. Allocate all memory upfront during initialization or on the stack to prevent heap fragmentation.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-d67976dd] **Open/Closed Principle (OCP)**: Software entities should be open for extension but closed for modification. Use interfaces and polymorphism to allow new features without changing existing code.
`contract · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-df4658a7] Not a runtime configuration mechanism. Anything tunable at runtime stays in `ConfigManager`/NVS. **Test:** *if it can change without a soldering iron, it does not belong here.*
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-e3293d2c] **Never Use `delay()`:** Using `delay()` blocks the entire thread, halting WiFi operations, display rendering, web services, and other modules. Use the `currentMillis` comparison pattern instead.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-e4fef347] **`fromVersion`:** The version number of the stored config file.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-e771cd00] `ITickable`, `IConfigurable`, `IDataSource`, `IStatusReporter`
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-e9e5a7ea] **Flash Wear:** Flash memory has a limited write endurance (~100K cycles per sector). Modules should batch configuration changes where possible and only call `saveModuleConfig()` when a user explicitly alters a setting. Do NOT call it on every `tick()` cycle.
`invariant · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-ec55056a] **Decouple Hardware from Logic**: Separate hardware-specific calls from business logic.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-efe7bb2a] **Schema Versioning**: `getSchemaVersion()` declares the active schema integer (starts at 1). `ConfigManager` automatically embeds this as a metadata field `"_v"` in the JSON payload.
`contract · kind: semantic · anchors: modules/ConfigManager/ConfigManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f3e81829] WiFi passwords are stored via `KeyManager`, not via `IConfigurable`. `WifiManager` retrieves them via `KeyManager` during `applyConfig()`.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-f7249319] **Namespace Configs:** Keep your configuration keys short and group them under a unique, lowercase namespace (e.g., `wifi`, `clock`, `myapp`).
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-f75835bc] **Exposing hardware code in native builds:** Failing to wrap `#include <Wire.h>` or hardware-dependent drivers in `#ifdef ESP_PLATFORM` will cause host test builds to fail to compile.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-fe0122d8] *Issue*: `moduleX.cpp` handles both networking and display logic.
`invariant · kind: semantic · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

## Ruled out, failure signatures and gotchas

### [docseed-01335f52] Reaches the `tick()` loop without panicking.
`failure-signature · kind: procedural · anchors: lib/ConfigManifest/ConfigManifest.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/config-system.md · evidence: doc:docs/developer/config-system.md`

### [docseed-5951c3bc] **`begin()` Injection:** For framework-level dependencies (like the `EventBus` or `ConfigManager`) that are resolved during the application bootstrap phase.
`gotcha · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-d6f3ed75] **Stack-allocating modules (no MMU on ESP32):** Declaring a module as a local variable inside `setup()` creates a dangling pointer. The ESP32 has no MMU to trap this — it will silently corrupt memory or hard-fault when the framework calls the module's `tick()`. Always declare modules at global or `static` scope.
`gotcha · kind: semantic · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

## Procedures / Recipes

### [docseed-17600b4c] **Abstraction & Interfaces**: What interfaces need to be created?
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-2a4f524d] **Modular Structure & Responsibilities**: What should be extracted?
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-3c5d29c1] Missing interfaces for swappable components.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-5616217d] Ensure the code remains compilable after each logical step. If you remove global variables, ensure they are properly passed into the dependent objects.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-585b27ed] Update the bootstrapping code to use Dependency Injection.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-5e07c6bc] Overcrowded files or classes violating SRP.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-6321136f] Disorganized global state or excessive `extern` variables.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-718c2658] Step 2 — Add the Framework as a Submodule: `git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git framework`
`recipe · kind: procedural · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: .agents/skills/framework-setup/SKILL.md · evidence: doc:.agents/skills/framework-setup/SKILL.md`

### [docseed-801e3339] 4.3 Channel Configuration: `curl -X POST http://<device-ip>/api/ota \`
`recipe · kind: procedural · anchors: include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-86983b76] Direct instantiation of complex dependencies instead of DI.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-9ab44779] **Encapsulation & Global State**: Which globals can be grouped or injected?
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-9cf52f18] Shift global state into localized configurations or manager classes.
`recipe · kind: procedural · anchors: include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp, modules/ConfigManager/ConfigManager.hpp · source_doc: .agents/skills/oo-expert/SKILL.md · evidence: doc:.agents/skills/oo-expert/SKILL.md`

### [docseed-a16da2f0] 4d — Create `.claude` symlink (Claude Code support): `ln -s .agents .claude` (`grep -qxF '.claude' .gitignore 2>/dev/null || echo '.claude' >> .gitignore`)
`recipe · kind: procedural · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: .agents/skills/framework-setup/SKILL.md · evidence: doc:.agents/skills/framework-setup/SKILL.md`

### [docseed-ff810651] 4a — Create baseline rules: `mkdir -p .agents/rules`
`recipe · kind: procedural · anchors: include/interfaces/ITickable.hpp, include/interfaces/IConfigurable.hpp, modules/ConfigManager/ConfigManager.hpp, include/interfaces/IWebContributor.hpp · source_doc: .agents/skills/framework-setup/SKILL.md · evidence: doc:.agents/skills/framework-setup/SKILL.md`
