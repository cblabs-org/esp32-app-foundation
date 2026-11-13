/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/ConfiguringStatusBoard.cpp
 * Description: Implementation of the ConfiguringStatusBoard. Renders a centred
 *              header, per-precondition status rows with cause-aware prefix indicators,
 *              an animated progress bar that lerps toward the target percentage
 *              (Startup-only), and a device IP footer during CONFIGURING phase.
 *
 * Exported Functions:
 * - ConfiguringStatusBoard::ConfiguringStatusBoard(): Layout widgets for display
 * - ConfiguringStatusBoard::setEngine():             Bind PreconditionEngine
 * - ConfiguringStatusBoard::setPreconditions():      Bind snapshot array
 * - ConfiguringStatusBoard::updatePrecondition():    Update/insert precondition status
 * - ConfiguringStatusBoard::dismiss():               Mark dismissed for expiry
 * - ConfiguringStatusBoard::render():                Draw header, status rows, progress bar, footer
 * - ConfiguringStatusBoard::isExpired():             Return dismissed state
 * - ConfiguringStatusBoard::getBoardName():          Return "SYS: Configuring"
 * - ConfiguringStatusBoard::getTotalPreconditions(): Return total bound precondition count
 */

#include "ConfiguringStatusBoard.hpp"
#include "PreconditionEngine/PreconditionEngine.hpp"
#include <cstring>
#include <cstdio>
#include <algorithm>

#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/Fonts.hpp"

/**
 * @brief Map a precondition's cause/severity to a single-character prefix.
 *
 * Used by render() for per-row status indicators:
 *   '*' = Met, '~'/'\>' = Connecting (animated), '!' = NeedsUser,
 *   '-' = DependencyUnmet, '?' = Indeterminate, '\>' = in progress.
 *
 * @param item     The precondition snapshot to derive the prefix from.
 * @param animFrame Current animation frame counter (for Connecting toggle).
 * @return Single character prefix for the status row.
 */
static char causeToPrefix(const PreconditionSnapshot& item, uint8_t animFrame) {
    if (item.cause == Cause::Met) return '*';
    if (item.cause == Cause::Connecting) return (animFrame % 20 < 10) ? '~' : '>';
    if (item.severity == Severity::NeedsUser) return '!';
    if (item.cause == Cause::DependencyUnmet) return '-';
    if (item.cause == Cause::Indeterminate) return '?';
    return '>';
}

// =============================================================================
// Construction
// =============================================================================

/**
 * @brief Construct the configuring status board, laying out widgets.
 *
 * Layout (256x64 example):
 *   Row 0-11:  Header "Configuring..." (bold, centred)
 *   Rows:      Per-precondition status rows (small font, left-aligned with prefix)
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
 * @brief Bind a PreconditionEngine for live status reading.
 * @param engine Pointer to PreconditionEngine (non-owning, or nullptr to unbind).
 */
void ConfiguringStatusBoard::setEngine(const PreconditionEngine* engine) {
    _engine = engine;
    _recomputeCounts();
}

/**
 * @brief Bind a static snapshot array of preconditions.
 * @param items Pointer to array of PreconditionSnapshot.
 * @param count Number of items in array.
 */
void ConfiguringStatusBoard::setPreconditions(const PreconditionSnapshot* items, size_t count) {
    _engine = nullptr;
    _snapshotCount = (count > MAX_DISPLAY_PRECONDITIONS) ? MAX_DISPLAY_PRECONDITIONS : count;
    if (items != nullptr) {
        for (size_t i = 0; i < _snapshotCount; ++i) {
            _snapshots[i] = items[i];
        }
    } else {
        _snapshotCount = 0;
    }
    _recomputeCounts();
}

/**
 * @brief Update or add a precondition entry from a health event.
 */
void ConfiguringStatusBoard::updatePrecondition(const PreconditionHealthPayload& payload, PreconditionRole role) {
    // When caller provides an explicit role override (e.g., from test code), use it.
    // Otherwise, use the role from the payload (populated by Application.cpp from PreconditionState).
    // The default parameter is PreconditionRole::Startup for backward compatibility.
    (void)role;  // Legacy parameter — payload.role is now the source of truth
    updatePrecondition(payload.name, payload.cause, payload.severity, payload.everMet, payload.role);
}

/**
 * @brief Update or add a precondition entry by explicit parameters.
 */
void ConfiguringStatusBoard::updatePrecondition(const char* name, Cause cause, Severity severity, bool everMet, PreconditionRole role) {
    if (name == nullptr) return;

    for (size_t i = 0; i < _snapshotCount; ++i) {
        if (strcmp(_snapshots[i].name, name) == 0) {
            _snapshots[i].cause = cause;
            _snapshots[i].severity = severity;
            _snapshots[i].everMet = everMet;
            _snapshots[i].met = (cause == Cause::Met);
            _recomputeCounts();
            return;
        }
    }

    if (_snapshotCount < MAX_DISPLAY_PRECONDITIONS) {
        strncpy(_snapshots[_snapshotCount].name, name, sizeof(_snapshots[_snapshotCount].name) - 1);
        _snapshots[_snapshotCount].name[sizeof(_snapshots[_snapshotCount].name) - 1] = '\0';
        _snapshots[_snapshotCount].role = role;
        _snapshots[_snapshotCount].cause = cause;
        _snapshots[_snapshotCount].severity = severity;
        _snapshots[_snapshotCount].everMet = everMet;
        _snapshots[_snapshotCount].met = (cause == Cause::Met);
        _snapshotCount++;
        _recomputeCounts();
    }
}

/**
 * @brief Clear all cached snapshots and unbind engine.
 */
void ConfiguringStatusBoard::clear() {
    _engine = nullptr;
    _snapshotCount = 0;
    _startupCount = 0;
    _startupMetCount = 0;
    _targetPercent = 0.0f;
    _currentPercent = 0.0f;
}

/**
 * @brief Return total number of bound/cached preconditions.
 */
int ConfiguringStatusBoard::getTotalPreconditions() const {
    if (_engine != nullptr) {
        return static_cast<int>(_engine->getPreconditionCount());
    }
    return static_cast<int>(_snapshotCount);
}

/**
 * @brief Set the authoritative actionable gate name from the Application.
 * @param gateName Gate name from AppConfiguringPayload::gateName (non-owning).
 */
void ConfiguringStatusBoard::setActionableGateName(const char* gateName) {
    _actionableGateName = gateName;
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
    } else {
        _networkName[0] = '\0';
    }
    if (url) {
        strncpy(_deviceUrl, url, sizeof(_deviceUrl) - 1);
        _deviceUrl[sizeof(_deviceUrl) - 1] = '\0';
    } else {
        _deviceUrl[0] = '\0';
    }
}

/**
 * @brief Mark the board as dismissed, causing isExpired() to return true.
 */
void ConfiguringStatusBoard::dismiss() {
    _dismissed = true;
}

void ConfiguringStatusBoard::_recomputeCounts() {
    _startupCount = 0;
    _startupMetCount = 0;

    if (_engine != nullptr) {
        size_t count = _engine->getPreconditionCount();
        for (size_t i = 0; i < count; ++i) {
            const auto& st = _engine->getStateAt(i);
            if (st.role == PreconditionRole::Startup || st.role == PreconditionRole::Both) {
                _startupCount++;
                if (st.dampedStatus.met || st.dampedStatus.cause == Cause::Met) {
                    _startupMetCount++;
                }
            }
        }
    } else {
        for (size_t i = 0; i < _snapshotCount; ++i) {
            if (_snapshots[i].role == PreconditionRole::Startup || _snapshots[i].role == PreconditionRole::Both) {
                _startupCount++;
                if (_snapshots[i].met || _snapshots[i].cause == Cause::Met) {
                    _startupMetCount++;
                }
            }
        }
    }

    _targetPercent = (_startupCount > 0)
        ? (static_cast<float>(_startupMetCount) / _startupCount) * 100.0f
        : 0.0f;
}

/**
 * @brief Render the configuring status screen to the U8G2 display buffer.
 *
 * Layout:
 *   1. Header "Configuring..." at Y=0 (bold, centred)
 *   2. Per-precondition status rows with cause-aware prefix indicators:
 *      - '*' = Met
 *      - '~' = Connecting (animated)
 *      - '!' = NeedsUser
 *      - '-' = DependencyUnmet (blocked)
 *      - '?' = Indeterminate (error)
 *      - '>' = In progress / transient
 *   3. Animated progress bar (Startup-only, lerps toward target)
 *   4. Device IP footer
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ConfiguringStatusBoard::render(U8G2& u8g2) {
    // --- 1. Draw header (centred, bold) ---
    _headerLabel.setFont(AppFonts::defaultLarge());
    _headerLabel.render(u8g2);

    // --- 2. Gather precondition snapshots ---
    PreconditionSnapshot items[MAX_DISPLAY_PRECONDITIONS];
    size_t totalCount = 0;

    if (_engine != nullptr) {
        totalCount = _engine->getPreconditionCount();
        if (totalCount > MAX_DISPLAY_PRECONDITIONS) totalCount = MAX_DISPLAY_PRECONDITIONS;
        for (size_t i = 0; i < totalCount; ++i) {
            const auto& st = _engine->getStateAt(i);
            if (st.name) {
                strncpy(items[i].name, st.name, sizeof(items[i].name) - 1);
                items[i].name[sizeof(items[i].name) - 1] = '\0';
            } else {
                items[i].name[0] = '\0';
            }
            items[i].role = st.role;
            items[i].cause = st.dampedStatus.cause;
            items[i].severity = st.severity;
            items[i].met = st.dampedStatus.met || (st.dampedStatus.cause == Cause::Met);
            items[i].everMet = st.everMet;
        }
    } else {
        totalCount = _snapshotCount;
        for (size_t i = 0; i < totalCount; ++i) {
            items[i] = _snapshots[i];
        }
    }

    _recomputeCounts();

    if (totalCount > 0) {
        _animFrame++;

        // Separate Startup vs Operational items
        uint8_t startupIndices[MAX_DISPLAY_PRECONDITIONS];
        uint8_t opIndices[MAX_DISPLAY_PRECONDITIONS];
        uint8_t startupNum = 0;
        uint8_t opNum = 0;

        for (size_t i = 0; i < totalCount; ++i) {
            if (items[i].role == PreconditionRole::Startup || items[i].role == PreconditionRole::Both) {
                startupIndices[startupNum++] = static_cast<uint8_t>(i);
            } else {
                opIndices[opNum++] = static_cast<uint8_t>(i);
            }
        }

        // Operational items: unmet first, then met
        uint8_t displayOrder[MAX_DISPLAY_PRECONDITIONS];
        uint8_t displayCount = 0;
        for (uint8_t j = 0; j < startupNum; ++j) {
            displayOrder[displayCount++] = startupIndices[j];
        }
        for (uint8_t j = 0; j < opNum; ++j) {
            if (!items[opIndices[j]].met) {
                displayOrder[displayCount++] = opIndices[j];
            }
        }
        for (uint8_t j = 0; j < opNum; ++j) {
            if (items[opIndices[j]].met) {
                displayOrder[displayCount++] = opIndices[j];
            }
        }

        u8g2.setFont(AppFonts::defaultSmall());
        const int rowHeight = 10;
        const int startY = 16;
        const int maxRows = (_displayHeight > 48) ? 3 : 2;

        if (displayCount <= static_cast<uint8_t>(maxRows)) {
            for (uint8_t r = 0; r < displayCount; ++r) {
                const auto& item = items[displayOrder[r]];
                char prefix = causeToPrefix(item, _animFrame);

                char rowBuf[64];
                snprintf(rowBuf, sizeof(rowBuf), "%c %s", prefix, item.name);
                int textWidth = u8g2.getStrWidth(rowBuf);
                int rowX = (_displayWidth - textWidth) / 2;
                if (rowX < 4) rowX = 4;
                u8g2.drawStr(rowX, startY + (r * rowHeight), rowBuf);
            }
        } else {
            const uint8_t visibleItems = static_cast<uint8_t>(maxRows - 1);
            for (uint8_t r = 0; r < visibleItems; ++r) {
                const auto& item = items[displayOrder[r]];
                char prefix = causeToPrefix(item, _animFrame);

                char rowBuf[64];
                snprintf(rowBuf, sizeof(rowBuf), "%c %s", prefix, item.name);
                int textWidth = u8g2.getStrWidth(rowBuf);
                int rowX = (_displayWidth - textWidth) / 2;
                if (rowX < 4) rowX = 4;
                u8g2.drawStr(rowX, startY + (r * rowHeight), rowBuf);
            }

            // Draw overflow row on last row
            char overflowBuf[32];
            uint8_t overflowCount = displayCount - visibleItems;
            snprintf(overflowBuf, sizeof(overflowBuf), "+%u more", overflowCount);
            int textWidth = u8g2.getStrWidth(overflowBuf);
            int rowX = (_displayWidth - textWidth) / 2;
            if (rowX < 4) rowX = 4;
            u8g2.drawStr(rowX, startY + (visibleItems * rowHeight), overflowBuf);
        }

        // --- 3. Animated progress bar (Startup-role only) ---
        _targetPercent = (_startupCount > 0)
            ? (static_cast<float>(_startupMetCount) / _startupCount) * 100.0f
            : 0.0f;

        // Lerp: move _currentPercent toward _targetPercent by 2% per frame
        if (_currentPercent < _targetPercent) {
            _currentPercent += 2.0f;
            if (_currentPercent > _targetPercent) _currentPercent = _targetPercent;
        } else if (_currentPercent > _targetPercent) {
            _currentPercent = _targetPercent; // Snap down
        }

        _progressBar.setPercent(static_cast<int>(_currentPercent), 0);
        _progressBar.setShowPercentText(false);
        _progressBar.setFont(AppFonts::defaultSmall());
        _progressBar.render(u8g2);
    } else {
        // No preconditions bound — show a simple waiting message
        _statusLabel.setText("Waiting...");
        _statusLabel.setFont(AppFonts::defaultSmall());
        _statusLabel.render(u8g2);
    }

    // --- 4. Draw connection info footer (device IP) ---
    // Under setFontPosTop() the y-coordinate is the top of the glyph, so
    // position at displayHeight minus font height for full visibility (#189).
    if (_deviceUrl[0] != '\0') {
        u8g2.setFont(AppFonts::defaultSmall());
        int fontH = u8g2.getMaxCharHeight();
        int textWidth = u8g2.getStrWidth(_deviceUrl);
        int footerX = (_displayWidth - textWidth) / 2;
        if (footerX < 0) footerX = 0;
        u8g2.drawStr(footerX, _displayHeight - fontH, _deviceUrl);
    } else if (_networkName[0] != '\0') {
        // Fallback: show network name if no URL set
        u8g2.setFont(AppFonts::defaultSmall());
        int fontH = u8g2.getMaxCharHeight();
        int textWidth = u8g2.getStrWidth(_networkName);
        int footerX = (_displayWidth - textWidth) / 2;
        if (footerX < 0) footerX = 0;
        u8g2.drawStr(footerX, _displayHeight - fontH, _networkName);
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
