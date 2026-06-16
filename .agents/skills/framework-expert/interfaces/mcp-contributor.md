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

## Registering Tools

Use the `registry.addTool()` method to expose functionality.

```cpp
void MyModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "reboot_device",
        "Restart the ESP32. Will disconnect the current session.",
        true, // mutatesState
        [this](const JsonObject& params) -> String {
            // Initiate reboot
            this->triggerReboot();
            return "Reboot sequence initiated.";
        }
    );
}
```

## The `mutatesState` Parameter

The third parameter to `addTool()` is a boolean indicating whether the tool modifies device state.

> [!IMPORTANT]
> Contributors MUST declare `mutatesState = true` on any tool that writes data, changes configuration, controls hardware, or triggers actions. 

When the MCP server is put into **Read-Only Mode** (e.g., via the physical switch or a security policy), the `MCPServer` engine automatically intercepts and blocks any tool call where `mutatesState == true`, returning JSON-RPC error `-32000` (Server error). The module handler is never invoked.

Tools that purely query data (e.g., `get_status`, `read_telemetry`) should set `mutatesState = false` to remain accessible in read-only mode.
