# Creating a Module Reference

## Interface Selection Guide

Inherit only from the interfaces your custom module requires:

| Requirement | Interface to Inherit |
| :--- | :--- |
| Periodic non-blocking main-loop execution | `ITickable` |
| Save/load configuration settings from flash | `IConfigurable` |
| Register REST routes or UI tabs in Web Portal | `IWebContributor` |
| Register JSON-RPC tools with local MCP Server | `IMCPContributor` |
| Block boot transition to RUNNING state | `IConfigGate` |
| Periodically pull remote API data in background | `IDataSource` |

## Class Declaration Pattern

Forward-declare dependencies in headers to minimize compile times and prevent circular dependency loops. Always use the `override` keyword on virtual overrides.

```cpp
#pragma once

#include <cstdint>
#include <ArduinoJson.h>
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"

class MyApp : public ITickable, public IConfigurable {
public:
    MyApp() = default;
    ~MyApp() override = default;

    // ITickable
    void tick(uint32_t currentMillis) override;

    // IConfigurable
    const char* getConfigNamespace() const override { return "myapp"; }
    int getSchemaVersion() const override { return 1; }
    void applyConfig(const JsonObject& config) override;
    void serializeConfig(JsonObject& config) const override;
    void resetConfig() override;

private:
    int _intervalMs = 5000;
    uint32_t _lastTickMs = 0;
};
```

## Universal Module Registration
Instantiate modules in global scope (or heap allocate during setup before boot) and register them:
```cpp
#include "AppFoundation.h"
#include "myApp.hpp"

Application app;
MyApp myApp;

void setup() {
    app.registerModule(&myApp); // Registers for ITickable, IConfigurable, etc.
    FrameworkConfig config;
    app.begin(config);
}

void loop() {
    app.tick();
}
```

## The Tick Callback Contract
*   **Signature:** `void tick(uint32_t currentMillis) override`
*   **Execution Budget:** Must return in **10 milliseconds or less**. Defer heavy computation or blocking operations (e.g., sockets, sensors) to FreeRTOS tasks or `IDataSource`.
*   **Time Parameter:** Use the passed `currentMillis` instead of calling `millis()` to ensure host testability.
*   **Throttled Executions:** Use unsigned arithmetic to safely check elapsed time (prevents overflow issues):
```cpp
void MyApp::tick(uint32_t currentMillis) {
    if (currentMillis - _lastTickMs >= (uint32_t)_intervalMs) {
        _lastTickMs = currentMillis;
        // Run throttled logic
    }
}
```

## Compilation Guards for Host Testing
Mock out hardware-specific headers and SDKs using `#ifndef NATIVE_BUILD` to allow tests to compile and run on host machines (macOS/Linux).
```cpp
#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif
```

## Module Development Checklist
*   [ ] **Never call `delay()`:** Defer or throttle executions using the tick comparison pattern.
*   [ ] **Avoid heap allocations in `tick()`:** Allocate objects and buffers upfront (e.g. static, class member, or pre-allocated stack objects). Avoid `new` and Arduino `String` allocations inside the tick loop.
*   [ ] **Mock out hardware:** Wrap register writes, I2C, SPI, or ESP32-specific SDK code in `#ifndef NATIVE_BUILD`.
*   [ ] **Doxygen comments:** Maintain house-style header and method comments.

## Common Mistakes to Avoid
1.  **Late Registration:** Calling `app.registerModule()` after `app.begin()` will fail because the registry locks at boot.
2.  **Duplicate Registration:** Calling both `app.registerTickable(&module)` and `app.registerModule(&module)`. The universal `registerModule()` handles everything.
3.  **Leaking in Tick:** Performing repeated JSON doc allocation or manipulation without resetting/scoping correctly.
