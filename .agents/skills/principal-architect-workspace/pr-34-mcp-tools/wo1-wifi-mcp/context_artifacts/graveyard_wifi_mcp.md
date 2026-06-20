# Graveyard: WifiManager MCP Tools

## Failed Attempts and Dead-ends

1. **Capturing Lambdas in MCPToolRegistry**
   - **Attempt**: Tried to use `[this]` capturing lambdas for `registry.addTool(...)`.
   - **Result**: Compilation failed with `error: no matching function for call to 'MCPToolRegistry::addTool...'`.
   - **Reason**: `MCPToolRegistry::addTool` expects a static function pointer `void(*)(...)`, not a `std::function`. This architectural choice deliberately prevents hidden heap allocations when captures exceed the SBO threshold.
   - **Resolution**: Used stateless `[]` lambdas and passed `this` explicitly via the `void* context` parameter. Inside the lambda, `context` was cast back via `WifiManager* self = static_cast<WifiManager*>(context)`.

2. **Unconditional STA-Mode Network Metrics**
   - **Attempt**: In `wifi_get_status`, queried `WiFi.localIP()` and `WiFi.RSSI()` unconditionally.
   - **Result**: When the ESP32 booted into Setup (AP mode), the tool misleadingly returned `0.0.0.0` and `rssi: 0`.
   - **Resolution**: Branched the query based on `self->_isAPMode` to return `WiFi.softAPIP()` and set `ap_mode: true`.
