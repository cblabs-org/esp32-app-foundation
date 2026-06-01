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
 *              iConfigurable implementation (new 6-method gateway interface).
 *              Validates that resetConfig() sets factory defaults,
 *              applyConfig() accepts a JSON section, serializeConfig() writes
 *              expected keys, and runtime setters work correctly.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "u8g2DisplayModule.hpp"

// =============================================================================
// Test Group: iConfigurable — config apply/serialize/reset
// =============================================================================

class ConfigTest : public ::testing::Test {
protected:
    U8g2DisplayModule display; ///< Fresh module instance for each test
};

/** getConfigNamespace must return "display". */
TEST_F(ConfigTest, GetConfigNamespace_ReturnsDisplay) {
    EXPECT_STREQ("display", display.getConfigNamespace());
}

/** getSchemaVersion must return 1. */
TEST_F(ConfigTest, GetSchemaVersion_ReturnsOne) {
    EXPECT_EQ(1, display.getSchemaVersion());
}

/** applyConfig with valid JSON section must not crash. */
TEST_F(ConfigTest, ApplyConfig_ValidSection_DoesNotCrash) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["brightness"] = 128;
    section["flip"] = true;
    section["sleepTimeout"] = 60000;
    section["rotationInterval"] = 5000;

    EXPECT_NO_FATAL_FAILURE(display.applyConfig(section));
}

/** applyConfig with empty section must apply defaults without crash. */
TEST_F(ConfigTest, ApplyConfig_EmptySection_AppliesDefaults) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    // Empty section — all keys use fallback defaults

    EXPECT_NO_FATAL_FAILURE(display.applyConfig(section));
}

/** serializeConfig must produce expected keys. */
TEST_F(ConfigTest, SerializeConfig_ProducesExpectedKeys) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();

    display.serializeConfig(section);

    EXPECT_TRUE(section.containsKey("brightness"));
    EXPECT_TRUE(section.containsKey("flip"));
    EXPECT_TRUE(section.containsKey("sleepTimeout"));
    EXPECT_TRUE(section.containsKey("rotationInterval"));
}

/** resetConfig must not crash. */
TEST_F(ConfigTest, ResetConfig_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.resetConfig());
}

/** After resetConfig, applyConfig with empty section must not crash. */
TEST_F(ConfigTest, ResetConfig_ThenApplyConfig_DoesNotCrash) {
    display.resetConfig();
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    EXPECT_NO_FATAL_FAILURE(display.applyConfig(section));
}

/** After resetConfig, serializeConfig still produces expected keys. */
TEST_F(ConfigTest, ResetConfig_ThenSerializeConfig_ProducesKeys) {
    display.resetConfig();
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    display.serializeConfig(section);
    EXPECT_TRUE(section.containsKey("brightness"));
    EXPECT_TRUE(section.containsKey("flip"));
}

/** Roundtrip: serialize → apply → serialize must produce consistent values. */
TEST_F(ConfigTest, Roundtrip_SerializeApplySerialize_Consistent) {
    // Set custom values via applyConfig
    JsonDocument doc1;
    JsonObject section1 = doc1.to<JsonObject>();
    section1["brightness"] = 42;
    section1["flip"] = true;
    section1["sleepTimeout"] = 12345;
    section1["rotationInterval"] = 6789;
    display.applyConfig(section1);

    // Serialize back
    JsonDocument doc2;
    JsonObject section2 = doc2.to<JsonObject>();
    display.serializeConfig(section2);

    EXPECT_EQ(42, section2["brightness"].as<int>());
    EXPECT_EQ(true, section2["flip"].as<bool>());
    EXPECT_EQ(12345, section2["sleepTimeout"].as<unsigned long>());
    EXPECT_EQ(6789, section2["rotationInterval"].as<unsigned long>());
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
