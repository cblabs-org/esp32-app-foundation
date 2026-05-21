/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/mcpServer/mcpServer.cpp
 * Description: Implementation of the MCP JSON-RPC 2.0 server. Reads
 *              line-delimited JSON from Serial on hardware, parses each line
 *              into a JsonDocument, and dispatches to the appropriate handler
 *              (initialize, tools/list, tools/call, ping). All responses are
 *              written as serialised JSON followed by a newline delimiter.
 *
 * Exported Functions/Classes:
 * - McpServer: see mcpServer.hpp
 */

#include "mcpServer.hpp"
#include "mcpBuiltinTools.hpp"
#include "interfaces/IMcpContributor.hpp"

// ---------------------------------------------------------------------------
// Platform-conditional includes & logging shims
// Matches the Application.cpp pattern: ESP-IDF logs on hardware, printf on native.
// ---------------------------------------------------------------------------
#ifndef NATIVE_BUILD
    #include "esp_log.h"
    static const char* TAG = "McpServer"; // ESP-IDF log tag
    #define MCP_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
    #define MCP_LOGW(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
    #define MCP_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#else
    #include <cstdio>
    #define MCP_LOGI(fmt, ...) printf("[I] McpServer: " fmt "\n", ##__VA_ARGS__)
    #define MCP_LOGW(fmt, ...) printf("[W] McpServer: " fmt "\n", ##__VA_ARGS__)
    #define MCP_LOGE(fmt, ...) printf("[E] McpServer: " fmt "\n", ##__VA_ARGS__)

    // Instantiate the mock Serial object for the native build linker
    MockSerial Serial;
#endif

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/**
 * @brief Collect tools from all IMcpContributor modules and mark the server
 *        as initialised so tick() begins processing.
 */
void McpServer::begin(const std::vector<IMcpContributor*>& contributors, ConfigManager* configManager) {
    // --- Step 1: Register built-in foundation tools ---
    // These are always available regardless of which app uses the framework.
    _registryPtr = &_registry;
    mcpBuiltinTools::registerAll(_registry, configManager);

    // --- Step 2: Collect tools from all contributors ---
    for (auto* contributor : contributors) {
        if (contributor != nullptr) {
            contributor->registerMcpTools(_registry);
        }
    }

    MCP_LOGI("MCP Server started with %zu tools (%zu built-in)",
             _registry.size(), _registry.size() - 0);

    // --- Step 3: Mark initialised ---
    _initialized = true;
}

// ---------------------------------------------------------------------------
// Tick — Serial polling (hardware) / no-op (native)
// ---------------------------------------------------------------------------

/**
 * @brief Read available bytes from Serial, accumulate into a line buffer,
 *        and dispatch complete lines to _processLine().
 */
void McpServer::tick() {
    if (!_initialized) return;

#ifndef NATIVE_BUILD
    // --- Hardware: read characters from Serial until newline ---
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            _processLine(_inputBuffer.c_str());
            _inputBuffer = "";
        } else {
            _inputBuffer += c;
        }
    }
#endif
    // Native: no-op — tests call processLineForTest() or _processLine() directly
}

#ifdef NATIVE_BUILD
/**
 * @brief Test harness entry point: feed a raw JSON-RPC line for processing.
 */
void McpServer::processLineForTest(const char* line) {
    _processLine(line);
}
#endif

// ---------------------------------------------------------------------------
// JSON-RPC Dispatch
// ---------------------------------------------------------------------------

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
void McpServer::_processLine(const char* line) {
    // --- Step 1: Parse the JSON payload ---
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);

    if (err) {
        // Parse error — respond with JSON-RPC parse error code
        MCP_LOGE("JSON parse error: %s", err.c_str());
        // Parse errors have no valid id — pass null via a temporary doc
        JsonDocument nullDoc;
        nullDoc["id"] = nullptr;
        _sendError(-32700, "Parse error", nullDoc["id"], Serial);
        Serial.println();
        return;
    }

    // --- Step 2: Extract the method string ---
    const char* method = doc["method"] | static_cast<const char*>(nullptr);

    if (method == nullptr) {
        // Missing "method" field — invalid request
        _sendError(-32600, "Invalid Request: missing method", doc["id"], Serial);
        Serial.println();
        return;
    }

    // --- Step 3: Route to the appropriate handler ---
    if (strcmp(method, "initialize") == 0) {
        _handleInitialize(doc, Serial);
    } else if (strcmp(method, "tools/list") == 0) {
        _handleToolsList(doc, Serial);
    } else if (strcmp(method, "tools/call") == 0) {
        _handleToolsCall(doc, Serial);
    } else if (strcmp(method, "ping") == 0) {
        _handlePing(doc, Serial);
    } else {
        // Unknown method
        MCP_LOGW("Unknown method: %s", method);
        _sendError(-32601, "Method not found", doc["id"], Serial);
    }

    // Newline delimiter after every response
    Serial.println();
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
void McpServer::_handleInitialize(const JsonDocument& req, Print& out) {
    JsonDocument resp;
    resp["jsonrpc"] = "2.0";
    resp["id"]      = req["id"];

    JsonObject result     = resp["result"].to<JsonObject>();
    result["protocolVersion"] = "2024-11-05";

    // Server identity uses the compile-time version macro
    JsonObject serverInfo = result["serverInfo"].to<JsonObject>();
    serverInfo["name"]    = "esp32-app-foundation";
    serverInfo["version"] = APP_FOUNDATION_VERSION;

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
void McpServer::_handleToolsList(const JsonDocument& req, Print& out) {
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
void McpServer::_handleToolsCall(const JsonDocument& req, Print& out) {
    // --- Step 1: Extract tool name and arguments ---
    const char* toolName = req["params"]["name"] | static_cast<const char*>(nullptr);

    if (toolName == nullptr) {
        _sendError(-32602, "Invalid params: missing tool name", req["id"], out);
        return;
    }

    // --- Step 2: Look up the tool ---
    const McpToolEntry* tool = _registry.findTool(toolName);

    if (tool == nullptr) {
        // Build descriptive error message
        MCP_LOGW("Unknown tool: %s", toolName);
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

    tool->handler(argsObj, resultObj);

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
void McpServer::_handlePing(const JsonDocument& req, Print& out) {
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
void McpServer::_sendError(int code, const char* message, JsonVariantConst id, Print& out) {
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
