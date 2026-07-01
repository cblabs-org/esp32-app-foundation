# IMCPContributor

Contract for modules that expose MCP (Model Context Protocol) tools to the `MCPServer`.

## Overview

The `MCPServer` calls `registerMCPTools()` once during `begin()`. The framework uses a two-tier command execution model for LLM agents connected to the device.

```cpp
class IMCPContributor {
public:
    virtual ~IMCPContributor() = default;
    virtual void registerMCPTools(MCPToolRegistry& registry) = 0;
};
```

Auto-discovery: `Application::registerModule()` uses `std::is_base_of_v<IMCPContributor, T>` at compile time to detect and register contributors. No manual wiring needed.

## Registering Tools

Use the `registry.addTool()` method. The handler signature is a **stateless function pointer** with a `void* context` parameter — never use `std::function` or capturing lambdas (GRAVE-05 heap allocation risk).

```cpp
void MyModule::registerMCPTools(MCPToolRegistry& registry) {
    // Read-only tool
    registry.addTool("my_get_status",
        "Get current status of MyModule",
        [](void* ctx, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<MyModule*>(ctx);
            result["value"] = self->getValue();
            result["active"] = self->isActive();
        }, this, false);  // mutatesState = false

    // Mutating tool
    registry.addTool("my_reset",
        "Reset MyModule to default state",
        [](void* ctx, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<MyModule*>(ctx);
            self->reset();
            result["success"] = true;
        }, this, true);   // mutatesState = true

    ESP_LOGI(TAG, "Registered MCP tools: my_get_status, my_reset");
}
```

### `addTool` Signature

```cpp
void addTool(
    const char* name,           // Tool name (snake_case, unique)
    const char* description,    // Human-readable description for agents
    MCPToolHandler handler,     // void(*)(void*, JsonObject&, JsonObject&)
    void* context = nullptr,    // Passed as first arg to handler (typically `this`)
    bool mutatesState = false   // true → blocked in read-only mode
);
```

An overload accepting a `JsonDocument` schema parameter is available for tools with explicit parameter definitions (see `display_control` for an example).

## Rules

1. **Stateless lambdas only** — capture-less `[](void* ctx, ...)` that decay to function pointers. No `std::function`, no heap captures.
2. **`void* context`** — pass `this`; cast back with `static_cast<MyModule*>(ctx)` inside the handler.
3. **`mutatesState` flag** — set `true` for any tool that writes data, changes config, controls hardware, or triggers actions. Read-only mode automatically blocks these with JSON-RPC error `-32000`.
4. **Stub Headers** — If the tool uses hardware APIs, ensure a stub is provided in `test/stubs/platform/` so native tests can compile without inline guards.
5. **Naming** — use `module_verb` pattern (e.g., `wifi_get_status`, `ota_check_now`, `config_factory_reset`).
6. **Logging** — end `registerMCPTools()` with an `ESP_LOGI` listing all registered tool names.

## Verifying on Device

After flashing, call `list_tools` via the device MCP server to confirm your tools appear. See `systems/mcp-server/overview.md` for the JSON-RPC protocol and device interaction scripts.
