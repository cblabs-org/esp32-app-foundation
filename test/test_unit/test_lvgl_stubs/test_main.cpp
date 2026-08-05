/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvgl_stubs/test_main.cpp
 * Description: Main entry point for test_lvgl_stubs test suite.
 */

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
