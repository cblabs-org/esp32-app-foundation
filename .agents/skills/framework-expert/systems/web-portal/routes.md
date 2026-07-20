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
        StaticJsonDocument<256> doc;
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

2. **StaticJsonDocument**: Handlers MUST parse incoming JSON using `StaticJsonDocument` allocated on the stack (e.g., `StaticJsonDocument<1024>`). You MUST NOT use `DynamicJsonDocument` in route handlers, as concurrent requests will fragment the heap and cause panics.

```cpp
void MyModule::registerRoutes(PsychicHttpServer* server) {
    server->on("/api/weather/config", HTTP_POST, [](PsychicRequest* request) {
        if (!request->hasBody()) {
            return request->reply(400, "text/plain", "Missing body");
        }
        
        // ADR-15: Stack allocation
        StaticJsonDocument<512> doc;
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

By strictly adhering to stack-allocated JSON documents and enforcing body size caps via the global middleware, the web server remains stable even under heavy load or malicious input.
