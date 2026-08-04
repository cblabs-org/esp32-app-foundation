/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/boards/configuringStatusBoard.cpp
 * Description: Implementation of the ConfiguringStatusBoard. Renders a centred
 *              header, per-gate status rows with prefix indicators, an animated
 *              progress bar that lerps toward the target percentage, and a
 *              device IP footer during the CONFIGURING lifecycle phase.
 *
 * Exported Functions:
 * - ConfiguringStatusBoard::ConfiguringStatusBoard(): Layout widgets for display
 * - ConfiguringStatusBoard::setGates():              Bind gate list pointer
 * - ConfiguringStatusBoard::setActiveGateStatus():   Update current gate action status
 * - ConfiguringStatusBoard::dismiss():               Mark dismissed for expiry
 * - ConfiguringStatusBoard::render():                Draw header, status rows, progress bar, footer
 * - ConfiguringStatusBoard::isExpired():             Return dismissed state
 * - ConfiguringStatusBoard::getBoardName():          Return "SYS: Configuring"
 * - ConfiguringStatusBoard::getSatisfiedCount():     Return count of satisfied gates
 * - ConfiguringStatusBoard::getTotalGateCount():     Return total bound gate count
 */

#include "ConfiguringStatusBoard.hpp"
#include <cstring>
#include <cstdio>

#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/Fonts.hpp"

// =============================================================================
// Construction
// =============================================================================

/**
 * @brief Construct the configuring status board, laying out widgets.
 *
 * Layout (256x64 example):
 *   Row 0-11:  Header "Configuring..." (bold, centred)
 *   Rows:      Per-gate status rows (small font, left-aligned with prefix)
 *   Bottom:    Animated progress bar + device IP footer
 *
 * @param displayWidth  Display width in pixels.
 * @param displayHeight Display height in pixels.
 */
ConfiguringStatusBoard::ConfiguringStatusBoard(uint16_t displayWidth, uint16_t displayHeight)
    : _displayWidth(displayWidth),
      _displayHeight(displayHeight),
      _headerLabel(0, 0, static_cast<int>(displayWidth), -1),
      _statusLabel(0, 24, static_cast<int>(displayWidth), -1),
      _progressBar(16, 38, static_cast<int>(displayWidth) - 32, 16, nullptr)
{
    _headerLabel.setText("Configuring...");
    _headerLabel.setAlignment(1); // CENTER
    _statusLabel.setAlignment(1); // CENTER
}

// =============================================================================
// Public API
// =============================================================================

/**
 * @brief Bind the gate list for rendering.
 * @param gates Pointer to the Application's gate vector (non-owning).
 */
void ConfiguringStatusBoard::setGates(const std::vector<IConfigGate*>* gates) {
    _gates = gates;
}

/**
 * @brief Update the action status of the current actionable gate.
 * @param status The GateActionStatus for the currently actionable gate.
 */
void ConfiguringStatusBoard::setActiveGateStatus(GateActionStatus status) {
    _activeGateStatus = status;
}

/**
 * @brief Set network connection info for the footer display.
 * @param network  The WiFi network name (SSID) the device is connected to.
 * @param url      The device URL (e.g., "http://192.168.1.42").
 */
void ConfiguringStatusBoard::setConnectionInfo(const char* network, const char* url) {
    if (network) {
        strncpy(_networkName, network, sizeof(_networkName) - 1);
        _networkName[sizeof(_networkName) - 1] = '\0';
    }
    if (url) {
        strncpy(_deviceUrl, url, sizeof(_deviceUrl) - 1);
        _deviceUrl[sizeof(_deviceUrl) - 1] = '\0';
    }
}

/**
 * @brief Mark the board as dismissed, causing isExpired() to return true.
 */
void ConfiguringStatusBoard::dismiss() {
    _dismissed = true;
}

/**
 * @brief Render the configuring status screen to the U8G2 display buffer.
 *
 * Layout:
 *   1. Header "Configuring..." at Y=0 (bold, centred)
 *   2. Per-gate status rows with prefix indicators:
 *      - '*' = satisfied
 *      - '>' = actionable (NeedsUserAction) — alternates with '~' for InProgress
 *      - '-' = blocked (dependencies not met)
 *   3. Animated progress bar (lerps toward target)
 *   4. Device IP footer
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ConfiguringStatusBoard::render(U8G2& u8g2) {
    // --- 1. Draw header (centred, bold) ---
    _headerLabel.setFont(AppFonts::defaultLarge());
    _headerLabel.render(u8g2);

    // --- 2. Compute gate status and draw per-gate rows ---
    if (_gates != nullptr && !_gates->empty()) {
        const int gateCount = static_cast<int>(_gates->size());
        int satisfiedCount = 0;

        // --- Count satisfied gates and determine actionable gate ---
        // We compute the actionable gate name from the gate list itself for
        // the status rows. The _activeGateStatus from the payload controls
        // the prefix character for the active row.
        const char* actionableGateName = nullptr;

        for (int i = 0; i < gateCount; i++) {
            const IConfigGate* gate = (*_gates)[i];
            if (gate->isSatisfied()) {
                satisfiedCount++;
            } else if (actionableGateName == nullptr) {
                // Check if this gate's dependencies are all satisfied
                uint8_t depCount = 0;
                const char* const* deps = gate->getDependencies(depCount);
                bool depsOk = true;
                for (uint8_t d = 0; d < depCount; d++) {
                    bool found = false;
                    for (int j = 0; j < gateCount; j++) {
                        if (strcmp((*_gates)[j]->getGateName(), deps[d]) == 0) {
                            if (!(*_gates)[j]->isSatisfied()) depsOk = false;
                            found = true;
                            break;
                        }
                    }
                    if (!found || !depsOk) { depsOk = false; break; }
                }
                if (depsOk) {
                    actionableGateName = gate->getGateName();
                }
            }
        }

        // --- Increment animation frame for InProgress effect ---
        _animFrame++;

        // --- Draw per-gate status rows ---
        u8g2.setFont(AppFonts::defaultSmall());
        const int rowHeight = 10;
        const int startY = 16;
        const int maxRows = (_displayHeight > 48) ? 3 : 2; // Limit visible rows on small displays
        int rowsDrawn = 0;

        for (int i = 0; i < gateCount && rowsDrawn < maxRows; i++) {
            const IConfigGate* gate = (*_gates)[i];
            char rowBuf[64];
            char prefix;

            if (gate->isSatisfied()) {
                prefix = '*';
            } else if (actionableGateName != nullptr &&
                       strcmp(gate->getGateName(), actionableGateName) == 0) {
                // Active gate — choose prefix based on action status
                if (_activeGateStatus == GateActionStatus::InProgress) {
                    // Alternate between '>' and '~' for animation effect
                    prefix = (_animFrame % 20 < 10) ? '>' : '~';
                } else {
                    prefix = '>';
                }
            } else {
                prefix = '-';
            }

            snprintf(rowBuf, sizeof(rowBuf), "%c %s", prefix, gate->getGateName());

            int textWidth = u8g2.getStrWidth(rowBuf);
            int rowX = (_displayWidth - textWidth) / 2;
            if (rowX < 4) rowX = 4;

            u8g2.drawStr(rowX, startY + (rowsDrawn * rowHeight), rowBuf);
            rowsDrawn++;
        }

        // --- 3. Animated progress bar ---
        _targetPercent = (gateCount > 0)
            ? (static_cast<float>(satisfiedCount) / gateCount) * 100.0f
            : 0.0f;

        // Lerp: move _currentPercent toward _targetPercent by 2% per frame
        if (_currentPercent < _targetPercent) {
            _currentPercent += 2.0f;
            if (_currentPercent > _targetPercent) _currentPercent = _targetPercent;
        } else if (_currentPercent > _targetPercent) {
            _currentPercent = _targetPercent; // Snap down (gates don't un-satisfy normally)
        }

        // Position progress bar below gate rows (uses constructed Y=38)
        _progressBar.setPercent(static_cast<int>(_currentPercent), 0);
        _progressBar.setShowPercentText(false);
        _progressBar.setFont(AppFonts::defaultSmall());
        _progressBar.render(u8g2);
    } else {
        // No gates bound — show a simple waiting message
        _statusLabel.setText("Waiting...");
        _statusLabel.setFont(AppFonts::defaultSmall());
        _statusLabel.render(u8g2);
    }

    // --- 4. Draw connection info footer (device IP) ---
    if (_deviceUrl[0] != '\0') {
        u8g2.setFont(AppFonts::defaultSmall());
        int textWidth = u8g2.getStrWidth(_deviceUrl);
        int footerX = (_displayWidth - textWidth) / 2;
        if (footerX < 0) footerX = 0;
        u8g2.drawStr(footerX, _displayHeight - 2, _deviceUrl);
    } else if (_networkName[0] != '\0') {
        // Fallback: show network name if no URL set
        u8g2.setFont(AppFonts::defaultSmall());
        int textWidth = u8g2.getStrWidth(_networkName);
        int footerX = (_displayWidth - textWidth) / 2;
        if (footerX < 0) footerX = 0;
        u8g2.drawStr(footerX, _displayHeight - 2, _networkName);
    }
}

/**
 * @brief Query whether the board has been dismissed.
 * @return true when dismiss() has been called.
 */
bool ConfiguringStatusBoard::isExpired() const {
    return _dismissed;
}

/**
 * @brief Return the human-readable board name.
 * @return "SYS: Configuring"
 */
const char* ConfiguringStatusBoard::getBoardName() const {
    return "SYS: Configuring";
}

/**
 * @brief Return the count of satisfied gates.
 * @return Number of gates where isSatisfied() == true, or 0 if no gates bound.
 */
int ConfiguringStatusBoard::getSatisfiedCount() const {
    if (_gates == nullptr) return 0;
    int count = 0;
    for (const auto* gate : *_gates) {
        if (gate->isSatisfied()) {
            count++;
        }
    }
    return count;
}

/**
 * @brief Return the total number of bound gates.
 * @return Total gate count, or 0 if no gates bound.
 */
int ConfiguringStatusBoard::getTotalGateCount() const {
    if (_gates == nullptr) return 0;
    return static_cast<int>(_gates->size());
}
