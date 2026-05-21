/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_main.cpp
 * Description: GoogleTest entry point for the u8g2Display test suite.
 *              All test fixtures are defined in dedicated files:
 *              - test_carousel.cpp:             Carousel engine and board pool tests
 *              - test_status_reporter.cpp:      IStatusReporter delegation tests
 *              - test_config.cpp:               iConfigurable load/save/reset tests
 *              - test_eventbus_integration.cpp:  EventBus subscription integration
 *              - test_boards.cpp:               System board unit tests
 *              - test_widgets_primitives.cpp:   Widget primitives layer tests
 *
 *              This file provides only the GoogleTest main() entry point
 *              that discovers and runs all test fixtures linked into the binary.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
