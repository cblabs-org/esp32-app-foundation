# Architecture Decision Record: WifiManager MCP Tools

## Context
Implementing the `IMCPContributor` interface on `WifiManager` to expose domain-specific tools: `wifi_get_status`, `wifi_scan_networks`, and `wifi_forget`. 

## Decisions
1. **Stateless Lambdas for Handlers**: `MCPToolRegistry` explicitly uses `void(*)(void* context, JsonObject& params, JsonObject& result)` to prevent hidden heap allocations associated with `std::function` lambda captures exceeding the SBO threshold. We passed stateless lambdas `[]` and cast the `void* context` back to `WifiManager*` to access instance state.
2. **AP Mode Status Branching**: `wifi_get_status` must differentiate between AP and STA modes. Unconditionally calling `WiFi.localIP()` in AP mode yields `0.0.0.0`. By checking `_isAPMode`, we correctly return `WiFi.softAPIP()`, `rssi = 0`, and `ap_mode = true`.
3. **Safe Scanning**: `wifi_scan_networks` uses `WIFI_AP_STA` mode when triggered during an AP session to allow the STA radio to scan without dropping the active AP connection. It performs a synchronous scan for the MCP caller and cleans up with `WiFi.scanDelete()`.
4. **Deferred Restarts**: `wifi_forget` utilizes the `scheduleRestart()` deferral pattern to avoid instantly tearing down the network stack and crashing the HTTP server thread responding to the MCP dispatch.
