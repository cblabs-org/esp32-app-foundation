# MCP Server

> ESP32 App Foundation (c) 2026 CB Labs
>
> The Model Context Protocol server, tool registration via `IMCPContributor`, built-in tools, HTTP JSON-RPC transport, and read-only safety mode.

---

This guide covers the MCP (Model Context Protocol) server built into the ESP32 App Foundation. The MCP server allows AI agents and other clients to interact with the device programmatically via JSON-RPC 2.0 over HTTP or Serial transports.

**Prerequisites:** Read the [Architecture Overview](architecture-overview.md) and [Creating a Module](creating-a-module.md) first.

---

## 1. Overview

The MCP server implements a JSON-RPC 2.0 interface that exposes device functionality as discrete **tools**. Each tool has a name, description, input schema, and a handler function. Clients discover available tools via `tools/list` and invoke them via `tools/call`.

The server supports two transports:

- **HTTP** — `POST /api/mcp` endpoint for network-based access (AI agents, automation scripts).
- **Serial** — Line-delimited JSON-RPC over the hardware UART (physical serial/USB connection).

---

## 2. Architecture

The MCP subsystem consists of three components:

| Component | File | Responsibility |
|---|---|---|
| [`MCPServer`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/MCPServer/MCPServer.hpp) | `modules/MCPServer/MCPServer.hpp` | JSON-RPC dispatcher, Serial polling, HTTP route handler, config persistence |
| [`MCPToolRegistry`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/MCPServer/MCPToolRegistry.hpp) | `modules/MCPServer/MCPToolRegistry.hpp` | Tool collection with name-based lookup |
| [`MCPBuiltinTools`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/modules/MCPServer/MCPBuiltinTools.hpp) | `modules/MCPServer/MCPBuiltinTools.hpp` | Foundation-level tools always available in the framework |

### Boot Sequence

During `MCPServer::begin()`:

1. **Built-in tools** are registered first via `mcpBuiltinTools::registerAll()`.
2. **Contributor tools** are collected from all registered `IMCPContributor` modules.
3. The server begins processing JSON-RPC requests on both transports.

---

## 3. Implementing `IMCPContributor`

Any module can expose MCP tools by implementing the [`IMCPContributor`](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/IMCPContributor.hpp) interface:

```cpp
#include "interfaces/IMCPContributor.hpp"

class IMCPContributor {
public:
    virtual ~IMCPContributor() = default;
    virtual void registerMCPTools(MCPToolRegistry& registry) = 0;
};
```

The framework auto-detects `IMCPContributor` during module registration. If your module inherits from this interface, `MCPServer::begin()` will call `registerMCPTools()` automatically — no manual wiring needed.

### Example Implementation

Tool handlers are **static function pointers** (not `std::function` — see GRAVE-05). Module state is passed through the opaque `void* context` parameter and recovered via `static_cast`. See `template-app/src/MyApp.cpp` for the canonical example.

```cpp
class WeatherModule : public ITickable, public IMCPContributor {
public:
    void registerMCPTools(MCPToolRegistry& registry) override {
        registry.addTool(
            "get_weather",
            "Get the current weather reading from the sensor",
            [](void* ctx, JsonObject& params, JsonObject& result) {
                auto* self = static_cast<WeatherModule*>(ctx);
                (void)params;
                result["temperature"] = self->_lastTemperature;
                result["humidity"]    = self->_lastHumidity;
                result["unit"]        = "celsius";
            },
            this   // context: WeatherModule*
        );

        registry.addTool(
            "set_weather_interval",
            "Set the weather polling interval in seconds",
            [](void* ctx, JsonObject& params, JsonObject& result) {
                auto* self = static_cast<WeatherModule*>(ctx);
                int interval = params["seconds"] | 60;
                self->_pollIntervalMs = interval * 1000;
                result["status"] = "ok";
                result["interval_seconds"] = interval;
            },
            this,  // context: WeatherModule*
            true   // mutatesState = true
        );
    }

    // ... rest of module implementation
};
```

---

## 4. Registering Tools

Tools are registered via `MCPToolRegistry::addTool()`. There are two overloads:

### Without Explicit Schema

```cpp
void addTool(const char* name,
             const char* description,
             MCPToolHandler handler,
             void* context = nullptr,
             bool mutatesState = false);
```

Creates a default empty schema: `{ "type": "object", "properties": {} }`.

### With Explicit Schema

```cpp
void addTool(const char* name,
             const char* description,
             const JsonDocument& inputSchema,
             MCPToolHandler handler,
             void* context = nullptr,
             bool mutatesState = false);
```

Allows you to define a JSON Schema for the tool's input parameters, which is returned to clients via `tools/list`.

### Handler Type

`MCPToolHandler` is a static function pointer defined in `MCPToolRegistry.hpp`:

```cpp
using MCPToolHandler = void(*)(void* context, JsonObject& params, JsonObject& result);
```

The handler receives three arguments:

- **`context`** — Opaque pointer passed through from `addTool()`. Typically the module's `this` pointer, recovered via `static_cast`.
- **`params`** — Input parameters from the JSON-RPC request. Access values with ArduinoJson syntax (e.g., `params["key"] | defaultValue`).
- **`result`** — Output object. Populate this with the tool's return values. The MCP server wraps this in the standard JSON-RPC response envelope.

> [!WARNING]
> **ArduinoJson 7 const-variant pitfall (Issue #13).** Inside `_handleToolsCall` the
> incoming request is a `const JsonDocument&`. Subscripting a const document yields a
> `JsonVariantConst`, and `is<JsonObject>()` / `is<JsonArray>()` silently return `false`
> for const variants — the mutable-type converters require a non-const `JsonVariant`.
> Always use `is<JsonObjectConst>()` / `is<JsonArrayConst>()` when the source is const.
> This does **not** affect handler code: `params` is passed as a mutable `JsonObject&`,
> so normal `params["key"].is<JsonObject>()` checks in handlers are correct.

> [!IMPORTANT]
> `std::function` and capturing lambdas are **not used** for MCP handlers (GRAVE-05). Lambda captures that exceed the Small Buffer Optimisation threshold cause hidden heap allocations. Use non-capturing lambdas with the `void* context` pattern instead.

### The `mutatesState` Parameter

The `mutatesState` boolean is critical for the read-only safety mode:

- **`false` (default):** The tool only reads data (e.g., `get_status`, `read_telemetry`). It remains accessible when the MCP server is in read-only mode.
- **`true`:** The tool writes data, changes configuration, controls hardware, or triggers actions. When the MCP server is in **read-only mode**, the server automatically blocks the tool call with JSON-RPC error `-32000` (Server error). The handler is never invoked.

> **Rule:** Contributors MUST declare `mutatesState = true` on any tool that writes data, changes configuration, controls hardware, or triggers actions.

---

## 5. Built-in Tools

The framework registers 6 built-in tools via `mcpBuiltinTools::registerAll()`. These are always available regardless of which application uses the framework:

| Tool | Description | Mutates State |
|---|---|---|
| `system_info` | Device heap metrics, uptime, chip info, WiFi status, firmware version | No |
| `get_config` | Read all module configurations via the ConfigManager gateway export | No |
| `set_config` | Update module configurations via the ConfigManager gateway import | Yes |
| `reboot` | Restart the ESP32 device | Yes |
| `get_logs` | Retrieve system logs (placeholder for future log ring-buffer) | No |
| `list_tools` | Enumerate all registered MCP tools with names, descriptions, and schemas | No |

*(Note: The `capture_display` tool is no longer built-in; it is self-registered by display modules and returns base64 PNG).*

### `system_info` Time & DHCP Telemetry Fields

When `TimeManager` is wired into `MCPServer`, `system_info` reports effective runtime timezone/NTP state alongside persisted DHCP enable flags and active lease discovery state:

| Field | Type | Description |
|---|---|---|
| `time_configured` | `bool` | True if manual config was loaded/saved or a DHCP timezone override is active |
| `time_synced` | `bool` | True once SNTP has completed at least one sync (`TimeSyncStatus::SYNCED`) |
| `time_timezone` | `string` | Active effective timezone (DHCP IANA/POSIX override when active, else persisted fallback) |
| `time_posix` | `string` | Active effective POSIX TZ string passed to `TZ` |
| `time_sync_status` | `string` | `"UNSYNCED"`, `"IN_PROGRESS"`, or `"SYNCED"` |
| `time_local` | `string` | Local ISO timestamp (`YYYY-MM-DDTHH:MM:SS`), present only when `time_synced` is true |
| `time_dhcp_ntp_enabled` | `bool` | Persisted `dhcpNtp` opt-in flag (`Option 42`) |
| `time_dhcp_ntp_active` | `bool` | True when at least one DHCP Option 42 NTP server occupies an SNTP slot |
| `time_dhcp_ntp_servers` | `string[]` | Active Option 42 NTP server IP addresses discovered from DHCP (up to 3) |
| `time_dhcp_tz_enabled` | `bool` | Persisted `dhcpTz` opt-in flag (RFC 4833 Options `100`/`101`) |
| `time_dhcp_tz_active` | `bool` | True when a DHCP Option 100 or 101 override is actively applied in RAM |
| `time_dhcp_tz_source` | `string` | Source of active DHCP timezone override: `"none"`, `"pcode"` (Option 100), or `"tcode"` (Option 101) |

### Display Module Contributed Tools

Optional display modules contribute additional MCP tools when registered:

| Module | Tool | Description | Mutates State |
|---|---|---|---|
| `U8G2DisplayModule` | `display_status` | Get display state, brightness, active board, and screensaver status | No |
| `U8G2DisplayModule` | `display_control` | Control display brightness, screensaver, board navigation, and orientation | Yes |
| `U8G2DisplayModule` | `capture_display` | Capture display framebuffer and return as Base64-encoded PNG image | No |
| `LVGLDisplayModule` | `display_status` | Get display state, brightness, active screen, and screensaver status | No |
| `LVGLDisplayModule` | `display_control` | Control display brightness, screensaver, and screen navigation | Yes |
| `LVGLDisplayModule` | `capture_display` | Capture display framebuffer and return as Base64-encoded PNG image | No |

---

## 6. HTTP Transport

### Endpoint

```
POST http://<device-ip>/api/mcp
Content-Type: application/json
```

### JSON-RPC Format

The server supports standard JSON-RPC 2.0 methods:

| Method | Purpose |
|---|---|
| `initialize` | MCP handshake — returns server capabilities |
| `ping` | Keep-alive / connectivity check |
| `tools/list` | Enumerate all registered tools with schemas |
| `tools/call` | Invoke a specific tool by name |

**Example — List all tools:**

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/list",
    "params": {}
}
```

**Example — Call a tool:**

```json
{
    "jsonrpc": "2.0",
    "id": 2,
    "method": "tools/call",
    "params": {
        "name": "system_info",
        "arguments": {}
    }
}
```

### Authentication

When `AuthManager` authentication is enabled, all HTTP `/api/mcp/*` requests require authorization:

1. **Session cookie** — `esp32_session=<hex>` (for browser-based MCP access).
2. **Bearer token** — `Authorization: Bearer <token>` header. The token is the stored password hash, retrievable via `GET /api/auth/mcp-token` (requires an active portal session).

When authentication is disabled, the HTTP endpoint is open.

The `/api/mcp` path prefix is excluded from the global portal auth middleware — `MCPServer` handles its own authentication independently via `AuthManager::checkMcpAuth()`.

**Serial transport is exempt from authentication** — physical serial access is treated as trusted.

---

## 7. Configuration Persistence

`MCPServer` implements `IConfigurable` (namespace: `"mcp"`) to persist two runtime toggles:

| Setting | Default | Purpose |
|---|---|---|
| `enabled` | `true` | Master on/off switch. When `false`, all Serial and HTTP processing is disabled. |
| `readOnly` | `false` | When `true`, tools with `mutatesState = true` are blocked with error `-32000`. |

These settings are persisted to `/mod/mcp.json` via `ConfigManager` and survive reboots.

### Web Portal Tab

`MCPServer` contributes a "MCP" sub-tab under the System bottom-nav tab (order: 30, after Keys at order 20). The portal UI exposes the enabled/readOnly toggles for user configuration.

---

## 8. Testing MCP Tools

### With `curl`

**List available tools:**

```bash
curl -s -X POST http://<device-ip>/api/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}' | jq
```

**Get system info:**

```bash
curl -s -X POST http://<device-ip>/api/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"system_info","arguments":{}}}' | jq
```

**With authentication (bearer token):**

```bash
# First, get the MCP token from the portal (requires active session)
TOKEN=$(curl -s -b "esp32_session=<your-session-hex>" \
  http://<device-ip>/api/auth/mcp-token | jq -r '.token')

# Then use the bearer token for MCP calls
curl -s -X POST http://<device-ip>/api/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"system_info","arguments":{}}}' | jq
```

### With an MCP Client

The framework-expert skill (`scripts/device-mcp/`) provides Python-based clients for programmatic interaction. Any JSON-RPC 2.0 compatible client can connect to the HTTP endpoint.

### Over Serial

Send line-delimited JSON-RPC to the device's serial port (typically 115200 baud):

```
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

The server responds with a single JSON line per request. Serial transport does not require authentication.
