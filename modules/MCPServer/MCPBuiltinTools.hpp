/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/mcpServer/MCPBuiltinTools.hpp
 * Description: Declaration of the built-in MCP tool registration function.
 *              These foundation-level tools (system_info, get_config, set_config,
 *              reboot, get_logs, list_tools) are always available
 *              regardless of which application uses the framework. The MCPServer
 *              calls registerAll() during begin() before any contributor registration.
 *
 *              Convention: Any built-in tool that writes data, modifies device
 *              state, or triggers an action (e.g. reboot, set_config) MUST pass
 *              mutatesState = true when calling registry.addTool(). Read-only
 *              query tools leave the default (false).
 *
 * Exported Functions:
 * - mcpBuiltinTools::registerAll(): Register all foundation-level MCP tools
 */

#pragma once

// Forward declarations — avoid pulling in full headers for interface-only usage
class MCPToolRegistry;
class ConfigManager;
class TimeManager;
class DataManager;

/**
 * @brief Namespace containing built-in MCP tool registration.
 *
 * Foundation-level tools are registered into the MCPToolRegistry before any
 * IMCPContributor modules, ensuring they are always available for system
 * introspection and management.
 */
namespace mcpBuiltinTools {

    /**
     * @brief Register all foundation-level MCP tools into the registry.
     *
     * Registers 7 tools: system_info, get_config, set_config, reboot,
     * get_logs, get_time, list_tools.
     *
     * @param registry      The MCPToolRegistry to register tools into.
     * @param configManager Pointer to the ConfigManager (nullable if unavailable).
     * @param timeManager   Pointer to the TimeManager (nullable if unavailable).
     * @param dataManager   Pointer to the DataManager (nullable if unavailable).
     */
    void registerAll(MCPToolRegistry& registry,
                     ConfigManager* configManager = nullptr,
                     TimeManager* timeManager = nullptr,
                     DataManager* dataManager = nullptr);

} // namespace mcpBuiltinTools
