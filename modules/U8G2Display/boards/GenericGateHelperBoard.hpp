/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/GenericGateHelperBoard.hpp
 * Description: Fallback modal helper board for config gates that have no
 *              dedicated built-in board. Extends MessageBoardBase to display
 *              a gate name title, help text body, and a device IP footer
 *              directing the user to the web portal.
 *
 * Exported Classes:
 * - GenericGateHelperBoard: Generic config gate helper extending MessageBoardBase
 *   - configure():    Set gate name, help text, and device IP for display
 *   - getBoardName(): Returns "SYS: Gate Helper"
 */

#pragma once

#include "MessageBoardBase.hpp"

/**
 * @brief Generic fallback modal for config gates without a dedicated board.
 *
 * Extends MessageBoardBase to display three rows:
 * - Title:  gate name (e.g., "Board Configuration")
 * - Body:   help text (e.g., "Add a departure board")
 * - Footer: "http://<deviceIP>" — always the IP, never the hostname
 *
 * The board self-expires when dismiss() is called (inherited from
 * MessageBoardBase) after the gate is satisfied or superseded.
 */
class GenericGateHelperBoard : public MessageBoardBase {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    GenericGateHelperBoard(uint16_t displayWidth, uint16_t displayHeight)
        : MessageBoardBase(displayWidth, displayHeight)
    {}

    /**
     * @brief Configure the board content for a specific gate.
     *
     * Updates the inherited SystemMessageWidget with the gate name as title,
     * help text as body, and a formatted URL using the device IP as footer.
     * Resets the dismissed state so the board becomes active.
     *
     * @param gateName Gate name string (used as title row).
     * @param helpText User-facing guidance text (truncated to fit display).
     * @param deviceIP Device IP address string (formatted as "http://<IP>").
     */
    void configure(const char* gateName, const char* helpText, const char* deviceIP);

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Gate Helper"
     */
    const char* getBoardName() const override;

private:
    char _gateName[32]  = "";   ///< Gate name for title display (max 31 chars + null)
    char _helpText[64]  = "";   ///< Help text body (max 63 chars + null)
    char _deviceIP[16]  = "";   ///< Device IP address for footer URL
};
