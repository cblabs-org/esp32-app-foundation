/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/ConfiguringStatusBoard.hpp
 * Description: System board displayed during the CONFIGURING lifecycle phase.
 *              Renders per-gate status rows with prefix indicators (* = satisfied,
 *              > = actionable, - = blocked), an animated progress bar that lerps
 *              toward the target percentage, and a footer with the device IP.
 *
 * Exported Classes:
 * - ConfiguringStatusBoard: Gate-progress display implementing IDisplayBoard
 *   - setGates():              Bind the gate list (vector of IConfigGate pointers)
 *   - setActiveGateStatus():   Update the action status of the current gate
 *   - dismiss():               Mark the board as dismissed (triggers expiry)
 *   - render():                Draw header, status rows, and progress bar
 *   - isExpired():             True when dismiss() has been called
 *   - getBoardName():          Returns "SYS: Configuring"
 */

#pragma once

#include <vector>
#include "interfaces/IDisplayBoard.hpp"
#include "interfaces/IConfigGate.hpp"
#include "widgets/LabelWidget.hpp"
#include "widgets/ProgressBarWidget.hpp"

/**
 * @brief System board showing gate-by-gate progress during the CONFIGURING phase.
 *
 * Visual layout (256x64):
 * ```
 * +------------------------------------------+
 * |          Configuring...                   |  <- Header (Y=0, bold, centred)
 * |                                          |
 * |  * WiFi Station                          |  <- Satisfied gate row
 * |  > API Keys                              |  <- Actionable gate row
 * |  - Time Sync                             |  <- Blocked gate row
 * |                                          |
 * |    [===========>              ]           |  <- Animated progress bar
 * |          http://192.168.1.42             |  <- Footer (device IP)
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
     * @brief Update the action status of the current actionable gate.
     *
     * Used by the display module to communicate the gate's action status
     * from the AppConfiguringPayload. Affects the prefix character for
     * the active gate row (> for NeedsUserAction, ~ for InProgress).
     *
     * @param status The GateActionStatus for the currently actionable gate.
     */
    void setActiveGateStatus(GateActionStatus status);

    /**
     * @brief Set the authoritative actionable gate name from the Application.
     *
     * Called by U8G2DisplayModule::_onAppConfiguring() alongside
     * setActiveGateStatus(). Eliminates the need for render() to
     * re-derive the actionable gate by re-walking the dependency graph.
     *
     * @param gateName  Gate name from AppConfiguringPayload::gateName (non-owning).
     */
    void setActionableGateName(const char* gateName);

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
     * Draws: (1) "Configuring..." header (centred), (2) per-gate status rows
     * with prefix indicators, (3) animated progress bar, (4) device IP footer.
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

    /**
     * @brief Set network connection info for the footer display.
     * @param network  The WiFi network name (SSID) the device is connected to.
     * @param url      The device URL (e.g., "http://192.168.1.42").
     */
    void setConnectionInfo(const char* network, const char* url);

    /**
     * @brief Return the current active gate status (for testing).
     * @return The GateActionStatus set via setActiveGateStatus().
     */
    GateActionStatus getActiveGateStatus() const { return _activeGateStatus; }

    /** Return the stored actionable gate name (for testing). */
    const char* getActionableGateName() const { return _actionableGateName; }

private:
    uint16_t _displayWidth;                               ///< Display width in pixels
    uint16_t _displayHeight;                              ///< Display height in pixels
    bool     _dismissed = false;                          ///< True after dismiss()

    const std::vector<IConfigGate*>* _gates = nullptr;    ///< Non-owning pointer to gate list

    // --- Gate action status ---
    GateActionStatus _activeGateStatus = GateActionStatus::NeedsUserAction; ///< Current actionable gate status
    const char* _actionableGateName = nullptr;  ///< Authoritative actionable gate name (from Application via event)

    // --- Animated progress ---
    float _currentPercent = 0.0f;                         ///< Current displayed progress (lerped)
    float _targetPercent  = 0.0f;                         ///< Target progress percentage

    // --- Widgets ---
    LabelWidget       _headerLabel;                       ///< "Configuring..." header (centred)
    LabelWidget       _statusLabel;                       ///< Gate name status text (centred)
    ProgressBarWidget _progressBar;                       ///< Gate satisfaction progress bar

    char _networkName[33] = "";                            ///< Connected WiFi network name
    char _deviceUrl[48] = "";                              ///< Device URL for portal access

    // --- Animation frame counter for InProgress effect ---
    uint8_t _animFrame = 0;                               ///< Frame counter for > / ~ alternation
};

