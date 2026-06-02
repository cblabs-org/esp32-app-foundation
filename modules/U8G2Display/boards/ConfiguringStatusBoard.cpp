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
 *              header, a single-line gate status label, and a progress bar
 *              matching the boot board's LOADING phase layout during the
 *              CONFIGURING lifecycle phase.
 *
 * Exported Functions:
 * - ConfiguringStatusBoard::ConfiguringStatusBoard(): Layout widgets for display
 * - ConfiguringStatusBoard::setGates():      Bind gate list pointer
 * - ConfiguringStatusBoard::dismiss():       Mark dismissed for expiry
 * - ConfiguringStatusBoard::render():        Draw header, status label, progress bar
 * - ConfiguringStatusBoard::isExpired():     Return dismissed state
 * - ConfiguringStatusBoard::getBoardName():  Return "SYS: Configuring"
 * - ConfiguringStatusBoard::getSatisfiedCount(): Return count of satisfied gates
 * - ConfiguringStatusBoard::getTotalGateCount(): Return total bound gate count
 */

#include "ConfiguringStatusBoard.hpp"
#include <cstring>
#include <cstdio>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/Fonts.hpp"
#endif

// =============================================================================
// Construction
// =============================================================================

/**
 * @brief Construct the configuring status board, laying out widgets.
 *
 * Layout matches boot board LOADING phase (256x64 example):
 *   Row 0-11:  Header "Configuring..." (bold, centred)
 *   Row 24:    Status label — gate name (small, centred)
 *   Row 38-53: Progress bar (X=16, width=dw-32, height=16)
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
 * Layout (matching boot board LOADING phase):
 *   1. Header "Configuring..." at Y=0 (bold, centred)
 *   2. Status label at Y=24 showing the first actionable unsatisfied gate
 *      name with a ">" prefix (ASCII-safe)
 *   3. Progress bar at Y=38, X=16, width=dw-32, height=16 showing
 *      (satisfied / total) as percentage
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ConfiguringStatusBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    // --- 1. Draw header (centred, bold) ---
    _headerLabel.setFont(AppFonts::defaultLarge());
    _headerLabel.render(u8g2);

    // --- 2. Compute gate status and draw status label ---
    if (_gates != nullptr && !_gates->empty()) {
        const int gateCount = static_cast<int>(_gates->size());
        int satisfiedCount = 0;
        const char* activeGateName = nullptr;

        for (int i = 0; i < gateCount; i++) {
            const IConfigGate* gate = (*_gates)[i];
            if (gate->isSatisfied()) {
                satisfiedCount++;
            } else if (activeGateName == nullptr) {
                // First unsatisfied gate — check if it's actionable
                // (all dependencies satisfied)
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
                    activeGateName = gate->getGateName();
                }
            }
        }

        // Format status label: "> GateName" or "Waiting..."
        char statusBuf[64];
        if (activeGateName != nullptr) {
            snprintf(statusBuf, sizeof(statusBuf), "> %s", activeGateName);
        } else {
            snprintf(statusBuf, sizeof(statusBuf), "Waiting...");
        }
        _statusLabel.setText(statusBuf);
        _statusLabel.setFont(AppFonts::defaultSmall());
        _statusLabel.render(u8g2);

        // --- 3. Draw progress bar ---
        int percent = (gateCount > 0)
            ? static_cast<int>((static_cast<float>(satisfiedCount) / gateCount) * 100.0f)
            : 0;
        _progressBar.setPercent(percent, 0);
        _progressBar.setShowPercentText(false);
        _progressBar.setFont(AppFonts::defaultSmall());
        _progressBar.render(u8g2);
    } else {
        // No gates bound — show a simple waiting message
        _statusLabel.setText("Waiting...");
        _statusLabel.setFont(AppFonts::defaultSmall());
        _statusLabel.render(u8g2);
    }

    // --- 4. Draw connection info footer ---
    if (_networkName[0] != '\0' && _deviceUrl[0] != '\0') {
        char footerBuf[80];
        snprintf(footerBuf, sizeof(footerBuf), "%s  %s", _networkName, _deviceUrl);
        u8g2.setFont(AppFonts::defaultSmall());
        // Centre the footer text at the very bottom
        int textWidth = u8g2.getStrWidth(footerBuf);
        int footerX = (_displayWidth - textWidth) / 2;
        if (footerX < 0) footerX = 0;
        u8g2.drawStr(footerX, _displayHeight - 2, footerBuf);
    }
#else
    (void)u8g2; // Suppress unused parameter warning in native builds
#endif
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
