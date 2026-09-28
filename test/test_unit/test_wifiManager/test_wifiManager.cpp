/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_wifiManager/test_wifiManager.cpp
 * Description: Implementation of WifiManagerTest class.
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "../../../modules/WifiManager/WifiManager.hpp"
#include "../test_helpers/MockTabRegistry.hpp"

class WifiManagerTest : public ::testing::Test {
protected:
    WifiManager wifiManager;
};

// Construction defaults
TEST_F(WifiManagerTest, ConstructionDefaults) {
    EXPECT_STREQ("", wifiManager.getSSID());
    EXPECT_FALSE(wifiManager.hasPassword());
    EXPECT_FALSE(wifiManager.getAPMode());
    EXPECT_FALSE(wifiManager.isReady());
}

// begin() initializes state machine
TEST_F(WifiManagerTest, BeginInitializesStateMachine) {
    wifiManager.begin();
    EXPECT_FALSE(wifiManager.isReady()); // In WIFI_INIT state
}

// setDefaultHostname() seeds resetConfig()'s factory default
TEST_F(WifiManagerTest, SetDefaultHostnameSeedsResetConfig) {
    wifiManager.setDefaultHostname("my-app-default");
    wifiManager.resetConfig();
    EXPECT_STREQ("my-app-default", wifiManager.getHostname());
}

// resetConfig() falls back to the framework default when no app default was set
TEST_F(WifiManagerTest, ResetConfigUsesFrameworkDefaultHostname) {
    wifiManager.resetConfig();
    EXPECT_STREQ("esp32-device", wifiManager.getHostname());
}

// getHostname() reflects the persisted/applied hostname
TEST_F(WifiManagerTest, GetHostnameReflectsAppliedConfig) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["hostname"] = "applied-host";
    wifiManager.applyConfig(section);
    EXPECT_STREQ("applied-host", wifiManager.getHostname());
}

// updateWiFi() stores SSID
TEST_F(WifiManagerTest, UpdateWiFiStoresSSID) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    EXPECT_STREQ("MySSID", wifiManager.getSSID());
    EXPECT_TRUE(wifiManager.hasPassword());
}

// updateWiFi() with empty pass
TEST_F(WifiManagerTest, UpdateWiFiEmptyPass) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    wifiManager.updateWiFi("NewSSID", "");
    EXPECT_STREQ("NewSSID", wifiManager.getSSID());
    EXPECT_TRUE(wifiManager.hasPassword());
}

// updateWiFi() with nullptr pass
TEST_F(WifiManagerTest, UpdateWiFiNullptrPass) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    wifiManager.updateWiFi("NewSSID", nullptr);
    EXPECT_STREQ("NewSSID", wifiManager.getSSID());
    EXPECT_TRUE(wifiManager.hasPassword());
}

// resetConfig()
TEST_F(WifiManagerTest, ResetConfig) {
    wifiManager.updateWiFi("MySSID", "MyPass");
    wifiManager.resetConfig();
    EXPECT_STREQ("", wifiManager.getSSID());
    EXPECT_FALSE(wifiManager.hasPassword());
}

// testConnection()
TEST_F(WifiManagerTest, TestConnection) {
    std::string ipOut;
    bool result = wifiManager.testConnection("MySSID", "MyPass", ipOut);
    EXPECT_TRUE(result);
    EXPECT_STREQ("192.168.1.100", ipOut.c_str());
}

// getMyUrl default
TEST_F(WifiManagerTest, GetMyUrlDefault) {
    EXPECT_STREQ("http://0.0.0.0", wifiManager.getMyUrl());
}

// isWifiPersistentError()
TEST_F(WifiManagerTest, IsWifiPersistentError) {
    EXPECT_FALSE(wifiManager.isWifiPersistentError());
}

// notifyApiActivity()
TEST_F(WifiManagerTest, NotifyApiActivity) {
    wifiManager.notifyApiActivity();
    SUCCEED();
}

// Multiple begin() calls
TEST_F(WifiManagerTest, MultipleBeginCalls) {
    wifiManager.begin();
    wifiManager.begin();
    SUCCEED();
}

// SSID boundary
TEST_F(WifiManagerTest, SSIDBoundary) {
    wifiManager.updateWiFi("12345678901234567890123456789012", "pass");
    EXPECT_STREQ("12345678901234567890123456789012", wifiManager.getSSID());
}

// Password boundary
TEST_F(WifiManagerTest, PasswordBoundary) {
    wifiManager.updateWiFi("ssid", "1234567890123456789012345678901234567890123456789012345678901234");
    EXPECT_TRUE(wifiManager.hasPassword());
}

// SSID overflow
TEST_F(WifiManagerTest, SSIDOverflow) {
    wifiManager.updateWiFi("12345678901234567890123456789012345", "pass");
    EXPECT_STREQ("12345678901234567890123456789012", wifiManager.getSSID());
}

// registerTabs()
TEST_F(WifiManagerTest, RegisterTabs_WiFiTopLevel) {
    MockTabRegistry collector;
    wifiManager.registerTabs(collector);
    ASSERT_EQ(collector.entries.size(), 1u);
    EXPECT_EQ(collector.entries[0].name, "WiFi");
    EXPECT_EQ(collector.entries[0].parent, "");
    EXPECT_EQ(collector.entries[0].order, 10);
}

// getConfigNamespace()
TEST_F(WifiManagerTest, GetConfigNamespace) {
    EXPECT_STREQ("wifi", wifiManager.getConfigNamespace());
}

// getSchemaVersion()
TEST_F(WifiManagerTest, GetSchemaVersion) {
    EXPECT_EQ(1, wifiManager.getSchemaVersion());
}

// applyConfig with valid JSON
TEST_F(WifiManagerTest, ApplyConfig_SetsSSIDAndHostname) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["ssid"] = "TestNetwork";
    section["hostname"] = "test-device";

    wifiManager.applyConfig(section);
    EXPECT_STREQ("TestNetwork", wifiManager.getSSID());
}

// applyConfig with empty JSON
TEST_F(WifiManagerTest, ApplyConfig_EmptySection_DoesNotCrash) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    EXPECT_NO_FATAL_FAILURE(wifiManager.applyConfig(section));
}

// serializeConfig produces expected keys
TEST_F(WifiManagerTest, SerializeConfig_ProducesExpectedKeys) {
    wifiManager.updateWiFi("MySSID", "MyPass");

    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    wifiManager.serializeConfig(section);

    EXPECT_TRUE(section["ssid"].is<JsonVariant>());
    EXPECT_TRUE(section["hostname"].is<JsonVariant>());
    EXPECT_STREQ("MySSID", section["ssid"].as<const char*>());
}

// serializeConfig roundtrip
TEST_F(WifiManagerTest, SerializeApplyRoundtrip_Consistent) {
    wifiManager.updateWiFi("RoundtripSSID", "pass");

    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    wifiManager.serializeConfig(section);

    WifiManager wm2;
    wm2.applyConfig(section);
    EXPECT_STREQ("RoundtripSSID", wm2.getSSID());
}

// ---------------------------------------------------------------------------
// STA resilience / setup-mode policy (issue #236)
//
// Stub limitations: test/stubs/platform/WiFi.h reports WL_CONNECTED
// unconditionally and WiFi.mode() is a no-op without a getter, so the native
// suite cannot drive a real STA failure through tick() nor observe the radio
// mode. The failure-path decisions are therefore factored into the pure
// policy helpers staRetryDelayMs() / shouldStartFallbackAp(), which tick()
// uses directly and which are tested here; tick() itself is driven through
// the setup and credentials paths the stub can express.
// ---------------------------------------------------------------------------

#include "EventBus.hpp"

namespace {
struct EventCounter {
    int connected = 0;
    int apStarted = 0;
    static void onConnected(const char*, const void*, void* ctx) { static_cast<EventCounter*>(ctx)->connected++; }
    static void onApStarted(const char*, const void*, void* ctx) { static_cast<EventCounter*>(ctx)->apStarted++; }
};
} // namespace

// Backoff between STA attempts: 5 s, 10 s, 30 s, then every 60 s.
TEST(WifiReconnectPolicyTest, StaRetryBackoffSchedule) {
    EXPECT_EQ(5000u,  WifiManager::staRetryDelayMs(1));
    EXPECT_EQ(10000u, WifiManager::staRetryDelayMs(2));
    EXPECT_EQ(30000u, WifiManager::staRetryDelayMs(3));
    EXPECT_EQ(60000u, WifiManager::staRetryDelayMs(4));
    EXPECT_EQ(60000u, WifiManager::staRetryDelayMs(5));
    EXPECT_EQ(60000u, WifiManager::staRetryDelayMs(1000));
}

// Fallback AP (AP+STA) starts only after > 3 min of continuous STA downtime,
// only when credentials exist, and not while an AP is up or it was dropped for idleness.
TEST(WifiReconnectPolicyTest, FallbackApStartsOnlyAfterThreeMinutesWithCredentials) {
    EXPECT_FALSE(WifiManager::shouldStartFallbackAp(true, false, false, 0));
    EXPECT_FALSE(WifiManager::shouldStartFallbackAp(true, false, false, 60000));   // 60 s router reboot
    EXPECT_FALSE(WifiManager::shouldStartFallbackAp(true, false, false, 180000));  // exactly 3 min
    EXPECT_TRUE (WifiManager::shouldStartFallbackAp(true, false, false, 180001));  // > 3 min
    EXPECT_TRUE (WifiManager::shouldStartFallbackAp(true, false, false, 600000));  // 10 min outage
    EXPECT_FALSE(WifiManager::shouldStartFallbackAp(false, false, false, 600000)); // no creds: setup path owns the AP
    EXPECT_FALSE(WifiManager::shouldStartFallbackAp(true, true, false, 600000));   // already up
    EXPECT_FALSE(WifiManager::shouldStartFallbackAp(true, false, true, 600000));   // dropped after 1 h idle
}

// Cumulative timeline for a 60 s outage under the schedule: the fallback AP
// cannot start before STA has had several attempts, and never within 3 min.
TEST(WifiReconnectPolicyTest, BackoffScheduleReachesFallbackAfterAboutThreeMinutes) {
    const uint32_t graceMs = 10000, attemptMs = 15000;
    uint32_t t = graceMs;  // first attempt after link-loss grace
    int attempts = 0;
    bool apStarted = false;
    uint32_t apAt = 0;
    while (t < 600000 && !apStarted) {
        t += attemptMs;                      // attempt times out
        attempts++;
        uint32_t backoffEnd = t + WifiManager::staRetryDelayMs(attempts);
        // fallback AP is evaluated during backoff
        if (WifiManager::shouldStartFallbackAp(true, false, false, backoffEnd)) {
            apStarted = true;
            apAt = (t > 180000) ? t : 180001;
        }
        t = backoffEnd;
    }
    EXPECT_TRUE(apStarted);
    EXPECT_GE(attempts, 4);
    EXPECT_GT(apAt, 180000u);
    EXPECT_LT(apAt, 250000u);  // "about 3 min"
}

// First boot, no credentials: setup AP with auth bypass (unchanged behaviour).
TEST(WifiSetupModeTest, NoCredentialsStartsSetupAp) {
    WifiManager wifi;
    EventBus bus;
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_AP_STARTED, &EventCounter::onApStarted, &counter);
    wifi.setEventBus(&bus);

    wifi.begin();
    wifi.tick(2000);   // Init -> ApStarting
    wifi.tick(2001);   // ApStarting -> Ready (AP up)
    EXPECT_TRUE(wifi.getAPMode());
    EXPECT_TRUE(wifi.isSetupMode());
    EXPECT_TRUE(wifi.isReady());
    EXPECT_EQ(1, counter.apStarted);
}

// Credentials arriving while in setup mode end the bypass immediately and
// the device leaves AP mode for STA.
TEST(WifiSetupModeTest, CredentialsEndSetupMode) {
    WifiManager wifi;
    EventBus bus;
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_CONNECTED, &EventCounter::onConnected, &counter);
    wifi.setEventBus(&bus);

    wifi.begin();
    wifi.tick(2000);
    wifi.tick(2001);
    ASSERT_TRUE(wifi.isSetupMode());

    wifi.updateWiFi("HomeNet", "secret");
    EXPECT_FALSE(wifi.isSetupMode());   // AP still up, but credentials exist: no bypass

    wifi.tick(3000);                    // deferred reconnect: AP torn down -> Init
    EXPECT_FALSE(wifi.getAPMode());
    wifi.tick(5000);                    // Init -> STA attempt
    wifi.tick(5001);                    // StaConnecting -> connected (stub)
    EXPECT_TRUE(wifi.isReady());
    EXPECT_FALSE(wifi.getAPMode());
    EXPECT_FALSE(wifi.isSetupMode());
    EXPECT_EQ(1, counter.connected);
}

// With credentials the device goes STA, never AP, and isSetupMode() is never true.
TEST(WifiSetupModeTest, WithCredentialsNeverEntersApOrSetupMode) {
    WifiManager wifi;
    EventBus bus;
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_AP_STARTED, &EventCounter::onApStarted, &counter);
    wifi.setEventBus(&bus);

    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["ssid"] = "HomeNet";
    wifi.applyConfig(section);
    EXPECT_FALSE(wifi.isSetupMode());

    wifi.begin();
    for (uint32_t t = 0; t <= 600000; t += 500) {
        wifi.tick(t);
        EXPECT_FALSE(wifi.getAPMode());
        EXPECT_FALSE(wifi.isSetupMode());
    }
    EXPECT_TRUE(wifi.isReady());
    EXPECT_EQ(0, counter.apStarted);
}

#include "PreconditionEngine.hpp"
#include "WifiPreconditionContributor.hpp"

// ---------------------------------------------------------------------------
// WifiPreconditionContributor Tests
// ---------------------------------------------------------------------------

TEST(WifiPreconditionContributorTest, RegistersSingleStartupPrecondition) {
    WifiManager wifi;
    WifiPreconditionContributor contributor(wifi);
    PreconditionEngine engine;

    contributor.registerPreconditions(engine);

    EXPECT_EQ(engine.getPreconditionCount(), 1);
    const PreconditionState* state = engine.getStateByName(PreconditionNames::WIFI_STATION);
    EXPECT_NE(state, nullptr);
    EXPECT_EQ(state->role, PreconditionRole::Startup);
    EXPECT_EQ(state->resolvedEdgeCount, 0);
}

TEST(WifiPreconditionContributorTest, EvaluationAndDescriptions) {
    WifiManager wifi;
    WifiPreconditionContributor contributor(wifi);
    PreconditionEngine engine;
    contributor.registerPreconditions(engine);
    EXPECT_TRUE(engine.resolveEdges());

    // 1. Initial / unconfigured state -> NotConfigured -> NeedsUser
    engine.tick(1000);
    const PreconditionState* state = engine.getStateByName(PreconditionNames::WIFI_STATION);
    EXPECT_NE(state, nullptr);
    EXPECT_FALSE(state->rawStatus.met);
    EXPECT_EQ(state->rawStatus.cause, Cause::NotConfigured);
    EXPECT_EQ(state->severity, Severity::NeedsUser);

    char desc[128] = {};
    state->describe(Cause::NotConfigured, desc, sizeof(desc), state->context);
    EXPECT_STREQ("Connect to the device WiFi to configure", desc);

    // 2. Connecting with configured credentials
    wifi.updateWiFi("MyNetwork", "secret");
    engine.tick(2000);
    state = engine.getStateByName(PreconditionNames::WIFI_STATION);
    EXPECT_NE(state, nullptr);
    EXPECT_FALSE(state->rawStatus.met);
    EXPECT_EQ(state->rawStatus.cause, Cause::Connecting);
    EXPECT_EQ(state->severity, Severity::Transient);

    state->describe(Cause::Connecting, desc, sizeof(desc), state->context);
    EXPECT_STREQ("Connecting to MyNetwork...", desc);

    // 3. Fallback / Rejected / default describe messages
    state->describe(Cause::FallbackEngaged, desc, sizeof(desc), state->context);
    EXPECT_TRUE(strstr(desc, "Can't reach your network") != nullptr);

    state->describe(Cause::Rejected, desc, sizeof(desc), state->context);
    EXPECT_STREQ("WiFi password was refused", desc);

    state->describe(Cause::Unreachable, desc, sizeof(desc), state->context);
    EXPECT_STREQ("WiFi unavailable", desc);

    // 4. Severity derivation for FallbackEngaged (AP mode)
    EXPECT_EQ(deriveSeverity(Cause::FallbackEngaged, false), Severity::Degraded);
    EXPECT_EQ(deriveSeverity(Cause::FallbackEngaged, true), Severity::Degraded);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

