# House Style Guide

This style guide defines the C++ naming conventions, documentation standards, and structural rules for the ESP32 App Foundation framework.

## 1. Naming Conventions

We follow a strict naming scheme to ensure consistency across the codebase:

| Category | Convention | Example |
|----------|-----------|---------| 
| **Classes / Structs** | PascalCase | `ConfigManager`, `FrameworkConfig` |
| **Interfaces** | PascalCase with `I` prefix | `ITickable`, `IWebContributor` |
| **Enums** | `enum class`, PascalCase | `enum class AppState` |
| **Enum values** | PascalCase | `AppState::Running` |
| **Methods / Functions** | camelCase | `void getData()`, `bool isReady()` |
| **Variables** | camelCase | `int retryCount` |
| **Member variables** | camelCase with `_` prefix | `int _retryCount` |
| **Constants / Macros** | SCREAMING_SNAKE_CASE | `MAX_RETRY_COUNT` |
| **TAG (ESP_LOG)** | `static constexpr const char*`, matching class name | `static constexpr const char* TAG = "ConfigManager";` |

> **Known legacy inconsistency:** Some modules use trailing underscore (`state_`, `app_`) instead of leading `_` prefix, and some use no prefix at all (`isAPMode`, `currentState`). The leading `_` prefix is the target convention. When auditing or touching a module, scan ALL of its members — naming violations cluster.

### Acronym Rule

Acronyms and initialisms **preserve their canonical uppercase casing** when embedded in PascalCase names (e.g., `OTAUpdateManager`, `MCPServer`, `_mcpServer`). The exception is `WiFi`, which retains its mixed casing to match the ESP32 SDK (`WifiManager`).

## 2. Directory ↔ File ↔ Class Alignment

The directory, primary file, and primary class MUST all use the exact same PascalCase name. This eliminates mixed-casing in `#include` paths.

- `class ConfigManager` → `ConfigManager/ConfigManager.hpp`
- `class ITickable` → file `ITickable.hpp` (interfaces don't require their own directory)

## 3. Module Header Template

Every `.hpp` and `.cpp` file must begin with a standard header block. The "Exported Functions/Classes" list is critical for quick API discovery.

### Interface-Grouped Exports

When a class implements interfaces, group the exported methods by the interface they fulfill. This makes the contracted behaviors obvious.

```cpp
/*
 * ESP32 App Foundation (c) 2026
 *
 * Module: modules/MyModule/MyModule.hpp
 * Description: Manages the custom sensor hardware and reports metrics.
 *
 * Exported Classes:
 * - MyModule: Main sensor controller
 *
 *   Lifecycle:
 *   - begin():       Initializes sensor hardware
 *
 *   Own Public API:
 *   - readSensor():  Triggers a manual read
 *
 *   ITickable:
 *   - tick():        Polls sensor data
 *
 *   IConfigurable:
 *   - applyConfig(): Applies calibration offsets
 */
```

## 4. Doxygen Documentation

Use Doxygen-style block comments (`/** ... */`) for all public declarations. Include `@brief`, `@param`, and `@return` tags for complex methods. For simple getters/setters, a single-line `@brief` is sufficient.

## 5. NATIVE_BUILD Stubs

When providing mock/stub definitions for native testing (under `#ifdef NATIVE_BUILD`), define the stub in **exactly one authoritative header file** and use forward declarations everywhere else. This prevents redefinition errors during the build process.
