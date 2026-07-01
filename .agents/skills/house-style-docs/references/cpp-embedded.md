# C++ Embedded (ESP32/Arduino) — House Style Reference

This reference file contains all language-specific documentation patterns, code examples, and naming conventions for embedded C++ projects targeting the ESP32 platform with PlatformIO.

---

## 1. Module Header Template

Every `.hpp` and `.cpp` file must begin with this header block. Replace placeholders with actual project values — derive `PROJECT_NAME` from `library.json`, `platformio.ini`, or `README.md`.

```cpp
/*
 * [PROJECT_NAME] (c) [YEAR_RANGE] [AUTHOR/ORG]
 *
 * [REPO_URL]
 *
 * [LICENSE_TEXT]
 *
 * Module: [file path relative to repo root]
 * Description: [Multi-line description of the module's responsibility.
 *               What does it manage? What abstractions does it provide?]
 *
 * Exported Functions/Classes:
 * - [ClassName]: [Class description]
 *   - [methodName](): [Single-line description]
 *   - [_memberVar]: [Single-line description]
 * - [freeFunction](): [Single-line description]
 */
```

### Interface-Grouped Exports

When a class inherits from one or more abstract interface classes, the exported method list should be **grouped by interface** rather than listed flat. A flat list forces the reader to cross-reference the class declaration to work out which methods belong to which contract; grouping makes this obvious at a glance.

**Grouping principle:**
1. **Lifecycle / own methods first.** Group the class's own public API (constructors, `begin()`, own methods) under descriptive headings. If a lifecycle method is also an interface override, annotate it with the interface name in brackets at the end of the line (e.g. `[IFoo]`).
2. **One heading per interface.** Each inherited interface gets its own heading named after the interface class. Parenthetical context can be added to the heading when it aids understanding (e.g. key config values, identifiers).
3. **Blank comment lines** between groups for visual separation.
4. **One-line-per-method** format within each group — same as the flat style.

Classes that do not implement interfaces should continue to use the flat list format shown in the base template above.

```cpp
/*
 * Exported Classes:
 * - MyModule: [Class description]
 *
 *   Lifecycle:
 *   - begin():       [description]
 *   - tick():        [description]  [IFoo]
 *
 *   [Own Public API heading]:
 *   - doSomething(): [description]
 *
 *   IBar:
 *   - barMethod():   [description]
 *   - otherMethod(): [description]
 *
 *   IBaz (some-context-hint):
 *   - bazMethod():   [description]
 */
```

> [!IMPORTANT]
> The "Exported Functions/Classes" list is critical. It allows a developer to quickly see the services offered by the module without reading the whole file. Provide a one-line summary for EVERY externalized method, property, and variable. When a class implements interfaces, group the methods by interface — a flat list obscures which contract each method fulfills.

---

## 2. Function and Method Documentation

Use Doxygen-style comment blocks for all declarations and implementations.

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

---

## 3. Scoped Variables and Constants

For variables and constants defined at module level (headers or file-scope in `.cpp`), add a short description on the same line or immediately above.

```cpp
#define MAX_RETRY_COUNT 5           // Maximum number of connection attempts before failing
constexpr int kBufferSize = 256;    // Pre-allocated receive buffer size in bytes
static bool _initialized = false;   // Guards against double-init in setup()
```

For `enum class` values, document the enum itself and any non-obvious values:

```cpp
/** @brief Represents the device's current operational state. */
enum class AppState : uint8_t {
    Booting,       // Hardware initialisation in progress
    Configuring,   // Awaiting user configuration via portal
    Running,       // Normal operation — data fetch + display loop
    Updating,      // OTA firmware update in progress
    Error          // Unrecoverable error — awaiting restart
};
```

Enum values with explicit integer assignments follow the same PascalCase rule:

```cpp
enum class TextAlign : uint8_t {
    Left   = 0,
    Center = 1,
    Right  = 2
};
```

> [!NOTE]
> **Format specifier exemption**: Enum values representing format notation or canonical specifiers may retain their established notation rather than PascalCase. For example, `ClockFormat::HH_MM` and `ClockFormat::HH_MM_SS` — these are universally recognised time format tokens and would lose clarity if converted to PascalCase.

---

## 4. Internal Functional Flow

For functions longer than ~15 lines, use block comments to group logical steps and explain the flow.

```cpp
void performOTAUpdate() {
    // --- Step 1: Validate firmware metadata ---
    // Fetch the release manifest and compare semantic versions.
    // Abort early if the remote version <= current version.
    ...

    // --- Step 2: Download firmware binary ---
    // Stream the binary in 4KB chunks to avoid heap exhaustion.
    // Each chunk is verified via running MD5 checksum.
    ...

    // --- Step 3: Apply and reboot ---
    // Write to the inactive OTA partition, verify the image,
    // then set the boot partition and trigger a controlled restart.
    ...
}
```

---

## 5. Arcane and Complex Logic

Identify lines of code that are not self-explanatory and provide an explanation.

```cpp
uint8_t flags = (data >> 4) & 0x0F;         // Extract 4-bit status nibble from high byte
uint32_t timeout = 1 << (retryCount + 2);   // Exponential backoff: 4ms, 8ms, 16ms, ...
portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // FreeRTOS: yield to unblocked higher-priority task
```

---

## 6. Naming Conventions

### Type and Symbol Names

| Category | Convention | Example |
|----------|-----------|---------| 
| **Classes** | PascalCase | `ConfigManager`, `ClockWidget`, `EventBus` |
| **Interfaces** | PascalCase with `I` prefix | `ITickable`, `IConfigurable`, `IWebContributor` |
| **Structs** | PascalCase | `FrameworkConfig`, `DisplayInfo`, `OTAProgressEvent` |
| **Enums** | `enum class`, PascalCase | `enum class AppState`, `enum class OTAError` |
| **Enum values** | PascalCase | `AppState::Running`, `OTAError::ChecksumMismatch` |
| **Methods / Functions** | camelCase | `void getData()`, `bool isReady()` |
| **Variables** | camelCase | `int retryCount`, `bool isConnected` |
| **Member variables** | camelCase with `_` prefix | `int _retryCount`, `String _hostname` |
| **Constants / Macros** | SCREAMING_SNAKE_CASE | `MAX_RETRY_COUNT`, `PIN_LED`, `FIRMWARE_VERSION` |
| **Namespaces** | lowercase | `namespace config {}` |

### Acronym and Initialism Rule

Acronyms and initialisms **preserve their canonical uppercase casing** when embedded in PascalCase names. This matches the Microsoft/Apple convention and reads naturally.

| Acronym | In class name | In variable name |
|---------|--------------|-----------------|
| OTA | `OTAUpdateManager` | `_otaManager` |
| MCP | `MCPServer` | `_mcpServer` |
| MD5 | `MD5Utils` | `md5Hash` |
| U8G2 | `U8G2DisplayModule` | `_u8g2Display` |
| HTTP | `HTTPClient` | `httpClient` |
| WiFi | `WifiManager`* | `_wifiManager` |

> [!NOTE]
> **WiFi exception**: `Wifi` is retained as PascalCase (not `WIFI`) because `WiFi` is a brand name with established mixed casing, and the ESP32 SDK uses `WiFi` throughout. Matching the SDK avoids confusion.

**Boundary rule**: When an acronym is followed by another word, the boundary is clear from the case transition: `OTAUpdateManager` (OTA + Update + Manager). Avoid adjacent acronyms where possible (e.g. prefer `HTTPApiClient` over `HTTPAPIClient`).

### File and Directory Names

| Category | Convention | Example |
|----------|-----------|---------| 
| **Source files** | PascalCase, matching primary class | `ConfigManager.hpp`, `EventBus.cpp` |
| **Interface files** | PascalCase, matching interface name | `ITickable.hpp`, `IConfigurable.hpp` |
| **Module directories** | PascalCase, matching module name | `ConfigManager/`, `OTAUpdateManager/` |
| **Library directories** | PascalCase, matching library name | `DeviceCrypto/`, `OTADownloadEngine/` |

> [!IMPORTANT]
> **Directory ↔ File ↔ Class Alignment Rule**: The directory, primary file, and primary class MUST all use the same PascalCase name.
> - `class ConfigManager` → `ConfigManager/ConfigManager.hpp`
> - `class ITickable` → file `ITickable.hpp` (interface files don't require their own directory)
> - `class OTAUpdateManager` → `OTAUpdateManager/OTAUpdateManager.hpp`
>
> This eliminates mixed-casing in `#include` paths and follows the LLVM/Unreal convention.

### `#include` Path Convention

With PascalCase directories, `#include` paths are consistently cased throughout:

```cpp
#include "../ConfigManager/ConfigManager.hpp"     // ✅ Consistent PascalCase
#include "../../lib/DeviceCrypto/DeviceCrypto.hpp" // ✅ Consistent PascalCase
```

### Naming Exemptions

The following are exempt from the naming conventions above:

| Category | Reason | Example |
|----------|--------|---------|
| **Test files** | PlatformIO requires `test_` prefix for test discovery | `test/test_unit/test_configManager/test_configManager.cpp` |
| **Auto-generated portal assets** | Generated from web filenames; changing breaks the build pipeline | `include/portal_assets/app_css.hpp` |
| **Binary/XBM assets** | Embedded image data with conventional names | `cblabs_logo.h` |
| **Third-party / SDK mock stubs** | Must match upstream header names for compatibility | `Arduino.h`, `WiFi.h`, `HTTPClient.h` |
| **PlatformIO structural dirs** | Framework-mandated names | `src/`, `lib/`, `include/`, `test/`, `modules/` |

### Platform Test Stubs

Hardware and framework APIs are mocked using the **Stub-Header Architecture** in `test/stubs/platform/`.

- **Never** use `#ifdef NATIVE_BUILD` to mock classes in your module headers.
- **Always** create a matching header file (e.g., `test/stubs/platform/WiFi.h`) that provides the minimum necessary definitions for the compiler to resolve your API calls.
- Stubs are selected at compile-time via include path priority during `env:native` builds.

### Git Case-Sensitivity on macOS

On macOS (case-insensitive filesystem), renaming files or directories from `configManager` to `ConfigManager` requires special handling. The standard `git mv` two-step process creates **duplicate index entries** when `core.ignorecase=true`.

**Reliable approach — `git update-index`:**

```bash
# 1. Temporarily enable case sensitivity
git config core.ignorecase false

# 2. Remove old entry, add new entry with correct casing
git update-index --force-remove "old/path/configManager.hpp"
hash=$(git hash-object -w "new/path/ConfigManager.hpp")
git update-index --add --cacheinfo 100644,"$hash","new/path/ConfigManager.hpp"

# 3. Restore default
git config core.ignorecase true
```

**Always verify** after bulk renames:
```bash
# Check for duplicate entries (should be 0)
git ls-files | sort -f | uniq -di | wc -l
```

---

## Summary Cheat Sheet

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
Acronyms:     CAPS        →  OTAManager, MCPServer, MD5Utils
```
