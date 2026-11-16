/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/ConfiguringStatusBoard.hpp
 * Description: System board displayed during the CONFIGURING lifecycle phase.
 *              Renders per-precondition status rows with cause-aware prefix
 *              indicators (* = satisfied, ~ = connecting, ! = needs user,
 *              - = blocked, ? = indeterminate, > = in progress), an animated
 *              progress bar that lerps toward the target percentage (Startup-only),
 *              and a footer with the device IP.
 *
 * Exported Classes:
 * - PreconditionSnapshot:  Precondition snapshot for display rendering
 * - ConfiguringStatusBoard: Precondition-progress display implementing IDisplayBoard
 *   - setEngine():             Bind PreconditionEngine (non-owning)
 *   - setPreconditions():      Bind direct snapshot array
 *   - updatePrecondition():    Update/insert a precondition health status
 *   - setActiveGateStatus():   Update the action status of the current gate
 *   - setActionableGateName(): Set the actionable gate/precondition name
 *   - setConnectionInfo():     Set WiFi SSID and portal URL for footer
 *   - dismiss():               Mark the board as dismissed (triggers expiry)
 *   - render():                Draw header, status rows, and progress bar
 *   - isExpired():             True when dismiss() has been called
 *   - getBoardName():          Returns "SYS: Configuring"
 *   - getStartupCount():       Count of Startup-role preconditions
 *   - getStartupMetCount():    Count of satisfied Startup-role preconditions
 *   - getSatisfiedCount():     Returns getStartupMetCount()
 *   - getTotalGateCount():     Returns total preconditions count
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include "IDisplayBoard.hpp"
#include "interfaces/IPrecondition.hpp"
#include "events/PreconditionHealthPayload.hpp"
#include "widgets/LabelWidget.hpp"
#include "widgets/ProgressBarWidget.hpp"

class PreconditionEngine;

/**
 * @brief Lightweight snapshot of a precondition's display state.
 */
struct PreconditionSnapshot {
    char             name[32] = "";
    PreconditionRole role     = PreconditionRole::Operational;
    Cause            cause    = Cause::NotConfigured;
    Severity         severity = Severity::NeedsUser;
    bool             met      = false;
    bool             everMet  = false;
};

/**
 * @brief System board showing precondition progress during the CONFIGURING phase.
 *
 * Visual layout (256x64):
 * ```
 * +------------------------------------------+
 * |          Configuring...                   |  <- Header (Y=0, bold, centred)
 * |                                          |
 * |  * WiFi Station                          |  <- Met precondition (* or ✓)
 * |  ! API Keys                              |  <- NeedsUser precondition (!)
 * |  - Time Sync                             |  <- Blocked precondition (-)
 * |                                          |
 * |    [===========>              ]           |  <- Animated progress bar (Startup only)
 * |          http://192.168.1.42             |  <- Footer (device IP)
 * +------------------------------------------+
 * ```
 *
 * Progress bar counts ONLY Startup-role preconditions (CG-14 projection).
 * Operational preconditions degrade features without blocking boot and do not
 * affect the progress bar.
 */
class ConfiguringStatusBoard : public IDisplayBoard {
public:
    static constexpr size_t MAX_DISPLAY_PRECONDITIONS = 16; ///< Maximum display preconditions

    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    ConfiguringStatusBoard(uint16_t displayWidth, uint16_t displayHeight);

    /**
     * @brief Bind a PreconditionEngine for live status reading.
     * @param engine Pointer to PreconditionEngine (non-owning, or nullptr to unbind).
     */
    void setEngine(const PreconditionEngine* engine);

    /**
     * @brief Bind a static snapshot array of preconditions (useful for testing or decoupled updates).
     * @param items Pointer to array of PreconditionSnapshot.
     * @param count Number of items in array (clamped to MAX_DISPLAY_PRECONDITIONS).
     */
    void setPreconditions(const PreconditionSnapshot* items, size_t count);

    /**
     * @brief Update or add a precondition entry from a health event.
     * @param payload Health payload containing name, cause, severity, everMet.
     * @param role Optional role override (defaults to Startup if new entry, or preserves existing).
     */
    void updatePrecondition(const PreconditionHealthPayload& payload, PreconditionRole role = PreconditionRole::Startup);

    /**
     * @brief Update or add a precondition entry by explicit parameters.
     */
    void updatePrecondition(const char* name, Cause cause, Severity severity, bool everMet = false, PreconditionRole role = PreconditionRole::Startup);

    /**
     * @brief Set the authoritative actionable gate name from the Application.
     *
     * @param gateName  Gate name from AppConfiguringPayload::gateName (non-owning).
     */
    void setActionableGateName(const char* gateName);

    /**
     * @brief Set network connection info for the footer display.
     * @param network  The WiFi network name (SSID) the device is connected to.
     * @param url      The device URL (e.g., "http://192.168.1.42").
     */
    void setConnectionInfo(const char* network, const char* url);

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
     * Draws: (1) "Configuring..." header (centred), (2) per-precondition status rows
     * with cause-aware prefix indicators, (3) animated progress bar (Startup-only),
     * (4) device IP footer.
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
     * @brief Return count of Startup preconditions.
     */
    uint8_t getStartupCount() const { return _startupCount; }

    /**
     * @brief Return count of satisfied Startup preconditions.
     */
    uint8_t getStartupMetCount() const { return _startupMetCount; }

    /**
     * @brief Return total number of bound/cached preconditions.
     */
    int getTotalPreconditions() const;

    /**
     * @brief Return count of satisfied Startup preconditions (for test compatibility).
     * @return Number of satisfied Startup preconditions.
     */
    int getSatisfiedCount() const { return static_cast<int>(_startupMetCount); }

    /**
     * @brief Return total number of bound/cached preconditions (for test compatibility).
     * @return Total precondition count.
     */
    int getTotalGateCount() const { return getTotalPreconditions(); }

    /**
     * @brief Return target progress percentage (0.0 to 100.0).
     */
    float getTargetPercent() const { return _targetPercent; }

    /**
     * @brief Return current animated progress percentage (0.0 to 100.0).
     */
    float getCurrentPercent() const { return _currentPercent; }

    /** Return the stored actionable gate name (for testing). */
    const char* getActionableGateName() const { return _actionableGateName; }

    /**
     * @brief Clear all cached snapshots.
     */
    void clear();

private:
    void _recomputeCounts();

    uint16_t _displayWidth;                               ///< Display width in pixels
    uint16_t _displayHeight;                              ///< Display height in pixels
    bool     _dismissed = false;                          ///< True after dismiss()

    const PreconditionEngine* _engine = nullptr;          ///< Non-owning pointer to live engine
    PreconditionSnapshot      _snapshots[MAX_DISPLAY_PRECONDITIONS];
    size_t                    _snapshotCount = 0;

    // --- Precondition counts (Startup-role only for progress bar) ---
    uint8_t _startupCount    = 0;                         ///< Total Startup-role preconditions
    uint8_t _startupMetCount = 0;                         ///< Met Startup-role preconditions

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

    // --- Animation frame counter for Connecting / InProgress effect ---
    uint8_t _animFrame = 0;                               ///< Frame counter for animation
};

