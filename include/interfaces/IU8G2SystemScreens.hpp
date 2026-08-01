/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/IU8G2SystemScreens.hpp
 * Description: Pluggable system screen provider for U8G2 display module.
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"

/**
 * @brief Pluggable system screen provider for U8G2 display module.
 *
 * Applications implement this to substitute custom IDisplayBoard
 * implementations for framework system states. Each method returns
 * nullptr to use the built-in default screen.
 *
 * Pattern matches ILVGLSystemScreens — same opt-in override model.
 */
class IU8G2SystemScreens {
public:
    virtual ~IU8G2SystemScreens() = default;

    /// Custom error board, or nullptr for built-in ErrorBoard.
    virtual IDisplayBoard* errorBoard()        { return nullptr; }

    /// Custom OTA progress board, or nullptr for built-in OTAProgressBoard.
    virtual IDisplayBoard* otaProgressBoard()  { return nullptr; }

    /// Custom configuring board, or nullptr for built-in ConfiguringStatusBoard.
    virtual IDisplayBoard* configuringBoard()  { return nullptr; }

    /// Custom WiFi wizard board, or nullptr for built-in WiFiWizardBoard.
    virtual IDisplayBoard* wifiWizardBoard()   { return nullptr; }
};
