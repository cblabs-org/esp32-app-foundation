/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/messageBoardBase.hpp
 * Description: Shared base class for boards that display centred multi-line
 *              messages via systemMessageWidget composition. Provides the
 *              common dismiss/render/isExpired plumbing. Subclasses only need
 *              to implement getBoardName() and their domain-specific API.
 *
 * Exported Classes:
 * - MessageBoardBase: [Abstract] Common base for message-style display boards.
 *   - dismiss():     Mark the board as dismissed (triggers expiry).
 *   - render():      Delegate rendering to the composed systemMessageWidget.
 *   - isExpired():   Return true when the board has been dismissed.
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/systemMessageWidget.hpp"

/**
 * @brief Abstract base class for message-style display boards.
 *
 * Extracts the common pattern shared by ErrorBoard and WiFiWizardBoard:
 * a single systemMessageWidget{0, 0, 128, 64} with dismiss-based expiry
 * and delegated rendering. Subclasses configure the widget content via
 * the protected _msgWidget member and override getBoardName().
 */
class MessageBoardBase : public IDisplayBoard {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    MessageBoardBase(uint16_t displayWidth, uint16_t displayHeight)
        : _msgWidget(0, 0, static_cast<int>(displayWidth), static_cast<int>(displayHeight))
    {}

    /**
     * @brief Mark the board as dismissed, causing isExpired() to return true.
     */
    void dismiss();

    /**
     * @brief Render the message screen into the U8G2 display buffer.
     *
     * Delegates to the composed systemMessageWidget. Guarded for native builds.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the board has been dismissed.
     * @return true when dismiss() has been called.
     */
    bool isExpired() const override;

protected:
    /**
     * @brief Reset the dismissed state (e.g. when new content is set).
     */
    void resetDismissed();

    // --- Widget composition ---
    systemMessageWidget _msgWidget;                     ///< Centred multi-line message display
    bool _dismissed = false;                            ///< True after dismiss() — triggers expiry
};
