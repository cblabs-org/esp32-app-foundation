/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/ConfigManager/ConfigRoot.hpp
 * Description: Central configuration root path accessor and path construction helpers.
 *              Abstracts system (/etc) and module (/mod) path locations for
 *              layout-agnostic storage resolution.
 *
 * Exported Classes:
 * - ConfigRoot: Static path accessor utility class
 *   - setMountBase():     Sets the detected mount point base
 *   - getEtcPath():       Returns system config directory root (e.g. "/config/etc")
 *   - getModPath():       Returns module config directory root (e.g. "/config/mod")
 *   - getEtcFilePath():   Constructs path for file in system config directory
 *   - getModFilePath():   Constructs path for module namespace config file
 */

#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Central configuration root path accessor and path construction helpers.
 *
 * Provides static accessors for configuration root directories (/etc, /mod) and
 * helper functions for building path strings across modules. In WO2 this class
 * will be extended to dynamically resolve paths based on partition mount state.
 */
class ConfigRoot {
public:
    /**
     * @brief Set the active mount base dynamically (e.g. "/config" or "/littlefs").
     * @param base The base path.
     */
    static void setMountBase(const char* base);

    /**
     * @brief Get the system configuration directory root path.
     * @return Const string pointer to system config root.
     */
    static const char* getEtcPath();

    /**
     * @brief Get the module configuration directory root path.
     * @return Const string pointer to module config root.
     */
    static const char* getModPath();

    /**
     * @brief Construct absolute path for a system configuration file.
     *
     * @param filename  Filename (e.g. "server.key", "keys.json").
     * @param buf       Output char buffer.
     * @param size      Buffer size in bytes.
     */
    static void getEtcFilePath(const char* filename, char* buf, size_t size);

    /**
     * @brief Construct absolute path for a system configuration file as an Arduino String.
     *
     * @param filename  Filename (e.g. "server.key", "keys.json").
     * @return Arduino String containing full path.
     */
    static String getEtcFilePath(const char* filename);

    /**
     * @brief Construct absolute path for a module configuration file (/mod/<ns>.json).
     *
     * @param ns    Module namespace string (e.g. "wifi").
     * @param buf   Output char buffer.
     * @param size  Buffer size in bytes.
     */
    static void getModFilePath(const char* ns, char* buf, size_t size);

    /**
     * @brief Construct absolute path for a module configuration file as an Arduino String.
     *
     * @param ns  Module namespace string (e.g. "wifi").
     * @return Arduino String containing full path.
     */
    static String getModFilePath(const char* ns);
};
