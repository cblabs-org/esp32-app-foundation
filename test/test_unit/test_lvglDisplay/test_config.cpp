/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_lvglDisplay/test_config.cpp
 * Description: GoogleTest unit test suite for LVGLDisplayModule's IConfigurable implementation.
 *              Validates namespace, schema version, applyConfig, serializeConfig, resetConfig,
 *              brightness/rotation clamping, legacy migration, runtime setters, and mock adapter integration.
 *
 * Exported Classes:
 * - (GoogleTest test cases discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "LVGLDisplayModule.hpp"
#include "MockLVGLHardwareAdapter.hpp"

// =============================================================================
// Test Group: IConfigurable — config apply/serialize/reset
// =============================================================================

class LVGLConfigTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    LVGLDisplayModule display{adapter};
};

/** @brief getConfigNamespace must return "display". */
TEST_F(LVGLConfigTest, GetConfigNamespace_ReturnsDisplay) {
    EXPECT_STREQ("display", display.getConfigNamespace());
}

/** @brief getSchemaVersion must return 1. */
TEST_F(LVGLConfigTest, GetSchemaVersion_ReturnsOne) {
    EXPECT_EQ(1, display.getSchemaVersion());
}

/** @brief applyConfig with valid JSON section applies values and updates adapter brightness. */
TEST_F(LVGLConfigTest, ApplyConfig_ValidSection_AppliesValuesAndUpdatesAdapter) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["brightness"] = 128;
    section["rotation"] = 90;
    section["screensaverEnabled"] = false;
    section["dimTimeout"] = 30000UL;
    section["blankTimeout"] = 60000UL;

    display.applyConfig(section);

    EXPECT_EQ(128, display.getBrightness());
    EXPECT_EQ(128, adapter.brightnessLevel);
    EXPECT_EQ(90, display.getRotation());
    EXPECT_EQ(false, display.getScreensaverEnabled());
    EXPECT_EQ(30000UL, display.getDimTimeout());
    EXPECT_EQ(60000UL, display.getBlankTimeout());
}

/** @brief applyConfig with empty section applies module-owned defaults. */
TEST_F(LVGLConfigTest, ApplyConfig_EmptySection_AppliesDefaults) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();

    display.applyConfig(section);

    EXPECT_EQ(255, display.getBrightness());
    EXPECT_EQ(255, adapter.brightnessLevel);
    EXPECT_EQ(0, display.getRotation());
    EXPECT_EQ(true, display.getScreensaverEnabled());
    EXPECT_EQ(150000UL, display.getDimTimeout());
    EXPECT_EQ(300000UL, display.getBlankTimeout());
}

/** @brief applyConfig clamps out-of-range brightness values. */
TEST_F(LVGLConfigTest, ApplyConfig_BrightnessClamping) {
    JsonDocument doc1;
    JsonObject s1 = doc1.to<JsonObject>();
    s1["brightness"] = -50;
    display.applyConfig(s1);
    EXPECT_EQ(0, display.getBrightness());
    EXPECT_EQ(0, adapter.brightnessLevel);

    JsonDocument doc2;
    JsonObject s2 = doc2.to<JsonObject>();
    s2["brightness"] = 350;
    display.applyConfig(s2);
    EXPECT_EQ(255, display.getBrightness());
    EXPECT_EQ(255, adapter.brightnessLevel);
}

/** @brief applyConfig validates rotation angles (0, 90, 180, 270) and maps invalid angles to 0. */
TEST_F(LVGLConfigTest, ApplyConfig_RotationValidation) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();

    section["rotation"] = 180;
    display.applyConfig(section);
    EXPECT_EQ(180, display.getRotation());

    section["rotation"] = 270;
    display.applyConfig(section);
    EXPECT_EQ(270, display.getRotation());

    section["rotation"] = 45; // Invalid
    display.applyConfig(section);
    EXPECT_EQ(0, display.getRotation());
}

/** @brief applyConfig converts legacy sleepTimeout into dimTimeout and blankTimeout. */
TEST_F(LVGLConfigTest, ApplyConfig_LegacySleepTimeoutMigration) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["sleepTimeout"] = 60000UL;

    display.applyConfig(section);

    EXPECT_EQ(30000UL, display.getDimTimeout());
    EXPECT_EQ(60000UL, display.getBlankTimeout());
}

/** @brief serializeConfig produces expected JSON keys. */
TEST_F(LVGLConfigTest, SerializeConfig_ProducesExpectedKeys) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();

    display.serializeConfig(section);

    EXPECT_TRUE(section["brightness"].is<JsonVariant>());
    EXPECT_TRUE(section["rotation"].is<JsonVariant>());
    EXPECT_TRUE(section["screensaverEnabled"].is<JsonVariant>());
    EXPECT_TRUE(section["dimTimeout"].is<JsonVariant>());
    EXPECT_TRUE(section["blankTimeout"].is<JsonVariant>());
}

/** @brief resetConfig restores module factory defaults and calls adapter. */
TEST_F(LVGLConfigTest, ResetConfig_RestoresFactoryDefaults) {
    // Mutate state first
    display.setBrightness(42);
    display.setRotation(180);
    display.setScreensaverEnabled(false);
    display.setDimTimeout(1000);
    display.setBlankTimeout(2000);

    display.resetConfig();

    EXPECT_EQ(255, display.getBrightness());
    EXPECT_EQ(255, adapter.brightnessLevel);
    EXPECT_EQ(0, display.getRotation());
    EXPECT_EQ(true, display.getScreensaverEnabled());
    EXPECT_EQ(150000UL, display.getDimTimeout());
    EXPECT_EQ(300000UL, display.getBlankTimeout());
}

/** @brief Config roundtrip (apply → serialize → apply) maintains consistent values. */
TEST_F(LVGLConfigTest, Roundtrip_SerializeApplySerialize_Consistent) {
    JsonDocument doc1;
    JsonObject section1 = doc1.to<JsonObject>();
    section1["brightness"] = 100;
    section1["rotation"] = 270;
    section1["screensaverEnabled"] = false;
    section1["dimTimeout"] = 45000UL;
    section1["blankTimeout"] = 90000UL;

    display.applyConfig(section1);

    JsonDocument doc2;
    JsonObject section2 = doc2.to<JsonObject>();
    display.serializeConfig(section2);

    EXPECT_EQ(100, section2["brightness"].as<int>());
    EXPECT_EQ(270, section2["rotation"].as<int>());
    EXPECT_EQ(false, section2["screensaverEnabled"].as<bool>());
    EXPECT_EQ(45000UL, section2["dimTimeout"].as<unsigned long>());
    EXPECT_EQ(90000UL, section2["blankTimeout"].as<unsigned long>());
}

// =============================================================================
// Test Group: Runtime Setters
// =============================================================================

/** @brief setBrightness updates brightness property and delegates to adapter.setBrightness. */
TEST_F(LVGLConfigTest, SetBrightness_UpdatesAdapterAndProperty) {
    display.setBrightness(77);

    EXPECT_EQ(77, display.getBrightness());
    EXPECT_EQ(77, adapter.brightnessLevel);
}

/** @brief setRotation updates rotation property and applies to active LVGL display handle. */
TEST_F(LVGLConfigTest, SetRotation_UpdatesDisplayAndProperty) {
    display.bootstrapHardware(); // Creates display handle

    display.setRotation(90);
    EXPECT_EQ(90, display.getRotation());
    EXPECT_EQ(LV_DISPLAY_ROTATION_90, lv_display_get_rotation(display.getDisplay()));

    display.setRotation(180);
    EXPECT_EQ(180, display.getRotation());
    EXPECT_EQ(LV_DISPLAY_ROTATION_180, lv_display_get_rotation(display.getDisplay()));
}

/** @brief setScreensaverEnabled, setDimTimeout, and setBlankTimeout update properties correctly. */
TEST_F(LVGLConfigTest, TimeoutAndScreensaverSetters_UpdateProperties) {
    display.setScreensaverEnabled(false);
    EXPECT_FALSE(display.getScreensaverEnabled());

    display.setDimTimeout(12000UL);
    EXPECT_EQ(12000UL, display.getDimTimeout());

    display.setBlankTimeout(24000UL);
    EXPECT_EQ(24000UL, display.getBlankTimeout());
}
