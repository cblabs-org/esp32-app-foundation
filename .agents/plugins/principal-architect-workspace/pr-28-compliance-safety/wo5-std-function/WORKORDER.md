---
name: pr28-wo5
description: Handoff instructions for Work Order 5 — eradicate std::function from MCPToolRegistry + OTADownloadEngine
complexity: architect
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Replace all `std::function` usage in framework-owned code with static function
pointer + `void* context` patterns. This eliminates hidden heap allocations from
lambda captures exceeding the SBO threshold (GRAVE-05 violation).

`std::function` usage in PsychicHttp library typedefs (`WebServerManager.hpp`)
is an **external constraint** — document it, don't change it.

# Strict Boundary Box

- **DO** modify MCPToolRegistry (hpp + cpp), MCPBuiltinTools.cpp, MCPServer.cpp,
  OTADownloadEngine (hpp + cpp), OTAUpdateManager.cpp.
- **DO NOT** touch WebServerManager.hpp — those `std::function` typedefs are from
  the PsychicHttp library API.
- **DO NOT** change TAG definitions, naming conventions, or other unrelated code.
- **DO NOT** modify any test files unless they directly test the changed APIs.

# Background: Function Pointer + Context Pattern

The framework's `EventBus` already uses this pattern. Follow it:

```cpp
// Type alias — replaces std::function
using MCPToolHandler = void(*)(void* context, JsonObject& params, JsonObject& result);

// Storage struct — replaces MCPToolEntry
struct MCPToolEntry {
    std::string name;
    std::string description;
    JsonDocument inputSchema;
    MCPToolHandler handler;     // Static function pointer
    void* context = nullptr;    // Opaque context for the handler
    bool mutatesState = false;
};
```

# Checklist

## A. MCPToolRegistry

- [ ] **`modules/MCPServer/MCPToolRegistry.hpp`**
  - Replace `std::function<void(JsonObject& params, JsonObject& result)>` in
    `MCPToolEntry::handler` with a static function pointer typedef:
    ```cpp
    using MCPToolHandler = void(*)(void* context, JsonObject& params, JsonObject& result);
    ```
  - Add `void* context = nullptr;` to `MCPToolEntry`
  - Update both `addTool()` overloads to take `MCPToolHandler handler, void* context`
    instead of `std::function<...> handler`
  - Remove `#include <functional>` if no other usage remains

- [ ] **`modules/MCPServer/MCPToolRegistry.cpp`**
  - Update both `addTool()` implementations to store `handler` and `context`
  - Update `findTool()`/dispatch to pass `entry.context` as first arg when
    invoking the handler

- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp`**
  - Update all `addTool()` call sites. Each lambda currently captures `this`
    (or module pointers). Convert to:
    ```cpp
    // Before:
    registry.addTool("system_info", "...", [this](JsonObject& p, JsonObject& r) { ... });

    // After — static function:
    static auto systemInfoHandler = [](void* ctx, JsonObject& p, JsonObject& r) {
        auto* self = static_cast<MCPBuiltinTools*>(ctx);
        // ... existing body using self-> instead of this->
    };
    registry.addTool("system_info", "...", systemInfoHandler, this);
    ```
    Note: If the lambda captures nothing beyond `this`, a static non-capturing
    lambda with `void* ctx` is the cleanest pattern. If it captures multiple
    pointers, create a small context struct.

- [ ] **`modules/MCPServer/MCPServer.cpp`** — if any `addTool` calls exist here,
  update them with the same pattern.

## B. OTADownloadEngine

- [ ] **`lib/OTADownloadEngine/OTADownloadEngine.hpp`**
  ```cpp
  // BEFORE:
  using OTAProgressCallback = std::function<void(size_t bytesWritten, size_t totalSize)>;

  // AFTER:
  using OTAProgressCallback = void(*)(void* context, size_t bytesWritten, size_t totalSize);
  ```
  - Update `setProgressCallback()` to also take `void* context`
  - Add `void* _progressContext = nullptr;` member variable
  - Remove `#include <functional>` if no other usage remains

- [ ] **`lib/OTADownloadEngine/OTADownloadEngine.cpp`**
  - Update `setProgressCallback()` to store both callback and context
  - Update progress callback invocation to pass context as first arg

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp`**
  - Update the `setProgressCallback()` call to pass context:
    ```cpp
    // Before:
    engine.setProgressCallback([this](size_t written, size_t total) {
        publishProgress(...)
    });

    // After:
    engine.setProgressCallback([](void* ctx, size_t written, size_t total) {
        auto* self = static_cast<OTAUpdateManager*>(ctx);
        self->publishProgress(...);
    }, this);
    ```

## C. Document External Constraint

- [ ] **`modules/WebServer/WebServerManager.hpp`** — Add a comment near the
  PsychicHttp typedefs:
  ```cpp
  // NOTE: PsychicHttpRequestCallback, PsychicMiddlewareNext, and
  // PsychicMiddlewareCallback are std::function typedefs defined by the
  // PsychicHttp library. These are an external constraint and cannot be
  // replaced without forking the library. (Audit ref: H-01 / GRAVE-05)
  ```

# Important Notes

- The `MCPBuiltinTools` conversion is the most labour-intensive — it has many
  `addTool()` calls with lambdas. Each lambda that captures `this` converts to
  a static function receiving `void* ctx`.
- Test any existing MCP tool tests still pass — the handler signature change
  will require test updates.
- `IMCPContributor` modules that register tools will need updating if they
  exist outside MCPBuiltinTools.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
