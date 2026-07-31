# Graveyard: Linker & Header Resolution Pitfalls (WO3)

## Issue 1: `DeviceCrypto.hpp` missing include path via `KeyManager.hpp`

- **Symptom:**
  ```
  ./modules/Application/../KeyManager/KeyManager.hpp:49:10: fatal error: '../DeviceCrypto/DeviceCrypto.hpp' file not found
  ```
- **Root Cause:**
  `KeyManager.hpp` had `#include "../DeviceCrypto/DeviceCrypto.hpp"`. When `DeviceCrypto` was migrated to `lib/DeviceCrypto/`, `Application.hpp` was updated to `#include "../../lib/DeviceCrypto/DeviceCrypto.hpp"`, but `KeyManager.hpp` retained the old relative path. When `Application.hpp` was included in standalone builds, `KeyManager.hpp` caused compilation to fail.
- **Fix:** Update `KeyManager.hpp` line 49 to `#include "../../lib/DeviceCrypto/DeviceCrypto.hpp"`.

## Issue 2: PlatformIO LDF library ignoring `InputModule` due to `Application` dependency

- **Symptom:**
  ```
  Undefined symbols for architecture arm64:
    "InputModule::registerMCPTools(MCPToolRegistry&)"
    "InputModule::tick(unsigned int)"
  ```
- **Root Cause:**
  `modules/InputModule/library.json` declared `"dependencies": { "Application": "*" }`. Because `Application` is listed in `platformio.ini` `lib_ignore`, PlatformIO LDF automatically marked `InputModule` as ignored too, so `InputModule.cpp` was skipped during compilation.
- **Fix:** Removed `"Application": "*"` from `modules/InputModule/library.json`. `InputModule` only forward-declares `Application` and does not link against the `Application` library.

## Issue 3: ArduinoJson `containsKey` Deprecation Warning

- **Symptom:**
  ```
  warning: 'containsKey' is deprecated [-Wdeprecated-declarations]
  ```
- **Root Cause:**
  ArduinoJson v7 deprecates `obj.containsKey("key")` in favor of `obj["key"].is<T>()` or `!obj["key"].isNull()`.
- **Fix:** Used `result["inputs"].is<JsonArray>()` in `test_InputModule.cpp`.
