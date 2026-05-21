/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/mcpServer/mcpToolRegistry.cpp
 * Description: Implementation of the McpToolRegistry tool registration and
 *              lookup methods. Tools are stored in a flat vector; lookup is
 *              by linear scan (tool count is expected to remain small).
 *
 * Exported Functions/Classes:
 * - McpToolRegistry: see mcpToolRegistry.hpp
 */

#include "mcpToolRegistry.hpp"
#include <cstring>

/**
 * @brief Register a tool with no explicit schema — creates a default empty one.
 */
void McpToolRegistry::addTool(const char* name,
                              const char* description,
                              std::function<void(JsonObject& params, JsonObject& result)> handler,
                              bool mutatesState) {
    // Build a minimal JSON Schema: { "type": "object", "properties": {} }
    JsonDocument defaultSchema;
    defaultSchema["type"] = "object";
    defaultSchema["properties"].to<JsonObject>();

    addTool(name, description, defaultSchema, std::move(handler), mutatesState);
}

/**
 * @brief Register a tool with an explicit JSON input schema.
 */
void McpToolRegistry::addTool(const char* name,
                              const char* description,
                              const JsonDocument& inputSchema,
                              std::function<void(JsonObject& params, JsonObject& result)> handler,
                              bool mutatesState) {
    McpToolEntry entry;
    entry.name        = name;
    entry.description = description;
    entry.handler     = std::move(handler);
    entry.mutatesState = mutatesState;

    // Deep-copy the schema document into the entry
    entry.inputSchema.set(inputSchema);

    _tools.push_back(std::move(entry));
}

/**
 * @brief Linear search for a tool by name.
 * @return Pointer to the matching entry, or nullptr if not found.
 */
const McpToolEntry* McpToolRegistry::findTool(const char* name) const {
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
const std::vector<McpToolEntry>& McpToolRegistry::getTools() const {
    return _tools;
}

/**
 * @brief Return the number of registered tools.
 */
size_t McpToolRegistry::size() const {
    return _tools.size();
}
