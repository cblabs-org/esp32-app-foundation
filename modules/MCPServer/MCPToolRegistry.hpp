/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <string>
#include <vector>

/**
 * @brief Static function pointer type for MCP tool handlers.
 *
 * Replaces std::function to avoid hidden heap allocations from lambda
 * captures exceeding the Small Buffer Optimisation threshold (GRAVE-05).
 * The context pointer carries any state the handler needs (typically a
 * module's `this` pointer, passed through as an opaque void*).
 *
 * @param context Opaque pointer passed through from MCPToolEntry::context.
 * @param params  JSON object containing the tool's input parameters.
 * @param result  JSON object to populate with the tool's output.
 */
using MCPToolHandler = void(*)(void* context, JsonObject& params, JsonObject& result);

/**
 * @brief Descriptor for a single MCP tool: name, schema, and handler callback.
 */
struct MCPToolEntry {
    std::string name;              ///< Tool identifier
    std::string description;       ///< Human-readable description
    JsonDocument inputSchema;      ///< JSON Schema for params
    MCPToolHandler handler;        ///< Static function pointer (never null after registration)
    void* context = nullptr;       ///< Opaque context for the handler
    bool mutatesState = false;     ///< Whether the tool modifies state
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
     * @param handler     Static function pointer invoked on tools/call dispatch.
     * @param context     Opaque pointer forwarded to the handler as its first argument.
     * @param mutatesState Whether the tool modifies device state.
     */
    void addTool(const char* name,
                 const char* description,
                 MCPToolHandler handler,
                 void* context = nullptr,
                 bool mutatesState = false);

    /**
     * @brief Register a tool with an explicit JSON input schema.
     * @param name        Unique tool identifier.
     * @param description Human-readable description of the tool.
     * @param inputSchema JSON Schema document describing accepted parameters.
     * @param handler     Static function pointer invoked on tools/call dispatch.
     * @param context     Opaque pointer forwarded to the handler as its first argument.
     * @param mutatesState Whether the tool modifies device state.
     */
    void addTool(const char* name,
                 const char* description,
                 const JsonDocument& inputSchema,
                 MCPToolHandler handler,
                 void* context = nullptr,
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
