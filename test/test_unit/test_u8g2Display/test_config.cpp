/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_config.cpp
 * Description: GoogleTest unit-test suite for the U8g2DisplayModule's
 *              iConfigurable implementation. Validates that resetConfig()
 *              sets factory defaults, loadConfig() and saveConfig() return
 *              true in NATIVE_BUILD, and runtime setters work correctly.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "u8g2DisplayModule.hpp"

// =============================================================================
// Test Group: iConfigurable — config load/save/reset
// =============================================================================

class ConfigTest : public ::testing::Test {
protected:
    U8g2DisplayModule display; ///< Fresh module instance for each test
};

/** loadConfig must return true in NATIVE_BUILD (no LittleFS). */
TEST_F(ConfigTest, LoadConfig_NativeBuild_ReturnsTrue) {
    EXPECT_TRUE(display.loadConfig());
}

/** saveConfig must return true in NATIVE_BUILD (no LittleFS). */
TEST_F(ConfigTest, SaveConfig_NativeBuild_ReturnsTrue) {
    EXPECT_TRUE(display.saveConfig());
}

/** resetConfig must not crash. */
TEST_F(ConfigTest, ResetConfig_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.resetConfig());
}

/** After resetConfig, loadConfig still returns true. */
TEST_F(ConfigTest, ResetConfig_ThenLoadConfig_ReturnsTrue) {
    display.resetConfig();
    EXPECT_TRUE(display.loadConfig());
}

/** After resetConfig, saveConfig still returns true. */
TEST_F(ConfigTest, ResetConfig_ThenSaveConfig_ReturnsTrue) {
    display.resetConfig();
    EXPECT_TRUE(display.saveConfig());
}

// =============================================================================
// Test Group: Runtime setters
// =============================================================================

class ConfigSettersTest : public ::testing::Test {
protected:
    U8g2DisplayModule display; ///< Fresh module instance for each test
};

/** setBrightness must not crash. */
TEST_F(ConfigSettersTest, SetBrightness_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setBrightness(128));
}

/** setBrightness with 0 must not crash. */
TEST_F(ConfigSettersTest, SetBrightness_Zero_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setBrightness(0));
}

/** setBrightness with max must not crash. */
TEST_F(ConfigSettersTest, SetBrightness_Max_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setBrightness(255));
}

/** setFlipScreen must not crash. */
TEST_F(ConfigSettersTest, SetFlipScreen_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setFlipScreen(true));
    EXPECT_NO_FATAL_FAILURE(display.setFlipScreen(false));
}

/** setSleepTimeout must not crash. */
TEST_F(ConfigSettersTest, SetSleepTimeout_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setSleepTimeout(60000));
}

/** setSleepTimeout with 0 (disable sleep) must not crash. */
TEST_F(ConfigSettersTest, SetSleepTimeout_Zero_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setSleepTimeout(0));
}

/** setRotationInterval must not crash. */
TEST_F(ConfigSettersTest, SetRotationInterval_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setRotationInterval(5000));
}

/** setRotationInterval with very small value must not crash. */
TEST_F(ConfigSettersTest, SetRotationInterval_SmallValue_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.setRotationInterval(100));
}
