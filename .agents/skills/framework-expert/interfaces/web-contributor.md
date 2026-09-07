# IWebContributor

Contract for modules that expose REST API routes and navigation tabs to the `WebServerManager`.

## Overview

The web server calls `registerRoutes()` once during `begin()` to register REST API endpoints.
Modules also contribute navigation tab metadata via the `registerTabs(ITabRegistry&)` visitor
pattern — the same approach used by `IMCPContributor` and `IPreconditionContributor`.

```cpp
class IWebContributor {
public:
    virtual ~IWebContributor() = default;

    // REST API — route registration
    virtual void registerRoutes(PsychicHttpServer& server) = 0;

    // UI Integration — tab metadata via registry/visitor pattern
    virtual void registerTabs(ITabRegistry& registry) = 0;

    // Body-limit middleware hints (1:1 per contributor)
    virtual const char* getApiPrefix() const { return nullptr; }
    virtual size_t getMaxPostBody() const { return 4096; }
};
```

## Registering Routes

Routes are registered directly against the `PsychicHttpServer`.

```cpp
void MyModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/my-module/status", HTTP_GET, [this](PsychicRequest* request) {
        JsonDocument doc;
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

Override `registerTabs()` and call `registry.addTab()` for each tab your module contributes.
A single module can register multiple tabs in one call — no inner stubs needed.

```cpp
void MyModule::registerTabs(ITabRegistry& registry) {
    // Top-level tab with custom icon and JavaScript
    registry.addTab("Weather", nullptr, 50, kWeatherIcon, "/weather.js");
}
```

Multi-tab modules register all their tabs in a single override:

```cpp
void AuthManager::registerTabs(ITabRegistry& registry) {
    registry.addTab("Security", nullptr, 5, getTabIcon()); // parent tab
    registry.addTab("Password", "Security", 10);            // child tab
    registry.addTab("TLS",      "Security", 20);            // child tab
}
```

### addTab() Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | `const char*` | required | Display name for the tab (static lifetime) |
| `parent` | `const char*` | `nullptr` | Parent tab name for sub-navigation. `nullptr` = top-level (bottom nav) |
| `order` | `int` | `100` | Sort order within the same hierarchy level (lower = first) |
| `icon` | `const char*` | `nullptr` | SVG inner content for bottom-nav icon. `nullptr` = generic grid icon |
| `script` | `const char*` | `nullptr` | Route path for tab JavaScript file (must start with `/`) |

> [!IMPORTANT]
> All `const char*` parameters must point to storage with static or application lifetime — the registry stores pointers, not copies. String literals and `const char*` class members are fine. Do NOT pass `std::string::c_str()`.

### Headless API-Only Contributors

If your module registers routes but has no visible tab, provide an empty `registerTabs()`:

```cpp
void PreconditionWebContributor::registerTabs(ITabRegistry& registry) {
    // API-only — no visible tab.
}
```

## Native Testing Stubs

PsychicHttp and PsychicHttpsServer are mocked via the framework's **Stub-Header Architecture** in `test/stubs/platform/`.

Because the build system maps `#include <PsychicHttp.h>` to the local mock during `env:native` builds, you do **not** need to wrap your `server.on()` declarations in `#ifndef ESP_PLATFORM` guards.

Use the shared `MockTabRegistry` (`test/test_unit/test_helpers/MockTabRegistry.hpp`) to verify tab registration in unit tests:

```cpp
#include "test_helpers/MockTabRegistry.hpp"

TEST_F(MyModuleTest, RegisterTabs) {
    MockTabRegistry registry;
    module.registerTabs(registry);
    ASSERT_EQ(registry.entries.size(), 1u);
    EXPECT_EQ(registry.entries[0].name, "Weather");
    EXPECT_EQ(registry.entries[0].parent, "");
    EXPECT_EQ(registry.entries[0].order, 50);
}
```
