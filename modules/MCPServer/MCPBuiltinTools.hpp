/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/MCPServer/MCPBuiltinTools.hpp
 * Description: Declaration of the built-in MCP tool registration function.
 *              These foundation-level tools (system_info, get_config, set_config,
 *              reboot, get_logs, list_tools, capture_display) are always available
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

#include "../../include/types/DisplayInfo.hpp"

// Forward declarations — avoid pulling in full headers for interface-only usage
class MCPToolRegistry;
class ConfigManager;

/**
 * @brief Namespace containing built-in MCP tool registration.
 *
 * Foundation-level tools are registered into the MCPToolRegistry before any
 * IMCPContributor modules, ensuring they are always available for system
 * introspection and management.
 */
namespace mcpBuiltinTools {

    // DisplayInfo is now defined in include/types/DisplayInfo.hpp

    /**
     * @brief Register all foundation-level MCP tools into the registry.
     *
     * Registers 7 tools: system_info, get_config, set_config, reboot,
     * get_logs, list_tools, and capture_display. The capture_display tool
     * is null-safe — it returns a descriptive error if no display is registered.
     *
     * @param registry      The MCPToolRegistry to register tools into.
     * @param configManager Pointer to the ConfigManager (nullable if unavailable).
     * @param displayInfo   Optional display descriptor (nullable if no display).
     */
    void registerAll(MCPToolRegistry& registry,
                     ConfigManager* configManager,
                     const DisplayInfo* displayInfo = nullptr);

    /**
     * @brief Update the display info after initial registration.
     *
     * Allows late-binding the framebuffer pointer when the display module
     * initialises after MCPServer::begin(). The capture_display tool
     * reads from this stored info on each invocation.
     *
     * @param info The populated display descriptor.
     */
    void setDisplayInfo(const DisplayInfo& info);

} // namespace mcpBuiltinTools
