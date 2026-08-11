# Routing with PsychicHttp

The framework utilizes the `PsychicHttp` library to handle web server routing. It is chosen for its performance, low memory overhead, and robust handling of asynchronous requests.

## Route Patterns

Modules register their API endpoints via the `IWebContributor` interface by overriding the `registerRoutes(PsychicHttpServer* server)` method.

Routes are typically defined using C++ lambdas. 

### GET Lambdas

GET requests are straightforward. You receive a request object and return a response.

```cpp
void MyModule::registerRoutes(PsychicHttpServer* server) {
    server->on("/api/weather", HTTP_GET, [](PsychicRequest* request) {
        JsonDocument doc;
        doc["temp"] = 22.5;
        doc["humidity"] = 45;
        
        String response;
        serializeJson(doc, response);
        return request->reply(200, "application/json", response.c_str());
    });
}
```

### POST Lambdas and ADR-15 Safety

Handling POST requests (which carry a JSON body) requires strict safety constraints on the ESP32 due to RAM limitations. This is governed by **ADR-15**.

**ADR-15 Rules:**
1. **Provider-Declared Body Limits**: The framework enforces POST/PUT body
   size limits via global middleware in `WebServerManager`. Each
   `IWebContributor` declares its own limit by overriding `getMaxPostBody()`
   (default: 4096 bytes, the ADR-15 standard). The middleware matches
   incoming requests to the contributor's `getApiPrefix()` and enforces
   the declared limit, rejecting oversized bodies with HTTP 413 before
   the handler is invoked.

   To allow larger payloads, override both methods in your contributor:
   ```cpp
   const char* getApiPrefix() const override { return "/api/mymodule"; }
   size_t getMaxPostBody() const override { return 8192; }
   ```

2. **`JsonDocument` (ArduinoJson v7)**: ArduinoJson 7 removed `StaticJsonDocument` and `DynamicJsonDocument`, replacing both with a single elastic `JsonDocument`. Handlers MUST parse incoming JSON using `JsonDocument doc;` (no template size parameter). Because the middleware already caps the body to the contributor's declared `getMaxPostBody()` limit, a `JsonDocument` fed a bounded body cannot over-allocate. As defense-in-depth, you may also pass a max length to the parser (`deserializeJson(doc, body, maxLen)`).

```cpp
void MyModule::registerRoutes(PsychicHttpServer* server) {
    server->on("/api/weather/config", HTTP_POST, [](PsychicRequest* request) {
        if (!request->hasBody()) {
            return request->reply(400, "text/plain", "Missing body");
        }
        
        // ADR-15: Bounded by getMaxPostBody() middleware
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, request->body());
        
        if (error) {
            return request->reply(400, "text/plain", "Invalid JSON");
        }
        
        // Process data...
        float newThreshold = doc["threshold"];
        
        return request->reply(200, "application/json", "{\"status\":\"ok\"}");
    });
}
```

By using `JsonDocument` with body size caps enforced via the global middleware (`getMaxPostBody()`), the web server remains stable even under heavy load or malicious input.

## Display REST Endpoints

The framework provides shared REST endpoints for display interaction, registered by `DisplayServiceHelper` (used by both U8G2 and LVGL modules):

- `GET /api/display/status`: Returns current display telemetry and state.
- `POST /api/display/brightness`: Sets display brightness (`{"value": 0-255}`).
- `GET /api/display/screenshot`: Captures a PNG screenshot of the current framebuffer.
- `POST /api/display/wake`: Wakes the display from screensaver/sleep.
- `POST /api/display/sleep`: Forces the display to sleep immediately.

## Built-in MCP Tool Endpoints

For AI agent interaction, the framework provides built-in JSON-RPC 2.0 MCP tools mapped alongside REST endpoints:

- `system_info`: Device telemetry (heap, PSRAM, storage, WiFi, uptime).
- `get_config`: Read all module configurations.
- `set_config`: Update full configuration by namespace (`{"config":{"wifi":{...}}}`).
- `patch_config`: Apply RFC 6902 JSON Patch ops to a single config namespace.
- `reboot`: Restart the device with a 1-second flush delay.
- `get_logs`: Read buffered log entries.
- `list_tools`: Enumerate all registered MCP tools and schemas.
- `capture_display`: Capture monochrome/RGB display framebuffer.
