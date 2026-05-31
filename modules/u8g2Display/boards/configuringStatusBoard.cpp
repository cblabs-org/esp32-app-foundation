/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/configuringStatusBoard.cpp
 * Description: Implementation of the ConfiguringStatusBoard. Renders a header,
 *              dynamic gate rows with status icons, and a progress bar during
 *              the CONFIGURING lifecycle phase.
 *
 * Exported Functions:
 * - ConfiguringStatusBoard::ConfiguringStatusBoard(): Layout widgets for display
 * - ConfiguringStatusBoard::setGates():      Bind gate list pointer
 * - ConfiguringStatusBoard::dismiss():       Mark dismissed for expiry
 * - ConfiguringStatusBoard::render():        Draw header, gate rows, progress bar
 * - ConfiguringStatusBoard::isExpired():     Return dismissed state
 * - ConfiguringStatusBoard::getBoardName():  Return "SYS: Configuring"
 * - ConfiguringStatusBoard::getSatisfiedCount(): Return count of satisfied gates
 * - ConfiguringStatusBoard::getTotalGateCount(): Return total bound gate count
 */

#include "configuringStatusBoard.hpp"
#include <cstring>
#include <cstdio>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <U8g2lib.h>
#include "fonts/fonts.hpp"
#endif

// =============================================================================
// Construction
// =============================================================================

/**
 * @brief Construct the configuring status board, laying out widgets.
 *
 * Layout (256×64 example):
 *   Row 0-13:  Header label "Configuring..." (bold, left-aligned)
 *   Row 14-47: Gate rows (drawn directly — up to ~4 visible rows)
 *   Row 48-63: Progress bar (bottom strip)
 *
 * @param displayWidth  Display width in pixels.
 * @param displayHeight Display height in pixels.
 */
ConfiguringStatusBoard::ConfiguringStatusBoard(uint16_t displayWidth, uint16_t displayHeight)
    : _displayWidth(displayWidth),
      _displayHeight(displayHeight),
      _headerLabel(4, 0, static_cast<int>(displayWidth) - 8, -1),
      _progressBar(8, static_cast<int>(displayHeight) - 14,
                   static_cast<int>(displayWidth) - 16, 10, nullptr)
{
    _headerLabel.setText("Configuring...");
    _headerLabel.setAlignment(0); // LEFT
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
 * @brief Mark the board as dismissed, causing isExpired() to return true.
 */
void ConfiguringStatusBoard::dismiss() {
    _dismissed = true;
}

/**
 * @brief Render the configuring status screen to the U8G2 display buffer.
 *
 * Layout:
 *   1. Header "Configuring..." at the top (bold font)
 *   2. Gate rows: each gate gets a status icon + gate name
 *      - ✓ (checkmark glyph) = isSatisfied() == true
 *      - > (arrow/spinner) = first unsatisfied gate (in-progress)
 *      - ○ (circle) = not yet reached / waiting
 *   3. Progress bar at the bottom showing (satisfied / total) as percentage
 *
 * @param u8g2 Reference to the active U8G2 driver instance.
 */
void ConfiguringStatusBoard::render(U8G2& u8g2) {
#ifndef NATIVE_BUILD
    // --- 1. Draw header ---
    _headerLabel.setFont(AppFonts::defaultLarge());
    _headerLabel.render(u8g2);

    // --- 2. Draw gate rows ---
    if (_gates != nullptr && !_gates->empty()) {
        const int gateCount = static_cast<int>(_gates->size());
        int satisfiedCount = 0;

        // Use small font for gate rows
        u8g2.setFont(AppFonts::defaultSmall());

        // Layout constants
        const int rowStartY = 16;       // Y position of first gate row
        const int rowHeight = 10;        // Pixels per row
        const int iconX = 6;            // X position of status icon
        const int textX = 16;           // X position of gate name text
        const int maxVisibleRows = (_displayHeight - rowStartY - 18) / rowHeight;

        // --- Classify gates into three buckets: satisfied, actionable, blocked ---
        // We render them in this order: satisfied first, then actionable, then blocked.
        struct GateEntry { const IConfigGate* gate; uint8_t category; /* 0=satisfied, 1=actionable, 2=blocked */ };
        // Stack-allocate a small array (max 8 gates typical in embedded)
        static constexpr int MAX_RENDER_GATES = 16;
        GateEntry sorted[MAX_RENDER_GATES];
        int sortedCount = 0;

        for (int i = 0; i < gateCount && sortedCount < MAX_RENDER_GATES; i++) {
            const IConfigGate* gate = (*_gates)[i];
            if (gate->isSatisfied()) {
                sorted[sortedCount++] = {gate, 0}; // satisfied
                satisfiedCount++;
            } else {
                // Check dependencies
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
                sorted[sortedCount++] = {gate, static_cast<uint8_t>(depsOk ? 1 : 2)};
            }
        }

        // Simple insertion sort by category (0 < 1 < 2)
        for (int i = 1; i < sortedCount; i++) {
            GateEntry key = sorted[i];
            int j = i - 1;
            while (j >= 0 && sorted[j].category > key.category) {
                sorted[j + 1] = sorted[j];
                j--;
            }
            sorted[j + 1] = key;
        }

        // Render sorted gate rows
        bool foundFirstActionable = false;
        for (int i = 0; i < sortedCount && i < maxVisibleRows; i++) {
            const IConfigGate* gate = sorted[i].gate;
            uint8_t category = sorted[i].category;
            int rowY = rowStartY + (i * rowHeight);

            if (category == 0) {
                // Satisfied: checkmark
                u8g2.drawLine(iconX, rowY + 5, iconX + 2, rowY + 7);
                u8g2.drawLine(iconX + 2, rowY + 7, iconX + 6, rowY + 3);
            } else if (category == 1 && !foundFirstActionable) {
                // First actionable: arrow/spinner
                foundFirstActionable = true;
                u8g2.drawTriangle(iconX, rowY + 2, iconX, rowY + 8, iconX + 5, rowY + 5);
            } else if (category == 1) {
                // Subsequent actionable: spinning icon (circle with dot)
                u8g2.drawCircle(iconX + 3, rowY + 5, 3, U8G2_DRAW_ALL);
                u8g2.drawDisc(iconX + 3, rowY + 3, 1, U8G2_DRAW_ALL);
            } else {
                // Blocked: empty circle
                u8g2.drawCircle(iconX + 3, rowY + 5, 3, U8G2_DRAW_ALL);
            }

            // Draw gate name
            u8g2.drawStr(textX, rowY, gate->getGateName());
        }

        // Count remaining satisfied gates beyond visible rows
        for (int i = maxVisibleRows; i < sortedCount; i++) {
            if (sorted[i].category == 0) {
                // Already counted above
            }
        }

        // --- 3. Draw progress bar ---
        int percent = (gateCount > 0)
            ? static_cast<int>((static_cast<float>(satisfiedCount) / gateCount) * 100.0f)
            : 0;
        _progressBar.setPercent(percent, 0);
        _progressBar.setShowPercentText(true);
        _progressBar.setFont(AppFonts::defaultSmall());
        _progressBar.render(u8g2);
    } else {
        // No gates bound — show a simple waiting message
        u8g2.setFont(AppFonts::defaultSmall());
        u8g2.drawStr(4, 28, "Waiting for configuration...");
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
