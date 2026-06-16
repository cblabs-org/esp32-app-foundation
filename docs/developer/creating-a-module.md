# Creating a Module

> ESP32 App Foundation (c) 2026 CB Labs
>
> Step-by-step tutorial on authoring a custom framework module, covering interface selection, class implementation, tick-timing patterns, and registration.

---

This tutorial walks through creating a custom application module. We will use the built-in template files as our reference:
- [myApp.hpp](../../template/src/myApp.hpp)
- [myApp.cpp](../../template/src/myApp.cpp)
- [main.cpp](../../template/src/main.cpp)

---

## 1. Choosing Your Interfaces

Before writing any code, determine what your module needs to do. The framework utilizes a contract-based model. You select and inherit from only the interfaces your module requires.

Use this decision table to guide your implementation:

| If your module needs to... | ...then implement: | Key Callback |
| :--- | :--- | :--- |
| Run code periodically on every main-loop tick | **`ITickable`** | `tick(uint32_t currentMillis)` |
| Save and load configuration settings from flash | **`IConfigurable`** | `applyConfig()`, `serializeConfig()` |
| Expose REST API routes or add tabs to the Web UI | **`IWebContributor`** | `registerRoutes()`, `getTabName()` |
| Expose JSON-RPC commands to external clients / agents | **`IMCPContributor`** | `registerMCPTools()` |
| Block entering the `RUNNING` state until configured | **`IConfigGate`** | `isSatisfied()` |
| Poll remote servers periodically in the background | **`IDataSource`** | `fetchUpdate()` |
| Render a telemetry pane on the OLED display carousel | **`IDisplayBoard`** | `render(U8G2&)`, `getBoardName()` |
| Report boot-phase progress and fatal errors to the display | **`IStatusReporter`** | `reportStatus()`, `reportError()` |

---

## 2. Declaring Your Module Class

The template module `MyApp` implements five different interfaces using multiple inheritance. Here is the class declaration from [myApp.hpp](../../template/src/myApp.hpp):

```cpp
#pragma once

#include "AppFoundation.h"

class PsychicHttpServer;
class MCPServer;
class MCPToolRegistry;

/**
 * @brief Main application class implementing specific business logic.
 *
 * Participates in the Application tick loop, participates in the ConfigManager,
 * contributes Web UI tabs and routes, contributes MCP tools, and exposes a config gate.
 */
class MyApp : public ITickable,
              public IConfigurable,
              public IWebContributor,
              public IMCPContributor,
              public IConfigGate {
public:
    MyApp() = default;
    ~MyApp() override = default;

    // ITickable
    void tick(uint32_t currentMillis) override;

    // IConfigurable
    const char* getConfigNamespace() const override;
    int getSchemaVersion() const override;
    void applyConfig(const JsonObject& config) override;
    void serializeConfig(JsonObject& config) const override;
    void resetConfig() override;

    // IWebContributor
    void registerRoutes(PsychicHttpServer& server) override;
    const char* getTabName() const override;
    int getTabOrder() const override;

    // IMCPContributor
    void registerMCPTools(MCPToolRegistry& registry) override;
    void registerMcpTools(MCPServer& mcp); // Deprecated legacy method signature

    // IConfigGate
    const char* getGateName() const override;
    bool isSatisfied() const override;

private:
    // Configuration state (persisted via IConfigurable)
    int _refreshIntervalMs = 5000;     // Example configurable parameter
    bool _featureEnabled = true;        // Example boolean toggle

    // Config gate state
    bool _gateOpen = false;

    // Tick timing
    uint32_t _lastActionMs = 0;
};
```

### Key Highlights:
- **Single Include:** Use `#include "AppFoundation.h"` instead of including individual interface headers. `AppFoundation.h` transitively includes all interfaces, `FrameworkConfig`, and the `Application` hub.
- **Forward Declarations:** Use forward declarations (e.g. `class PsychicHttpServer;`) in your header file instead of full `#include` statements to reduce compile times and prevent circular dependency loops.
- **Interfaces Inheritance:** Standard multiple inheritance is used to implement multiple interfaces.
- **`override` Keyword:** Always mark inherited virtual functions with `override` to allow the compiler to detect signature mismatches.

---

## 3. Implementing the Module Logic

Let's examine how each interface is implemented in [myApp.cpp](../../template/src/myApp.cpp).

### The Tick Loop & Injected Time
The `ITickable::tick()` function is called repeatedly on every main-loop execution. To ensure testability, the current time is captured once at the beginning of the framework tick cycle and passed to you as `currentMillis`. 

> [!IMPORTANT]
> **10ms Execution Budget:** Your `tick()` implementation MUST execute quickly and return in **10 milliseconds or less**. If your module performs blocking operations (like raw socket reads, heavy math, or slow sensor queries), defer them to a FreeRTOS task or use the asynchronous background `DataManager` via `IDataSource`.

Here is the non-blocking timing pattern from the template:

```cpp
void MyApp::tick(uint32_t currentMillis) {
    // --- Step 1: Throttled Periodic Execution ---
    // Check if the refresh interval has elapsed using unsigned subtraction.
    // This arithmetic remains correct even when millis() overflows and wraps to 0.
    if (currentMillis - _lastActionMs >= (uint32_t)_refreshIntervalMs) {
        _lastActionMs = currentMillis;
        
        // Execute periodic logic here
    }

    // --- Step 2: Gate Satisfaction ---
    // Demonstrate gate pattern: open the gate after the first tick completes.
    if (!_gateOpen) {
        _gateOpen = true;
    }
}
```

### NATIVE_BUILD Compilation Guards
To allow your module to be fully tested on host machines (running tests on macOS or Linux), any hardware-specific code or SDK library that is unavailable on host machines must be guarded using `#ifndef NATIVE_BUILD`.

Observe how the web server routes are registered:
```cpp
#ifndef NATIVE_BUILD
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // Register real route handlers targeting target hardware here...
    server.on("/api/myapp/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        // Response logic
    });
}
#else
void MyApp::registerRoutes(PsychicHttpServer& server) {
    // Compile-time no-op when executing tests on host machine
    (void)server; 
}
#endif
```

---

## 4. Module Registration

Once your module is declared and implemented, wire it into the framework. In your application's entry point, [main.cpp](../../template/src/main.cpp), you instantiate your module and register it using the universal registration API:

```cpp
#include "AppFoundation.h"
#include "myApp.hpp"

Application app;
MyApp myApp;  // Stack allocated inside global scope

void setup() {
    // Register the module - one call auto-attaches it to:
    // ITickable (tick), IConfigurable (config), IWebContributor (REST),
    // IMCPContributor (MCP tools), and IConfigGate (boot gates).
    app.registerModule(&myApp);

    FrameworkConfig config;
    app.begin(config);
}

void loop() {
    app.tick();
}
```

---

## 5. Developer Checklist & Best Practices

When writing custom modules for the ESP32 App Foundation, you must adhere to these standards:

- [ ] **Never Use `delay()`:** Using `delay()` blocks the entire thread, halting WiFi operations, display rendering, web services, and other modules. Use the `currentMillis` comparison pattern instead.
- [ ] **Avoid Runtime Heap Allocation:** Avoid using `new`, `std::vector::push_back`, or Arduino `String` manipulation inside `tick()`. Allocate all memory upfront during initialization or on the stack to prevent heap fragmentation.
- [ ] **Implement `#ifndef NATIVE_BUILD` Guards:** Ensure your module compiles on host CPUs by mocking out target-specific hardware headers (e.g. wire drivers, ESP32 specific registers).
- [ ] **Adhere to the 10ms Budget:** Profile your `tick()` callback to ensure it returns within 10ms.
- [ ] **Namespace Configs:** Keep your configuration keys short and group them under a unique, lowercase namespace (e.g., `wifi`, `clock`, `myapp`).
- [ ] **Follow House-Style Documentation:** Add standard file header comment blocks and document your public methods using Doxygen-style markup.
- [ ] **Static Scope Only (ADR-16):** Modules MUST be declared at global or static scope, not stack-allocated in `setup()`. The ESP32 has no MMU — stack-allocated modules will silently corrupt memory or cause hard faults when the framework invokes them after `setup()` returns.

---

## 6. Common Mistakes to Avoid

1. **Late registration panics the device:** Calling `app.registerModule()` after `app.begin()` has been called will trigger a **fatal panic**. The registration registry is compacted and locked during boot — late registration is unconditionally forbidden. All registrations must occur in `setup()` before `app.begin()` is invoked.
2. **Stack-allocating modules (no MMU on ESP32):** Declaring a module as a local variable inside `setup()` creates a dangling pointer. The ESP32 has no MMU to trap this — it will silently corrupt memory or hard-fault when the framework calls the module's `tick()`. Always declare modules at global or `static` scope.
3. **Exposing hardware code in native builds:** Failing to wrap `#include <Wire.h>` or hardware-dependent drivers in `#ifndef NATIVE_BUILD` will cause host test builds to fail to compile.
4. **Implicit memory leaks in `tick()`:** Doing `JsonObject obj = doc.to<JsonObject>()` repeatedly inside `tick()` without clearing memory can lead to out-of-memory crashes.
5. **Duplicate registrations:** Calling `app.registerTickable(&myModule)` AND `app.registerModule(&myModule)` is redundant. The universal `registerModule()` call does all the work.
