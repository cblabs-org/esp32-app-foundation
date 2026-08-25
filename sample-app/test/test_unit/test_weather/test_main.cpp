/*
 * ESP32 App Foundation — Sample Application Tests
 *
 * Module: sample-app/test/test_unit/test_weather/test_main.cpp
 * Description: Unit tests for WeatherModule demonstrating how to test
 *              framework modules in the native GoogleTest environment.
 *              Tests cover IConfigurable (config round-trip, defaults,
 *              schema metadata), IPreconditionContributor (precondition logic), and
 *              IDataSource (source metadata).
 *
 * Test Groups:
 * - WeatherModuleConfigTest: Config serialization, application, and reset
 * - WeatherModulePreconditionTest: IPreconditionContributor satisfaction logic
 * - WeatherModuleMetadataTest: Interface metadata (namespace, name, tier)
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>

// Include implementation directly — PlatformIO test mode does not compile
// src/ files automatically; framework modules live in lib_extra_dirs.
// This single-TU include pattern is standard for testing app-level modules.
#include "WeatherModule.cpp"

// =============================================================================
// Test Fixture
// =============================================================================

class WeatherModuleTest : public ::testing::Test {
protected:
    WeatherModule module;
};

// =============================================================================
// IConfigurable — Config Round-Trip
// =============================================================================

TEST_F(WeatherModuleTest, ConfigRoundTrip) {
    // Apply known config values
    JsonDocument applyDoc;
    JsonObject applyObj = applyDoc.to<JsonObject>();
    applyObj["lat"] = 51.5074;
    applyObj["lon"] = -0.1278;
    applyObj["interval"] = 300000;
    applyObj["units"] = "imperial";

    module.applyConfig(applyObj);

    // Serialize back and verify
    JsonDocument serDoc;
    JsonObject serObj = serDoc.to<JsonObject>();
    module.serializeConfig(serObj);

    EXPECT_DOUBLE_EQ(serObj["lat"].as<double>(), 51.5074);
    EXPECT_DOUBLE_EQ(serObj["lon"].as<double>(), -0.1278);
    EXPECT_EQ(serObj["interval"].as<uint32_t>(), 300000u);
    EXPECT_STREQ(serObj["units"].as<const char*>(), "imperial");
}

TEST_F(WeatherModuleTest, ConfigDefaults) {
    // After resetConfig(), verify factory defaults
    module.resetConfig();

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    module.serializeConfig(obj);

    EXPECT_DOUBLE_EQ(obj["lat"].as<double>(), 0.0);
    EXPECT_DOUBLE_EQ(obj["lon"].as<double>(), 0.0);
    EXPECT_EQ(obj["interval"].as<uint32_t>(), 600000u);
    EXPECT_STREQ(obj["units"].as<const char*>(), "metric");
}

TEST_F(WeatherModuleTest, ConfigApplyPartial) {
    // Apply full config first
    JsonDocument fullDoc;
    JsonObject fullObj = fullDoc.to<JsonObject>();
    fullObj["lat"] = 40.7128;
    fullObj["lon"] = -74.0060;
    fullObj["interval"] = 120000;
    fullObj["units"] = "imperial";
    module.applyConfig(fullObj);

    // Apply partial config — missing fields should use ArduinoJson defaults
    JsonDocument partialDoc;
    JsonObject partialObj = partialDoc.to<JsonObject>();
    partialObj["lat"] = 48.8566;
    // lon, interval, units are missing — they should get default fallback values
    module.applyConfig(partialObj);

    JsonDocument serDoc;
    JsonObject serObj = serDoc.to<JsonObject>();
    module.serializeConfig(serObj);

    EXPECT_DOUBLE_EQ(serObj["lat"].as<double>(), 48.8566);
    // Missing fields get the "|" default from applyConfig
    EXPECT_DOUBLE_EQ(serObj["lon"].as<double>(), 0.0);
    EXPECT_EQ(serObj["interval"].as<uint32_t>(), 600000u);
    EXPECT_STREQ(serObj["units"].as<const char*>(), "metric");
}

// =============================================================================
// IPreconditionContributor — Precondition Logic
// =============================================================================

TEST_F(WeatherModuleTest, PreconditionRegistrationAndMetadata) {
    PreconditionRegistry reg;
    module.registerPreconditions(reg);

    EXPECT_EQ(reg.count(), 1u);
    const auto& spec = reg.get(0);
    EXPECT_STREQ(spec.name, "Weather Setup");
    EXPECT_EQ(spec.role, PreconditionRole::Operational);
    EXPECT_EQ(spec.edgeCount, 1u);
    EXPECT_STREQ(spec.edges[0].name, PreconditionNames::WIFI_STATION);
    EXPECT_EQ(spec.edges[0].kind, EdgeKind::Requires);
}

TEST_F(WeatherModuleTest, PreconditionNoKeyManager_ReturnsIndeterminate) {
    // Without calling begin(), _keyManager is nullptr.
    // §2.4: Must return Indeterminate (defect fix: not true / not met).
    PreconditionRegistry reg;
    module.registerPreconditions(reg);

    ASSERT_EQ(reg.count(), 1u);
    PreconditionStatus status = reg.get(0).evaluate();
    EXPECT_FALSE(status.met);
    EXPECT_EQ(status.cause, Cause::Indeterminate);
}

TEST_F(WeatherModuleTest, PreconditionDescriptions) {
    PreconditionRegistry reg;
    module.registerPreconditions(reg);

    ASSERT_EQ(reg.count(), 1u);
    ASSERT_NE(reg.get(0).describe, nullptr);

    char buf[128] = {};
    reg.get(0).describe(Cause::NotConfigured, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Set API key and location in Weather settings");

    reg.get(0).describe(Cause::Indeterminate, buf, sizeof(buf));
    EXPECT_STREQ(buf, "BUG: KeyManager not wired — call setKeyManager()");

    reg.get(0).describe(Cause::Connecting, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Weather setup incomplete");
}

// =============================================================================
// IConfigurable — Metadata
// =============================================================================

TEST_F(WeatherModuleTest, ConfigNamespace) {
    EXPECT_STREQ(module.getConfigNamespace(), "weather");
}

TEST_F(WeatherModuleTest, SchemaVersion) {
    EXPECT_EQ(module.getSchemaVersion(), 1);
}

// =============================================================================
// IDataSource — Metadata
// =============================================================================

TEST_F(WeatherModuleTest, SourceName) {
    EXPECT_STREQ(module.getSourceName(), "OpenWeatherMap");
}

TEST_F(WeatherModuleTest, PriorityTier) {
    EXPECT_EQ(module.getPriorityTier(), PriorityTier::Normal);
}

TEST_F(WeatherModuleTest, DefaultUpdateInterval) {
    // Default should be 600000ms (10 minutes)
    EXPECT_EQ(module.getUpdateIntervalMs(), 600000u);
}

TEST_F(WeatherModuleTest, UpdateIntervalReflectsConfig) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["interval"] = 120000;
    module.applyConfig(obj);

    EXPECT_EQ(module.getUpdateIntervalMs(), 120000u);
}

// =============================================================================
// Thread-Safe Getters — Initial State
// =============================================================================

TEST_F(WeatherModuleTest, InitialDataState) {
    EXPECT_FALSE(module.hasData());
    EXPECT_FLOAT_EQ(module.getTemperature(), 0.0f);
    EXPECT_FLOAT_EQ(module.getWindSpeed(), 0.0f);
    EXPECT_EQ(module.getHumidity(), 0);
    EXPECT_EQ(module.getPressure(), 0);
}

TEST_F(WeatherModuleTest, GetUnitsDefault) {
    EXPECT_EQ(module.getUnits(), "metric");
}

TEST_F(WeatherModuleTest, GetUnitsAfterConfig) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["units"] = "imperial";
    module.applyConfig(obj);

    EXPECT_EQ(module.getUnits(), "imperial");
}

// =============================================================================
// MCP Tool Registration
// =============================================================================

TEST_F(WeatherModuleTest, TabMetadata) {
    EXPECT_STREQ(module.getTabName(), "Weather");
    EXPECT_EQ(module.getParentTab(), nullptr);
    EXPECT_EQ(module.getTabOrder(), 50);
    EXPECT_NE(module.getTabIcon(), nullptr);
    EXPECT_STREQ(module.getTabScript(), "/weather.js");
    EXPECT_STREQ(module.getApiPrefix(), "/api/weather");
    EXPECT_EQ(module.getMaxPostBody(), 4096u);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
