/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/test_main.cpp
 * Description: GoogleTest entry point for the u8g2glDisplay test suite, which
 *              tests the logic of modules/U8G2GLDisplay on the host. Pixels are
 *              not tested here: test/render renders the board through real U8G2
 *              and compares it byte for byte with the legacy board.
 *              Fixtures live in:
 *              - test_diagnostics_format.cpp: formatDiagnostics() and readDiagnosticsInputs()
 *              - test_diagnostics_board.cpp:  DiagnosticsScreenBoard throttle and hostname
 *              - test_screen_board.cpp:       U8G2GLScreenBoard lifecycle and state handling
 *              - test_display.cpp:            U8G2GLDisplay::begin()
 *
 * Exported Functions:
 * - main(): GoogleTest entry point, runs every fixture linked into the binary
 */

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
