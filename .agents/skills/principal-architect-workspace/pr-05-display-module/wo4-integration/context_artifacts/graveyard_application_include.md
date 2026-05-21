# Graveyard: Application.hpp Include in u8g2DisplayModule.cpp

## CRITICAL — Future agents MUST read this before touching the display module includes

### Problem

The `begin()` method needs to call `_app->getEventBus().subscribe(...)` which requires the full `Application` class definition (not just a forward declaration). The natural approach is:

```cpp
#include "../application/Application.hpp"
```

### What Failed

#### Attempt 1: Include behind `#ifndef NATIVE_BUILD`

```cpp
#ifndef NATIVE_BUILD
#include "../application/Application.hpp"
#endif
```

**Result**: ❌ FAILED. PlatformIO's LDF (Library Dependency Finder) scanner reads ALL `#include` directives **regardless of preprocessor guards**. It does NOT evaluate `#ifndef`, `#ifdef`, `#if defined(...)`, or any other conditional compilation directives. The scanner uses a simple regex/text-based approach.

This caused PlatformIO to resolve the `application` module as a dependency, which transitively pulled in `webServer`, which includes `portal_assets/index_html.hpp`, which includes `<Arduino.h>` — breaking the native build.

**Error**: `fatal error: 'Arduino.h' file not found` in `webServer.cpp`

#### Attempt 2: Include with `#else` stub

```cpp
#ifndef NATIVE_BUILD
#include "../application/Application.hpp"
#else
// forward declaration only
#endif
```

**Result**: ❌ FAILED. Same issue — LDF still sees the include directive.

#### Attempt 3: Move include to EventBus.hpp only

```cpp
#include "../eventBus/EventBus.hpp"
// Plus Application.hpp inside #ifndef NATIVE_BUILD
```

**Result**: ❌ FAILED. Still included Application.hpp conditionally.

### What Worked

**Final solution**: Include ONLY `../eventBus/EventBus.hpp` (native-safe, no Arduino.h dependency). Do NOT include `Application.hpp` **at all** in the translation unit visible to native builds. On hardware builds, the `Application.hpp` symbols are resolved by the linker from the separately compiled `application` module.

The `begin()` method's `_app->getEventBus()` call is guarded behind `#ifndef NATIVE_BUILD`, so the compiler never needs the full Application definition on native.

```cpp
// In u8g2DisplayModule.cpp
#include "../eventBus/EventBus.hpp"
// NO Application.hpp include at all in non-guarded code

void U8g2DisplayModule::begin(Application& app) {
    _app = &app;
#ifndef NATIVE_BUILD
    // These calls need Application's full definition — 
    // resolved by linker on hardware, never compiled on native
    _app->getEventBus().subscribe("ota.progress", _onOtaProgress, this);
    _app->getEventBus().subscribe("display.message", _onDisplayMessage, this);
#endif
}
```

### Root Cause Summary

PlatformIO LDF `chain` mode (the default) scans `#include` lines as text patterns and adds all referenced libraries as dependencies. It compiles ALL `.cpp` files in those libraries. Since `webServer.cpp` includes `portal_assets/index_html.hpp` which includes `<Arduino.h>` without a `NATIVE_BUILD` guard, any module that transitively depends on `application` (which depends on `webServer`) will fail on native.

### System-Wide Fix (Not Done in WO4)

Adding a `library.json` to `modules/webServer/` with `"platforms": ["espressif32"]` would prevent LDF from compiling webServer on native at all. This would fix `test_application` and `test_webServer` native tests too (both currently ERRORED with the same issue).
