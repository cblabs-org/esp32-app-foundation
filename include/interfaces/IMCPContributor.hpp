/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IMCPContributor.hpp
 * Description: Pure interface for kernel modules and optional modules that
 *              contribute MCP tools to the MCPServer via the MCPToolRegistry.
 *              The MCP server calls registerMCPTools() once during begin().
 *
 * Exported Classes:
 * - IMCPContributor: Abstract base for modules exposing MCP tools
 *   - registerMCPTools(): Register all tools with the provided MCPToolRegistry
 */

#pragma once

// Forward-declare to avoid pulling in the full MCPToolRegistry header
// in interface-only files. Concrete implementations include the full header.
class MCPToolRegistry;

/**
 * @brief Contract for modules that expose MCP tools to the MCPServer.
 *
 * Implementing modules call registry.addTool() inside registerMCPTools()
 * to install their tool handlers. The MCPServer passes its MCPToolRegistry
 * reference during the begin() phase, before the server starts processing
 * JSON-RPC requests.
 */
class IMCPContributor {
public:
    /** @brief Destructor for IMCPContributor. */
    virtual ~IMCPContributor() = default;

    /**
     * @brief Register all MCP tools this module provides.
     *
     * Contributors MUST declare mutatesState = true on any tool that writes
     * data, changes device state, or triggers actions. When the MCP server
     * is in read-only mode, tools with mutatesState = true are blocked with
     * error code -32000.
     *
     * @param registry The MCPToolRegistry to register tools with.
     */
    virtual void registerMCPTools(MCPToolRegistry& registry) = 0;
};
