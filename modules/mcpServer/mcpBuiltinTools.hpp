/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/mcpServer/mcpBuiltinTools.hpp
 * Description: Declaration of the built-in MCP tool registration function.
 *              These foundation-level tools (system_info, get_config, set_config,
 *              reboot, get_logs, list_tools, capture_display) are always available
 *              regardless of which application uses the framework. The McpServer
 *              calls registerAll() during begin() before any contributor registration.
 *
 * Exported Functions:
 * - mcpBuiltinTools::registerAll(): Register all foundation-level MCP tools
 *
 * Exported Structs:
 * - mcpBuiltinTools::DisplayInfo: Optional display descriptor for capture_display
 */

#pragma once

#include <cstddef>
#include <cstdint>

// Forward declarations — avoid pulling in full headers for interface-only usage
class McpToolRegistry;
class ConfigManager;

/**
 * @brief Namespace containing built-in MCP tool registration.
 *
 * Foundation-level tools are registered into the McpToolRegistry before any
 * IMcpContributor modules, ensuring they are always available for system
 * introspection and management.
 */
namespace mcpBuiltinTools {

    /**
     * @brief Optional descriptor for a framebuffer-based display.
     *
     * Decoupled from any specific display driver (e.g. U8G2). The Application
     * hub populates this struct if a display module is registered, otherwise
     * capture_display gracefully returns an error indicating no display.
     */
    struct DisplayInfo {
        const uint8_t* framebuffer = nullptr;  ///< Pointer to the raw framebuffer bytes
        size_t framebufferSize     = 0;        ///< Total size of the framebuffer in bytes
        uint16_t width             = 0;        ///< Display width in pixels
        uint16_t height            = 0;        ///< Display height in pixels
    };

    /**
     * @brief Register all foundation-level MCP tools into the registry.
     *
     * Registers 7 tools: system_info, get_config, set_config, reboot,
     * get_logs, list_tools, and capture_display. The capture_display tool
     * is null-safe — it returns a descriptive error if no display is registered.
     *
     * @param registry      The McpToolRegistry to register tools into.
     * @param configManager Pointer to the ConfigManager (nullable if unavailable).
     * @param displayInfo   Optional display descriptor (nullable if no display).
     */
    void registerAll(McpToolRegistry& registry,
                     ConfigManager* configManager,
                     const DisplayInfo* displayInfo = nullptr);

} // namespace mcpBuiltinTools
