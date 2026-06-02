/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/MCPServer/MCPToolRegistry.hpp
 * Description: Registry for MCP tool entries. Each tool has a name, description,
 *              JSON schema for input parameters, and a handler callback.
 *              The MCPServer populates this registry via IMCPContributor modules
 *              during begin(), then uses it for tools/list and tools/call dispatch.
 *
 * Exported Classes:
 * - MCPToolEntry: POD struct describing a single MCP tool
 *   - name:        Tool identifier string
 *   - description: Human-readable tool description
 *   - inputSchema: ArduinoJson document with JSON Schema for params
 *   - handler:     Callback invoked when tools/call targets this tool
 * - MCPToolRegistry: Collection of MCPToolEntry items with lookup
 *   - addTool():   Register a tool (with or without explicit inputSchema)
 *   - findTool():  Lookup a tool by name (returns nullptr if not found)
 *   - getTools():  Return const reference to the full tool vector
 *   - size():      Return the number of registered tools
 */

#pragma once

#include <ArduinoJson.h>
#include <functional>
#include <string>
#include <vector>

/**
 * @brief Descriptor for a single MCP tool: name, schema, and handler callback.
 */
struct MCPToolEntry {
    std::string name;                                                     ///< Tool identifier
    std::string description;                                              ///< Human-readable description
    JsonDocument inputSchema;                                             ///< JSON Schema for params
    std::function<void(JsonObject& params, JsonObject& result)> handler;  ///< Execution callback
    bool mutatesState = false;                                            ///< Whether the tool modifies state
};

/**
 * @brief Registry that collects and looks up MCP tools by name.
 */
class MCPToolRegistry {
public:
    /**
     * @brief Register a tool with no explicit input schema.
     *
     * Creates a default empty schema: { "type": "object", "properties": {} }
     *
     * @param name        Unique tool identifier.
     * @param description Human-readable description of the tool.
     * @param handler     Callback invoked on tools/call dispatch.
     */
    void addTool(const char* name,
                 const char* description,
                 std::function<void(JsonObject& params, JsonObject& result)> handler,
                 bool mutatesState = false);

    /**
     * @brief Register a tool with an explicit JSON input schema.
     * @param name        Unique tool identifier.
     * @param description Human-readable description of the tool.
     * @param inputSchema JSON Schema document describing accepted parameters.
     * @param handler     Callback invoked on tools/call dispatch.
     */
    void addTool(const char* name,
                 const char* description,
                 const JsonDocument& inputSchema,
                 std::function<void(JsonObject& params, JsonObject& result)> handler,
                 bool mutatesState = false);

    /**
     * @brief Find a tool by name.
     * @param name Tool identifier to search for.
     * @return Pointer to the matching MCPToolEntry, or nullptr if not found.
     */
    const MCPToolEntry* findTool(const char* name) const;

    /**
     * @brief Get the full list of registered tools.
     * @return Const reference to the internal tool vector.
     */
    const std::vector<MCPToolEntry>& getTools() const;

    /**
     * @brief Get the number of registered tools.
     * @return Tool count.
     */
    size_t size() const;

private:
    std::vector<MCPToolEntry> _tools; ///< Internal storage for all registered tools
};
