/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IU8G2SystemScreens.hpp
 * Description: Pluggable system screen provider for U8G2 display module.
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "events/AppConfiguringPayload.hpp"

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

    /// Custom helper modal for a specific config gate.
    ///
    /// Called when a gate's actionStatus is NeedsUserAction. The returned
    /// board pointer must be non-owning — the board must outlive the display
    /// module (typically a static or member object of the consumer's
    /// IU8G2SystemScreens implementation). The same pointer may be returned
    /// for multiple gates if the board supports reconfiguration.
    ///
    /// Return nullptr to use the built-in mapping or generic helpText fallback.
    ///
    /// The payload pointer references Application::_configuringPayload, a member
    /// variable valid for the Application's entire lifetime. The payload is stable
    /// for the duration of the CONFIGURING phase and is updated in-place on each
    /// re-publish.
    ///
    /// @param payload Full configuring event payload with gate name, help text,
    ///                action status, hostname, and device IP.
    /// @return Non-owning pointer to a custom board, or nullptr.
    virtual IDisplayBoard* gateHelperBoard(const AppConfiguringPayload* payload) {
        (void)payload;
        return nullptr;
    }
};
