# IWebContributor

Contract for modules that expose REST API routes to the `WebServerManager`.

## Overview

The web server calls `registerRoutes()` once during `begin()`. Modules also provide navigation hierarchy metadata to inject themselves into the web portal UI.

```cpp
class IWebContributor {
public:
    virtual ~IWebContributor() = default;
    virtual void registerRoutes(PsychicHttpServer& server) = 0;
    
    // UI Integration
    virtual const char* getTabName() const { return nullptr; }
    virtual const char* getParentTab() const { return nullptr; }
    virtual int getTabOrder() const { return 100; }
    virtual const char* getTabIcon() const { return nullptr; }
};
```

## Registering Routes

Routes are registered directly against the `PsychicHttpServer`.

```cpp
void MyModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/my-module/status", HTTP_GET, [this](PsychicRequest* request) {
        StaticJsonDocument<256> doc; // Or JsonDocument in v7
        doc["status"] = "ok";
        doc["uptime"] = _uptime;
        
        String response;
        serializeJson(doc, response);
        return request->reply(200, "application/json", response.c_str());
    });
}
```

> [!TIP]
> Auth enforcement for `/api/` paths is handled centrally by the global auth middleware in `WebServerManager`. You do NOT need to check auth or use guards in your handlers.

## UI Integration (Tabs & Sub-Tabs)

- **`getTabName()`**: The display name for the tab (e.g., `"Network"`). Return `nullptr` to be a headless API.
- **`getParentTab()`**: The parent grouping. Return `nullptr` to be a top-level tab (bottom nav). Return a string (e.g., `"System"`) to be a sub-tab nested under that section.
- **`getTabOrder()`**: Controls display order within the level. Lower is first.
- **`getTabIcon()`**: Return the SVG inner content (e.g., `<path d="..." />`) for the bottom-nav icon. Return `nullptr` to use a generic grid icon.

## Native Testing Stubs

PsychicHttp and PsychicHttpsServer are mocked via the framework's **Stub-Header Architecture** in `test/stubs/platform/`.

Because the build system maps `#include <PsychicHttp.h>` to the local mock during `env:native` builds, you do **not** need to wrap your `server.on()` declarations in `#ifndef NATIVE_BUILD` guards.
