# Graveyard: Native Build Stub Issues

## String Stub Redefinition (FIXED)
**Problem**: Both `wifiManager.hpp` and `mcpServer.hpp` defined a `class String {}` stub inside `#ifdef NATIVE_BUILD` blocks. When `Application.hpp` started including `wifiManager.hpp` (for the new WifiManager member), both String classes were visible in the same translation unit → `error: redefinition of 'String'`.

**Fix**: Unified both String stubs behind a `#ifndef STRING_STUB_DEFINED` / `#define STRING_STUB_DEFINED` include guard. Both files now define the exact same String class with all methods needed by either module (operator+=, operator=, clear, length, indexOf, replace, etc.).

**Warning for future work**: Any new module that adds a String stub MUST use this same guard. Better yet, extract the String stub into a shared `platform/StringStub.h` header.

## EventBus Callback Signature Mismatch (FIXED)
**Problem**: Tried to use a capturing lambda `[this](const void*) { ... }` with `EventBus::subscribe()`. But EventBus uses raw C function pointers: `typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx)`. Capturing lambdas cannot be converted to function pointers.

**Fix**: Used the EventBus context parameter as a trampoline:
```cpp
_eventBus.subscribe("wifi.ready",
    [](const char*, const void*, void* ctx) {
        static_cast<Application*>(ctx)->_webServerManager.startServer();
    }, this);
```
The non-capturing lambda converts to a function pointer; `this` is passed as `ctx` and cast back inside.

## Mock AsyncWebServer Missing Methods (FIXED)
**Problem**: Added `startServer()` calling `_server.end()` and `onNotFound()` — both missing from the mock.

**Fix**: Added `void end() {}` and `void onNotFound(ArRequestHandlerFunction) {}` stubs plus `void redirect(const char*) {}` on `AsyncWebServerRequest`.
