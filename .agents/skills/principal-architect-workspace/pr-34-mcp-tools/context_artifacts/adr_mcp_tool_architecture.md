# ADR: MCP Tool Architecture Patterns

## Decision: Stateless Lambdas + void* Context

All module MCP tools use capture-less lambdas `[](void* ctx, JsonObject& args, JsonObject& result)` that decay to `MCPToolHandler` function pointers. The `void* context` parameter carries `this`, cast back via `static_cast<Module*>(ctx)`.

**Why:** `std::function` is prohibited (GRAVE-05) because it heap-allocates a closure object. On ESP32 with ~300KB DRAM, every hidden allocation matters. Function pointers are zero-cost.

## Decision: #ifndef NATIVE_BUILD Guards

All `registerMCPTools()` implementations wrap hardware-dependent tool registrations in `#ifndef NATIVE_BUILD`. This allows the module to compile in the native test environment without linking against ESP-IDF APIs.

**Why:** The project uses PlatformIO's `native` environment for unit testing. Hardware APIs (`WiFi`, `LittleFS`, `esp_restart()`) are not available there.

## Decision: Naming Convention — module_verb

Tool names follow `module_verb` snake_case: `wifi_get_status`, `ota_check_now`, `config_factory_reset`. This groups tools by domain for discoverability in `list_tools` output.

## Decision: mutatesState Flag

Every tool that writes data, changes config, controls hardware, or triggers actions MUST set `mutatesState = true`. Read-only mode automatically blocks these with JSON-RPC error `-32000`.
