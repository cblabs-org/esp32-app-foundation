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

TEST_F(WeatherModuleTest, LocationConfiguredWithKeys) {
    EXPECT_FALSE(module.isLocationConfigured());
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["lat"] = 51.5074;
    obj["lon"] = -0.1278;
    module.applyConfig(obj);
    EXPECT_TRUE(module.isLocationConfigured());
}

TEST_F(WeatherModuleTest, LocationConfiguredNonDefaultValues) {
    EXPECT_FALSE(module.isLocationConfigured());
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["lat"] = 40.7128;
    module.applyConfig(obj);
    EXPECT_TRUE(module.isLocationConfigured());
}

TEST_F(WeatherModuleTest, LocationConfiguredZeroZeroWithKeys) {
    EXPECT_FALSE(module.isLocationConfigured());
    // Explicit 0.0, 0.0 in config (Gulf of Guinea) should be marked configured
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["lat"] = 0.0;
    obj["lon"] = 0.0;
    module.applyConfig(obj);
    EXPECT_TRUE(module.isLocationConfigured());
}

TEST_F(WeatherModuleTest, LocationResetRestoresUnconfigured) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["lat"] = 51.5074;
    obj["lon"] = -0.1278;
    module.applyConfig(obj);
    EXPECT_TRUE(module.isLocationConfigured());

    module.resetConfig();
    EXPECT_FALSE(module.isLocationConfigured());
}

TEST_F(WeatherModuleTest, LocationEmptyConfigRemainsUnconfigured) {
    EXPECT_FALSE(module.isLocationConfigured());
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    module.applyConfig(obj);
    EXPECT_FALSE(module.isLocationConfigured());
}

// =============================================================================
// IPreconditionContributor — Precondition Logic
// =============================================================================

struct MockPreconditionRegistry : public IPreconditionRegistry {
    struct Spec {
        const char* name;
        PreconditionRole role;
        EvalFn evalFn;
        DescribeFn describeFn;
        void* context;
        const Edge* edges;
        uint8_t edgeCount;

        PreconditionStatus evaluate() const {
            return evalFn ? evalFn(context) : PreconditionStatus{};
        }

        void describe(Cause c, char* buf, size_t len) const {
            if (describeFn) describeFn(c, buf, len, context);
        }
    };
    std::vector<Spec> specs;
    void add(const char* name, PreconditionRole role, EvalFn evaluate,
             DescribeFn describe = nullptr, void* context = nullptr,
             const Edge* edges = nullptr, uint8_t edgeCount = 0,
             const DampingPolicy* damping = nullptr,
             const RetryPolicy* retry = nullptr) override {
        (void)damping;
        (void)retry;
        specs.push_back({name, role, evaluate, describe, context, edges, edgeCount});
    }
    size_t count() const { return specs.size(); }
    const Spec& get(size_t i) const { return specs[i]; }
};

TEST_F(WeatherModuleTest, PreconditionRegistrationAndMetadata) {
    MockPreconditionRegistry reg;
    module.registerPreconditions(reg);

    EXPECT_EQ(reg.count(), 3u);

    // Precondition 0: Weather API Key
    const auto& specKey = reg.get(0);
    EXPECT_STREQ(specKey.name, "Weather API Key");
    EXPECT_EQ(specKey.role, PreconditionRole::Startup);
    EXPECT_EQ(specKey.edgeCount, 1u);
    EXPECT_STREQ(specKey.edges[0].name, PreconditionNames::WIFI_STATION);
    EXPECT_EQ(specKey.edges[0].kind, EdgeKind::Requires);

    // Precondition 1: Weather Location
    const auto& specLoc = reg.get(1);
    EXPECT_STREQ(specLoc.name, "Weather Location");
    EXPECT_EQ(specLoc.role, PreconditionRole::Startup);
    EXPECT_EQ(specLoc.edgeCount, 1u);
    EXPECT_STREQ(specLoc.edges[0].name, PreconditionNames::WIFI_STATION);
    EXPECT_EQ(specLoc.edges[0].kind, EdgeKind::Requires);

    // Precondition 2: Weather API
    const auto& specApi = reg.get(2);
    EXPECT_STREQ(specApi.name, "Weather API");
    EXPECT_EQ(specApi.role, PreconditionRole::Operational);
    EXPECT_EQ(specApi.edgeCount, 2u);
    EXPECT_STREQ(specApi.edges[0].name, "Weather API Key");
    EXPECT_STREQ(specApi.edges[1].name, "Weather Location");
}

TEST_F(WeatherModuleTest, PreconditionWeatherLocation_ReflectsConfiguredState) {
    MockPreconditionRegistry reg;
    module.registerPreconditions(reg);

    ASSERT_EQ(reg.count(), 3u);
    const auto& specLoc = reg.get(1);

    // Initially not configured
    EXPECT_FALSE(specLoc.evaluate().met);
    EXPECT_EQ(specLoc.evaluate().cause, Cause::NotConfigured);

    // Configure location
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["lat"] = 51.5074;
    obj["lon"] = -0.1278;
    module.applyConfig(obj);

    EXPECT_TRUE(specLoc.evaluate().met);
    EXPECT_EQ(specLoc.evaluate().cause, Cause::Met);

    // Reset config
    module.resetConfig();
    EXPECT_FALSE(specLoc.evaluate().met);
    EXPECT_EQ(specLoc.evaluate().cause, Cause::NotConfigured);
}

TEST_F(WeatherModuleTest, PreconditionNoKeyManager_ReturnsIndeterminate) {
    // Without calling begin(), _keyManager is nullptr.
    MockPreconditionRegistry reg;
    module.registerPreconditions(reg);

    ASSERT_EQ(reg.count(), 3u);
    PreconditionStatus status = reg.get(0).evaluate();
    EXPECT_FALSE(status.met);
    EXPECT_EQ(status.cause, Cause::Indeterminate);
}

TEST_F(WeatherModuleTest, PreconditionDescriptions) {
    MockPreconditionRegistry reg;
    module.registerPreconditions(reg);

    ASSERT_EQ(reg.count(), 3u);
    ASSERT_NE(reg.get(0).describeFn, nullptr);
    ASSERT_NE(reg.get(1).describeFn, nullptr);
    ASSERT_NE(reg.get(2).describeFn, nullptr);

    char buf[128] = {};
    reg.get(0).describe(Cause::NotConfigured, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Enter your OpenWeatherMap API key in the Weather tab");

    reg.get(0).describe(Cause::Indeterminate, buf, sizeof(buf));
    EXPECT_STREQ(buf, "BUG: KeyManager not wired — call begin(app)");

    reg.get(1).describe(Cause::NotConfigured, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Set latitude and longitude in the Weather tab");

    reg.get(2).describe(Cause::Rejected, buf, sizeof(buf));
    EXPECT_STREQ(buf, "API key rejected — check your OpenWeatherMap key");

    reg.get(2).describe(Cause::Unreachable, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Cannot reach OpenWeatherMap API — check internet");

    reg.get(2).describe(Cause::Connecting, buf, sizeof(buf));
    EXPECT_STREQ(buf, "Waiting for first weather data fetch...");
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
    // Inline mock — sample-app is a separate PlatformIO project; can't
    // reach the parent framework's test/test_unit/test_helpers/.
    struct MockTabRegistry : public ITabRegistry {
        struct Entry { std::string name; std::string parent; int order; std::string icon; std::string script; };
        std::vector<Entry> entries;
        using ITabRegistry::addTab;  // Expose TabEntry convenience overload
        void addTab(const char* name, const char* parent, int order,
                    const char* icon, const char* script) override {
            entries.push_back({name ? name : "", parent ? parent : "", order,
                               icon ? icon : "", script ? script : ""});
        }
    };
    MockTabRegistry collector;
    module.registerTabs(collector);
    ASSERT_EQ(collector.entries.size(), 1u);
    EXPECT_EQ(collector.entries[0].name, "Weather");
    EXPECT_EQ(collector.entries[0].parent, "");
    EXPECT_EQ(collector.entries[0].order, 50);
    EXPECT_FALSE(collector.entries[0].icon.empty());
    EXPECT_EQ(collector.entries[0].script, "/weather.js");

    EXPECT_NE(module.getTabIcon(), nullptr);
    EXPECT_STREQ(module.getApiPrefix(), "/api/weather");
    EXPECT_EQ(module.getMaxPostBody(), 4096u);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
