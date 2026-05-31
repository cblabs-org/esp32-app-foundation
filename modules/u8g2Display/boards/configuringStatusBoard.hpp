/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/configuringStatusBoard.hpp
 * Description: System board displayed during the CONFIGURING lifecycle phase.
 *              Renders a single-line layout matching the boot board's LOADING
 *              phase: centred "Configuring..." header, a status label showing
 *              the current gate name, and a progress bar at the same position
 *              as the boot screen (Y=38, X=16, width=dw-32, height=16).
 *
 * Exported Classes:
 * - ConfiguringStatusBoard: Gate-progress display implementing IDisplayBoard
 *   - setGates():      Bind the gate list (vector of IConfigGate pointers)
 *   - dismiss():       Mark the board as dismissed (triggers expiry)
 *   - render():        Draw header, status label, and progress bar to U8G2 buffer
 *   - isExpired():     True when dismiss() has been called
 *   - getBoardName():  Returns "SYS: Configuring"
 */

#pragma once

#include <vector>
#include "interfaces/IDisplayBoard.hpp"
#include "interfaces/IConfigGate.hpp"
#include "widgets/labelWidget.hpp"
#include "widgets/progressBarWidget.hpp"

/**
 * @brief System board showing gate-by-gate progress during the CONFIGURING phase.
 *
 * Visual layout (256x64, matching boot board LOADING phase):
 * ```
 * +------------------------------------------+
 * |          Configuring...                   |  <- Header (Y=0, bold, centred)
 * |                                          |
 * |          > WiFi Station                  |  <- Status label (Y=24, centred)
 * |                                          |
 * |    [==========>              ]            |  <- Progress bar (Y=38, X=16)
 * +------------------------------------------+
 * ```
 *
 * The board does NOT own the gate list -- it holds a non-owning pointer set via
 * setGates(). The Application owns the vector and the board reads it each frame.
 */
class ConfiguringStatusBoard : public IDisplayBoard {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    ConfiguringStatusBoard(uint16_t displayWidth, uint16_t displayHeight);

    /**
     * @brief Bind the gate list for rendering.
     *
     * The board reads this vector each frame to determine gate status. The
     * caller retains ownership of the vector and its contents. Pass nullptr
     * to unbind.
     *
     * @param gates Pointer to the Application's gate vector (non-owning).
     */
    void setGates(const std::vector<IConfigGate*>* gates);

    /**
     * @brief Mark the board as dismissed, causing isExpired() to return true.
     *
     * Called when the app transitions to RUNNING and the configuring board
     * should be removed from the display.
     */
    void dismiss();

    /**
     * @brief Render the configuring status screen to the U8G2 display buffer.
     *
     * Draws: (1) "Configuring..." header (centred), (2) status label showing
     * the first unsatisfied actionable gate name, (3) progress bar at Y=38.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the board has been dismissed.
     * @return true when dismiss() has been called.
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Configuring"
     */
    const char* getBoardName() const override;

    /**
     * @brief Return the count of satisfied gates (for testing).
     * @return Number of gates where isSatisfied() == true, or 0 if no gates bound.
     */
    int getSatisfiedCount() const;

    /**
     * @brief Return the total number of bound gates (for testing).
     * @return Total gate count, or 0 if no gates bound.
     */
    int getTotalGateCount() const;

private:
    uint16_t _displayWidth;                               ///< Display width in pixels
    uint16_t _displayHeight;                              ///< Display height in pixels
    bool     _dismissed = false;                          ///< True after dismiss()

    const std::vector<IConfigGate*>* _gates = nullptr;    ///< Non-owning pointer to gate list

    // --- Widgets ---
    labelWidget       _headerLabel;                       ///< "Configuring..." header (centred)
    labelWidget       _statusLabel;                       ///< Gate name status text (centred)
    progressBarWidget _progressBar;                       ///< Gate satisfaction progress bar
};
