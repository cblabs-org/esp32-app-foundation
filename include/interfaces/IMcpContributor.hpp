/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/IMcpContributor.hpp
 * Description: Pure interface for kernel modules and optional modules that
 *              contribute MCP tools to the McpServer via the McpToolRegistry.
 *              The MCP server calls registerMcpTools() once during begin().
 *
 * Exported Classes:
 * - IMcpContributor: Abstract base for modules exposing MCP tools
 *   - registerMcpTools(): Register all tools with the provided McpToolRegistry
 */

#pragma once

// Forward-declare to avoid pulling in the full McpToolRegistry header
// in interface-only files. Concrete implementations include the full header.
class McpToolRegistry;

/**
 * @brief Contract for modules that expose MCP tools to the McpServer.
 *
 * Implementing modules call registry.addTool() inside registerMcpTools()
 * to install their tool handlers. The McpServer passes its McpToolRegistry
 * reference during the begin() phase, before the server starts processing
 * JSON-RPC requests.
 */
class IMcpContributor {
public:
    virtual ~IMcpContributor() = default;

    /**
     * @brief Register all MCP tools this module provides.
     *
     * Contributors MUST declare mutatesState = true on any tool that writes
     * data, changes device state, or triggers actions. When the MCP server
     * is in read-only mode, tools with mutatesState = true are blocked with
     * error code -32000.
     *
     * @param registry The McpToolRegistry to register tools with.
     */
    virtual void registerMcpTools(McpToolRegistry& registry) = 0;
};
