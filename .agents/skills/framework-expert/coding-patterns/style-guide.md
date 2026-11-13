# House Style Guide

This style guide defines the C++ naming conventions, documentation standards, and structural rules for the ESP32 App Foundation framework. It is the single house-style reference for this repository and for consumer applications.

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
| **Namespaces** | lowercase | `namespace config {}` |
| **TAG (ESP_LOG)** | `static constexpr const char*`, matching class name | `static constexpr const char* TAG = "ConfigManager";` |

> **Known legacy inconsistency:** Some modules use trailing underscore (`state_`, `app_`) instead of leading `_` prefix, and some use no prefix at all (`isAPMode`, `currentState`). The leading `_` prefix is the target convention. When auditing or touching a module, scan ALL of its members — naming violations cluster.

### Acronym Rule

Acronyms and initialisms **preserve their canonical uppercase casing** when embedded in PascalCase names (e.g., `OTAUpdateManager`, `MCPServer`, `_mcpServer`). The exception is `WiFi`, which retains its mixed casing to match the ESP32 SDK (`WifiManager`).

| Acronym | In class name | In variable name |
|---------|--------------|-----------------|
| OTA | `OTAUpdateManager` | `_otaManager` |
| MCP | `MCPServer` | `_mcpServer` |
| MD5 | `MD5Utils` | `md5Hash` |
| U8G2 | `U8G2Display` | `_u8g2Display` |
| HTTP | `HTTPClient` | `httpClient` |
| WiFi | `WifiManager` | `_wifiManager` |

**Boundary rule:** the case transition marks where an acronym ends (`OTAUpdateManager` = OTA + Update + Manager). Avoid adjacent acronyms where possible (prefer `HTTPApiClient` over `HTTPAPIClient`).

**Format-specifier exemption:** enum values that spell a canonical notation may keep it instead of PascalCase, e.g. `ClockFormat::HH_MM`, `ClockFormat::HH_MM_SS`.

### Naming Exemptions

The following repository-specific files and paths are exempt from the standard naming conventions:

| Category | Reason | Example |
|----------|--------|---------|
| **Test files** | PlatformIO requires `test_` prefix for test discovery | `test/test_unit/test_configManager/test_configManager.cpp` |
| **Auto-generated portal assets** | Generated from web filenames; changing breaks the build pipeline | `include/portal_assets/app_css.hpp` |
| **Binary/XBM assets** | Embedded image data with conventional names | `cblabs_logo.h` |
| **Third-party / SDK mock stubs** | Must match upstream header names for compatibility | `Arduino.h`, `WiFi.h`, `HTTPClient.h` (in `test/stubs/platform/`) |
| **PlatformIO structural dirs** | Framework-mandated directory names | `src/`, `lib/`, `include/`, `test/`, `modules/` |

## 2. Directory ↔ File ↔ Class Alignment

The directory, primary file, and primary class MUST all use the exact same PascalCase name. This eliminates mixed-casing in `#include` paths.

- `class ConfigManager` → `ConfigManager/ConfigManager.hpp`
- `class ITickable` → file `ITickable.hpp` (interfaces don't require their own directory)
- `class OTAUpdateManager` → `OTAUpdateManager/OTAUpdateManager.hpp`

The same PascalCase applies to library directories (`lib/DeviceCrypto/DeviceCrypto.hpp`), so `#include` paths are consistently cased:

```cpp
#include "../ConfigManager/ConfigManager.hpp"
#include "../../lib/DeviceCrypto/DeviceCrypto.hpp"
```

### Renaming for case on macOS

On a case-insensitive filesystem (`core.ignorecase=true`), a `git mv configManager ConfigManager` can leave **duplicate index entries**. Rename through the index instead:

```bash
git config core.ignorecase false
git update-index --force-remove "old/path/configManager.hpp"
hash=$(git hash-object -w "new/path/ConfigManager.hpp")
git update-index --add --cacheinfo 100644,"$hash","new/path/ConfigManager.hpp"
git config core.ignorecase true

# Verify: should print 0
git ls-files | sort -f | uniq -di | wc -l
```

## 3. Module Header Template

Every `.hpp` and `.cpp` file must begin with a standard header block. The "Exported Functions/Classes" list is critical for quick API discovery: give a one-line summary for **every** externally visible class, method, property and variable.

Classes that implement no interfaces use a flat list:

```cpp
/*
 * Exported Functions/Classes:
 * - ClassName: Class description
 *   - methodName(): Single-line description
 *   - _memberVar:   Single-line description
 * - freeFunction(): Single-line description
 */
```

### Interface-Grouped Exports

When a class implements interfaces, group the exported methods by the interface they fulfill. This makes the contracted behaviors obvious.

1. **Lifecycle / own methods first**, under descriptive headings. If a lifecycle method is also an interface override, annotate it with the interface name in brackets (e.g. `tick(): … [ITickable]`).
2. **One heading per interface**, named after the interface class; a parenthetical hint may be added when it helps.
3. **A blank comment line** between groups.
4. **One line per method** within each group.

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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

Use Doxygen-style block comments (`/** ... */`) for all public declarations. Include `@brief`, `@param`, and `@return` tags for complex methods:

```cpp
/**
 * @brief Connects to the configured WiFi network with retry logic.
 *
 * Attempts connection up to maxRetries times with exponential backoff.
 * Publishes a WiFiConnected event on success via the EventBus.
 *
 * @param ssid        Network SSID to connect to.
 * @param password    Network password (WPA2).
 * @param maxRetries  Maximum connection attempts (default: 5).
 * @return true if connection succeeded, false if all retries exhausted.
 */
bool connectToNetwork(const char* ssid, const char* password, int maxRetries = 5);
```

For simple getters/setters, a single-line `@brief` is sufficient:

```cpp
/** @brief Returns the current connection state. */
bool isConnected() const;
```

## 5. Inline Comments

### Scoped variables and constants

Module-level variables and constants (headers, or file scope in `.cpp`) get a short description on the same line or immediately above. Document each `enum class` and any non-obvious values:

```cpp
constexpr int BUFFER_SIZE = 256;    // Pre-allocated receive buffer size in bytes
static bool _initialized = false;   // Guards against double-init in setup()

/** @brief Represents the device's current operational state. */
enum class AppState : uint8_t {
    Booting,       // Hardware initialisation in progress
    Configuring,   // Awaiting user configuration via portal
    Running,       // Normal operation — data fetch + display loop
    Updating,      // OTA firmware update in progress
    Error          // Unrecoverable error — awaiting restart
};
```

### Functional flow

Functions longer than ~15 lines group their logical steps with block comments:

```cpp
void performOTAUpdate() {
    // --- Step 1: Validate firmware metadata ---
    // Fetch the release manifest and compare semantic versions.
    ...
    // --- Step 2: Download firmware binary ---
    // Stream in 4 KB chunks to avoid heap exhaustion.
    ...
    // --- Step 3: Apply and reboot ---
    ...
}
```

### Non-obvious logic

Explain any line that isn't self-explanatory — bit manipulation, hardware quirks, RTOS calls, non-obvious algorithms:

```cpp
uint8_t flags = (data >> 4) & 0x0F;           // Extract 4-bit status nibble from high byte
uint32_t timeout = 1 << (retryCount + 2);     // Exponential backoff: 4ms, 8ms, 16ms, ...
portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Yield to the higher-priority task we just unblocked
```

## 6. Native Test Stubs

Hardware and framework APIs are mocked using the **Stub-Header Architecture** in `test/stubs/platform/`.

- **Never** mock classes in module headers behind the legacy `NATIVE_BUILD` macro (replaced by `ESP_PLATFORM`). Use `#ifndef ESP_PLATFORM` when compile-time guards are necessary.
- **Always** create a matching header file (e.g., `test/stubs/platform/WiFi.h`) that provides the minimum necessary definitions for the compiler to resolve your API calls.
- Stubs are selected at compile-time via include path priority during `env:native` builds.
- When providing mock/stub definitions for native testing (under `#ifndef ESP_PLATFORM`), define the stub in **exactly one authoritative header file** and use forward declarations everywhere else. This prevents redefinition errors during the build process.

## Cheat Sheet

```
Files:        PascalCase  →  ConfigManager.hpp, ITickable.hpp
Directories:  PascalCase  →  ConfigManager/, EventBus/
Classes:      PascalCase  →  ConfigManager, EventBus
Interfaces:   I + Pascal  →  ITickable, IWebContributor
Structs:      PascalCase  →  FrameworkConfig, DisplayInfo
Enums:        PascalCase  →  enum class AppState
Enum values:  PascalCase  →  AppState::Running
Methods:      camelCase   →  getData(), isReady()
Variables:    camelCase   →  retryCount, isConnected
Members:      _camelCase  →  _retryCount, _hostname
Constants:    SCREAMING   →  MAX_RETRY_COUNT, PIN_LED
Acronyms:     CAPS        →  OTAUpdateManager, MCPServer, MD5Utils
```
