/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/MCPServer/MCPServer.cpp
 * Description: Implementation of the MCP JSON-RPC 2.0 server. Reads
 *              line-delimited JSON from Serial on hardware, parses each line
 *              into a JsonDocument, and dispatches to the appropriate handler
 *              (initialize, tools/list, tools/call, ping). All responses are
 *              written as serialised JSON followed by a newline delimiter.
 *
 * Exported Functions/Classes:
 * - MCPServer: see MCPServer.hpp
 */

#include "MCPServer.hpp"
#include "MCPBuiltinTools.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "../WifiManager/WifiManager.hpp"
#include "../WebServer/WebServerManager.hpp"
#include "../AuthManager/AuthManager.hpp"
#include <ConfigManager.hpp>

// ---------------------------------------------------------------------------
// Platform-conditional includes & logging shims
// Matches the Application.cpp pattern: ESP-IDF logs on hardware, printf on native.
// ---------------------------------------------------------------------------
#include "platform/LogShim.h"

static constexpr const char* TAG = "MCPServer";

    // Serial is now instantiated in test/stubs/platform/Arduino.h for native builds

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/**
 * @brief Collect tools from all IMCPContributor modules and mark the server
 *        as initialised so tick() begins processing.
 */
void MCPServer::begin(const std::vector<IMCPContributor*>& contributors, ConfigManager* configManager, TimeManager* timeManager, WifiManager* wifiManager, AuthManager* authManager) {
    // --- Step 0: Cache dependencies for runtime config persistence ---
    _configManager = configManager;
    _wifiManager = wifiManager;
    _authManager = authManager;

    // --- Step 1: Register built-in foundation tools ---
    // These are always available regardless of which app uses the framework.
    _registryPtr = &_registry;
    mcpBuiltinTools::registerAll(_registry, configManager, nullptr, timeManager);

    // --- Step 2: Collect tools from all contributors ---
    for (auto* contributor : contributors) {
        if (contributor != nullptr) {
            contributor->registerMCPTools(_registry);
        }
    }

    ESP_LOGI(TAG, "MCP Server started with %zu tools (%zu built-in)",
             _registry.size(), _registry.size() - 0);

    // --- Step 3: Mark initialised ---
    _initialized = true;
}

// ---------------------------------------------------------------------------
// Late-binding display info
// ---------------------------------------------------------------------------

void MCPServer::setDisplayInfo(const DisplayInfo& info) {
    mcpBuiltinTools::setDisplayInfo(info);
}

// ---------------------------------------------------------------------------
// Tick — Serial polling (hardware) / no-op (native)
// ---------------------------------------------------------------------------

/**
 * @brief Read available bytes from Serial, accumulate into a line buffer,
 *        and dispatch complete lines to _processLine().
 *
 * A per-tick read limit prevents the loop from monopolising CPU time.
 * On some ESP32-S3 boards with CONFIG_ESP_CONSOLE_NONE, Serial.available()
 * can return non-zero indefinitely, which would block the main task without
 * this guard.
 */
void MCPServer::tick() {
    if (!_initialized || !_mcpEnabled) return;

    // --- Hardware: read characters from Serial until newline ---
    static constexpr int kMaxBytesPerTick = 2048;
    static constexpr size_t kMaxInputLen = 8192;
    int bytesRead = 0;
    while (Serial.available() && bytesRead < kMaxBytesPerTick) {
        char c = Serial.read();
        bytesRead++;
        if (c == '\n') {
            _processLine(_inputBuffer.c_str());
            _inputBuffer.clear();
        } else if (_inputBuffer.size() < kMaxInputLen) {
            _inputBuffer += c;
        }
        // else: silently drop characters beyond kMaxInputLen
    }
}

/**
 * @brief Test harness entry point: feed a raw JSON-RPC line for processing.
 */
void MCPServer::processLineForTest(const char* line) {
    _processLine(line);
}

// ---------------------------------------------------------------------------
// JSON-RPC Dispatch
// ---------------------------------------------------------------------------

/**
 * @brief Parse a JSON-RPC request and route to the correct handler.
 */
void MCPServer::processRequest(const char* requestJson, Print& responseStream) {
    // --- Step 1: Parse the JSON payload ---
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, requestJson);

    if (err) {
        // Parse error — respond with JSON-RPC parse error code
        ESP_LOGE(TAG, "JSON parse error: %s", err.c_str());
        // Parse errors have no valid id — pass null via a temporary doc
        JsonDocument nullDoc;
        nullDoc["id"] = nullptr;
        _sendError(-32700, "Parse error", nullDoc["id"], responseStream);
        responseStream.println();
        return;
    }

    // --- Step 2: Extract the method string ---
    const char* method = doc["method"] | static_cast<const char*>(nullptr);

    if (method == nullptr) {
        // Missing "method" field — invalid request
        _sendError(-32600, "Invalid Request: missing method", doc["id"], responseStream);
        responseStream.println();
        return;
    }

    // --- Step 3: Route to the appropriate handler ---
    if (strcmp(method, "initialize") == 0) {
        _handleInitialize(doc, responseStream);
    } else if (strcmp(method, "tools/list") == 0) {
        _handleToolsList(doc, responseStream);
    } else if (strcmp(method, "tools/call") == 0) {
        _handleToolsCall(doc, responseStream);
    } else if (strcmp(method, "ping") == 0) {
        _handlePing(doc, responseStream);
    } else {
        // Unknown method
        ESP_LOGW(TAG, "Unknown method: %s", method);
        _sendError(-32601, "Method not found", doc["id"], responseStream);
    }

    // Newline delimiter after every response
    responseStream.println();
}

/**
 * @brief Parse a JSON-RPC request line and route to the correct handler.
 *
 * Routing table:
 *   "initialize" → _handleInitialize()
 *   "tools/list" → _handleToolsList()
 *   "tools/call" → _handleToolsCall()
 *   "ping"       → _handlePing()
 *   (other)      → method-not-found error (-32601)
 */
void MCPServer::_processLine(const char* line) {
    processRequest(line, Serial);
}

// ---------------------------------------------------------------------------
// Handler Implementations
// ---------------------------------------------------------------------------

/**
 * @brief Respond to the MCP "initialize" handshake with server capabilities.
 *
 * Returns the MCP protocol version, server info (name + firmware version),
 * and declared capabilities (tools with listChanged: false).
 */
void MCPServer::_handleInitialize(const JsonDocument& req, Print& out) {
    JsonDocument resp;
    resp["jsonrpc"] = "2.0";
    resp["id"]      = req["id"];

    JsonObject result     = resp["result"].to<JsonObject>();
    result["protocolVersion"] = "2024-11-05";

    // Server identity uses the compile-time version macro
    JsonObject serverInfo = result["serverInfo"].to<JsonObject>();
    serverInfo["name"]    = "esp32-app-foundation";
    serverInfo["version"] = BUILD_VERSION;

    // Capabilities: we support tool listing but not dynamic listChanged
    JsonObject capabilities = result["capabilities"].to<JsonObject>();
    JsonObject tools        = capabilities["tools"].to<JsonObject>();
    tools["listChanged"]    = false;

    serializeJson(resp, out);
}

/**
 * @brief Enumerate all registered tools as a JSON-RPC response.
 *
 * Each tool entry includes its name, description, and inputSchema.
 */
void MCPServer::_handleToolsList(const JsonDocument& req, Print& out) {
    JsonDocument resp;
    resp["jsonrpc"] = "2.0";
    resp["id"]      = req["id"];

    JsonObject result   = resp["result"].to<JsonObject>();
    JsonArray toolsArr  = result["tools"].to<JsonArray>();

    for (const auto& tool : _registry.getTools()) {
        JsonObject toolObj       = toolsArr.add<JsonObject>();
        toolObj["name"]          = tool.name;
        toolObj["description"]   = tool.description;
        toolObj["inputSchema"]   = tool.inputSchema;
        toolObj["mutatesState"]  = tool.mutatesState;
    }

    serializeJson(resp, out);
}

/**
 * @brief Invoke a specific tool by name via the registry.
 *
 * Extracts params.name and params.arguments from the request, looks up the
 * tool in the registry, calls its handler, and wraps the result in MCP
 * content format. If the handler sets result["isError"] = true, the outer
 * isError flag is flipped accordingly.
 */
void MCPServer::_handleToolsCall(const JsonDocument& req, Print& out) {
    // --- Step 1: Extract tool name and arguments ---
    const char* toolName = req["params"]["name"] | static_cast<const char*>(nullptr);

    if (toolName == nullptr) {
        _sendError(-32602, "Invalid params: missing tool name", req["id"], out);
        return;
    }

    // --- Step 2: Look up the tool ---
    const MCPToolEntry* tool = _registry.findTool(toolName);

    if (tool == nullptr) {
        // Build descriptive error message
        ESP_LOGW(TAG, "Unknown tool: %s", toolName);
        JsonDocument errResp;
        errResp["jsonrpc"] = "2.0";
        errResp["id"]      = req["id"];
        JsonObject error   = errResp["error"].to<JsonObject>();
        error["code"]      = -32602;

        // Construct error message with tool name
        std::string msg = "Unknown tool: ";
        msg += toolName;
        error["message"] = msg;

        serializeJson(errResp, out);
        return;
    }

    // --- Step 2.5: Enforce Security Toggle ---
    if (_mcpReadOnly && tool->mutatesState) {
        ESP_LOGW(TAG, "Security error: tool %s mutates state in read-only mode", toolName);
        JsonDocument errResp;
        errResp["jsonrpc"] = "2.0";
        errResp["id"]      = req["id"];
        JsonObject error   = errResp["error"].to<JsonObject>();
        error["code"]      = -32000;
        error["message"]   = "Tool modifies state but server is in read-only mode";
        serializeJson(errResp, out);
        return;
    }

    // --- Step 3: Prepare arguments object ---
    // If params.arguments is missing, create an empty object
    JsonDocument argsDoc;
    if (req["params"]["arguments"].is<JsonObject>()) {
        argsDoc.set(req["params"]["arguments"]);
    } else {
        argsDoc.to<JsonObject>();
    }
    JsonObject argsObj = argsDoc.as<JsonObject>();

    // --- Step 4: Execute the handler ---
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();

    tool->handler(tool->context, argsObj, resultObj);

    // --- Step 5: Check if handler flagged an error ---
    bool isError = resultObj["isError"].is<bool>() && resultObj["isError"].as<bool>();
    // Remove the isError flag from the inner result if present
    resultObj.remove("isError");

    // --- Step 6: Serialize the inner result to a text string ---
    std::string innerText;
    serializeJson(resultDoc, innerText);

    // --- Step 7: Build the MCP-format response ---
    JsonDocument resp;
    resp["jsonrpc"] = "2.0";
    resp["id"]      = req["id"];

    JsonObject result   = resp["result"].to<JsonObject>();
    JsonArray content   = result["content"].to<JsonArray>();
    JsonObject textObj  = content.add<JsonObject>();
    textObj["type"]     = "text";
    textObj["text"]     = innerText;
    result["isError"]   = isError;

    serializeJson(resp, out);
}

/**
 * @brief Respond to "ping" with an empty result object.
 */
void MCPServer::_handlePing(const JsonDocument& req, Print& out) {
    JsonDocument resp;
    resp["jsonrpc"] = "2.0";
    resp["id"]      = req["id"];
    resp["result"].to<JsonObject>(); // Empty result object

    serializeJson(resp, out);
}

/**
 * @brief Send a standard JSON-RPC error response.
 *
 * Format: { "jsonrpc": "2.0", "id": <id>, "error": { "code": <code>, "message": <msg> } }
 */
void MCPServer::_sendError(int code, const char* message, JsonVariantConst id, Print& out) {
    JsonDocument resp;
    resp["jsonrpc"] = "2.0";

    // Preserve the request id (may be null for parse errors)
    if (id.isNull()) {
        resp["id"] = nullptr;
    } else {
        resp["id"] = id;
    }

    JsonObject error = resp["error"].to<JsonObject>();
    error["code"]    = code;
    error["message"] = message;

    serializeJson(resp, out);
}

// ---------------------------------------------------------------------------
// IWebContributor Implementation
// ---------------------------------------------------------------------------

void MCPServer::registerRoutes(PsychicHttpServer& server) {
    // --- POST /api/mcp — JSON-RPC endpoint for MCP tool calls ---
    // NOTE: Serial transport (_processLine) is intentionally NOT guarded —
    // Serial/USB access is physical access and is treated as trusted.
    server.on("/api/mcp", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            // --- Guard: bearer token auth (WO6) ---
            if (_authManager && !_authManager->checkMcpAuth(request, response)) return ESP_FAIL;
            // --- Guard: reject requests when MCP server is disabled ---
            if (!_mcpEnabled) {
                response->send(503, "application/json", "{\"error\":\"MCP server disabled\"}");
                return ESP_FAIL;
            }

            // --- Per-route body size enforcement (1KB) ---
            if (request->contentLength() > 1024) {
                response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
                return ESP_FAIL;
            }

            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"Missing body\"}");
                return ESP_FAIL;
            }

            class StringPrinter : public Print {
            public:
                std::string output;
                size_t write(uint8_t c) override { output += (char)c; return 1; }
                size_t write(const uint8_t *buffer, size_t size) override {
                    // Pre-allocate for performance if empty
                    if (output.length() == 0) output.reserve(size);
                    for(size_t i = 0; i < size; ++i) { output += (char)buffer[i]; }
                    return size;
                }
            } printer;

            this->processRequest(body, printer);
            response->send(200, "application/json", printer.output.c_str());
            return ESP_OK;
        }
    );

    // --- GET /api/mcp/status — MCP server status summary ---
    server.on("/api/mcp/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        // --- Guard: bearer token auth (WO6) ---
        if (_authManager && !_authManager->checkMcpAuth(request, response)) return ESP_FAIL;
        JsonDocument doc;
        doc["initialized"] = _initialized;
        doc["enabled"]     = _mcpEnabled;
        doc["readOnly"]    = _mcpReadOnly;
        doc["toolCount"]   = _registry.size();
        std::string output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // --- GET /api/mcp/config — MCP runtime configuration ---
    server.on("/api/mcp/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        // --- Guard: bearer token auth (WO6) ---
        if (_authManager && !_authManager->checkMcpAuth(request, response)) return ESP_FAIL;
        JsonDocument doc;
        doc["enabled"]     = _mcpEnabled;
        doc["readOnly"]    = _mcpReadOnly;
        if (_wifiManager) {
            doc["hostname"] = _wifiManager->getHostname();
        }
        std::string output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // --- POST /api/mcp/config — Runtime toggle for enabled/readOnly ---
    server.on("/api/mcp/config", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            // --- Guard: bearer token auth (WO6) ---
            if (_authManager && !_authManager->checkMcpAuth(request, response)) return ESP_FAIL;

            if (request->contentLength() > 512) {
                response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
                return ESP_FAIL;
            }

            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"Missing body\"}");
                return ESP_FAIL;
            }

            // --- Step 1: Parse the JSON body ---
            JsonDocument inDoc;
            DeserializationError err = deserializeJson(inDoc, body);

            if (err) {
                response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return ESP_FAIL;
            }

            // --- Step 2: Apply requested changes ---
            bool changed = false;
            if (inDoc["enabled"].is<bool>()) {
                bool newEnabled = inDoc["enabled"].as<bool>();
                if (newEnabled != _mcpEnabled) {
                    _mcpEnabled = newEnabled;
                    changed = true;
                    ESP_LOGI(TAG, "MCP server %s via runtime toggle",
                             _mcpEnabled ? "enabled" : "disabled");
                }
            }
            if (inDoc["readOnly"].is<bool>()) {
                bool newReadOnly = inDoc["readOnly"].as<bool>();
                if (newReadOnly != _mcpReadOnly) {
                    _mcpReadOnly = newReadOnly;
                    changed = true;
                    ESP_LOGI(TAG, "MCP read-only mode %s via runtime toggle",
                             _mcpReadOnly ? "enabled" : "disabled");
                }
            }

            // --- Step 3: Persist via ConfigManager if anything changed ---
            if (changed && _configManager) {
                _configManager->saveModuleConfig(this);
            }

            // --- Step 4: Return updated state ---
            JsonDocument outDoc;
            outDoc["enabled"]  = _mcpEnabled;
            outDoc["readOnly"] = _mcpReadOnly;
            std::string output;
            serializeJson(outDoc, output);
            response->send(200, "application/json", output.c_str());
            return ESP_OK;
        }
    );
}

// ---------------------------------------------------------------------------
// IConfigurable Implementation
// ---------------------------------------------------------------------------

/**
 * @brief Return the config namespace identifier.
 * @return "mcp" — config file stored at /mod/mcp.json.
 */
const char* MCPServer::getConfigNamespace() const {
    return "mcp";
}

/**
 * @brief Return the config schema version.
 * @return 1 (initial schema: enabled + readOnly).
 */
int MCPServer::getSchemaVersion() const {
    return 1;
}

/**
 * @brief Apply persisted MCP settings from a JSON config section.
 *
 * Called by ConfigManager during beginAll() to restore runtime toggles.
 * Missing keys fall back to secure defaults.
 */
void MCPServer::applyConfig(const JsonObject& section) {
    _mcpEnabled  = section["enabled"]  | true;   // Default: server enabled
    _mcpReadOnly = section["readOnly"] | true;    // Default: secure read-only
    ESP_LOGI(TAG, "Config applied: enabled=%d readOnly=%d", _mcpEnabled, _mcpReadOnly);
}

/**
 * @brief Serialize current MCP runtime settings for persistence.
 */
void MCPServer::serializeConfig(JsonObject& section) const {
    section["enabled"]  = _mcpEnabled;
    section["readOnly"] = _mcpReadOnly;
}

/**
 * @brief Reset MCP settings to secure factory defaults.
 *
 * Server enabled, read-only mode on (secure-by-default).
 */
void MCPServer::resetConfig() {
    _mcpEnabled  = true;
    _mcpReadOnly = true;
    ESP_LOGI(TAG, "Config reset to factory defaults");
}
