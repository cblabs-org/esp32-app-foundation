/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/MCPServer/MCPToolRegistry.cpp
 * Description: Implementation of the MCPToolRegistry tool registration and
 *              lookup methods. Tools are stored in a flat vector; lookup is
 *              by linear scan (tool count is expected to remain small).
 *
 * Exported Functions/Classes:
 * - MCPToolRegistry: see MCPToolRegistry.hpp
 */

#include "MCPToolRegistry.hpp"
#include <cstring>

/**
 * @brief Register a tool with no explicit schema — creates a default empty one.
 */
void MCPToolRegistry::addTool(const char* name,
                              const char* description,
                              MCPToolHandler handler,
                              void* context,
                              bool mutatesState) {
    // Build a minimal JSON Schema: { "type": "object", "properties": {} }
    JsonDocument defaultSchema;
    defaultSchema["type"] = "object";
    defaultSchema["properties"].to<JsonObject>();

    addTool(name, description, defaultSchema, handler, context, mutatesState);
}

/**
 * @brief Register a tool with an explicit JSON input schema.
 */
void MCPToolRegistry::addTool(const char* name,
                              const char* description,
                              const JsonDocument& inputSchema,
                              MCPToolHandler handler,
                              void* context,
                              bool mutatesState) {
    MCPToolEntry entry;
    entry.name        = name;
    entry.description = description;
    entry.handler     = handler;
    entry.context     = context;
    entry.mutatesState = mutatesState;

    // Deep-copy the schema document into the entry
    entry.inputSchema.set(inputSchema);

    _tools.push_back(std::move(entry));
}

/**
 * @brief Linear search for a tool by name.
 * @return Pointer to the matching entry, or nullptr if not found.
 */
const MCPToolEntry* MCPToolRegistry::findTool(const char* name) const {
    for (const auto& tool : _tools) {
        if (tool.name == name) {
            return &tool;
        }
    }
    return nullptr;
}

/**
 * @brief Return const reference to the full tool vector.
 */
const std::vector<MCPToolEntry>& MCPToolRegistry::getTools() const {
    return _tools;
}

/**
 * @brief Return the number of registered tools.
 */
size_t MCPToolRegistry::size() const {
    return _tools.size();
}
