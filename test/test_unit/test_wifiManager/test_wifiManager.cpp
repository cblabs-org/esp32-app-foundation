/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_wifiManager/test_wifiManager.cpp
 * Description: Implementation of WifiManagerTest class.
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include <PsychicHttp.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <filesystem>
#include <functional>
#include "../../../modules/WifiManager/WifiManager.hpp"
#include "ConfigManager.hpp"
#include "ConfigRoot.hpp"
#include "DeviceCrypto.hpp"
#include "KeyManager.hpp"
#include "MCPToolRegistry.hpp"
#include "../test_helpers/MockTabRegistry.hpp"

class WifiManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        WiFi.resetTestStubs();
    }
    WifiManager wifiManager;
};

// Construction defaults
TEST_F(WifiManagerTest, ConstructionDefaults) {
    EXPECT_STREQ("", wifiManager.getSSID());
    EXPECT_FALSE(wifiManager.hasPassword());
    EXPECT_FALSE(wifiManager.getAPMode());
    EXPECT_FALSE(wifiManager.isReady());
    EXPECT_STREQ(WifiManager::DEFAULT_AP_SSID, wifiManager.getAPSSID());
    EXPECT_STREQ(WifiManager::DEFAULT_HOSTNAME, wifiManager.getHostname());
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

// getDefaultHostname() returns framework default or app-set default
TEST_F(WifiManagerTest, GetDefaultHostnameReturnsExpectedValue) {
    EXPECT_STREQ(WifiManager::DEFAULT_HOSTNAME, wifiManager.getDefaultHostname());
    wifiManager.setDefaultHostname("my-app-default");
    EXPECT_STREQ("my-app-default", wifiManager.getDefaultHostname());
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

// setDefaultHostname() immediately updates getHostname() and getAPSSID() (issue #294)
TEST_F(WifiManagerTest, SetDefaultHostnameImmediatelyUpdatesHostnameAndAPSSID) {
    wifiManager.setDefaultHostname("my-device");
    EXPECT_STREQ("my-device", wifiManager.getHostname());
    EXPECT_STREQ("my-device", wifiManager.getAPSSID());
}

// setDefaultHostname() called multiple times updates both default and current hostname (issue #294)
TEST_F(WifiManagerTest, SetDefaultHostnameMultipleTimesUpdatesCurrentHostname) {
    wifiManager.setDefaultHostname("first-default");
    EXPECT_STREQ("first-default", wifiManager.getHostname());
    EXPECT_STREQ("first-default", wifiManager.getAPSSID());

    wifiManager.setDefaultHostname("second-default");
    EXPECT_STREQ("second-default", wifiManager.getDefaultHostname());
    EXPECT_STREQ("second-default", wifiManager.getHostname());
    EXPECT_STREQ("second-default", wifiManager.getAPSSID());
}

// setDefaultHostname() does not overwrite user-configured hostname from applyConfig() (issue #294)
TEST_F(WifiManagerTest, SetDefaultHostnameDoesNotOverwriteUserConfiguredHostname) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["hostname"] = "user-configured";
    wifiManager.applyConfig(section);
    EXPECT_STREQ("user-configured", wifiManager.getHostname());

    wifiManager.setDefaultHostname("new-default");
    EXPECT_STREQ("new-default", wifiManager.getDefaultHostname());
    EXPECT_STREQ("user-configured", wifiManager.getHostname());
    EXPECT_STREQ("user-configured", wifiManager.getAPSSID());
}

// applyConfig() with generic "esp32-device" does not overwrite app-supplied default (issue #294)
TEST_F(WifiManagerTest, ApplyConfigWithGenericDefaultDoesNotOverwriteAppDefault) {
    wifiManager.setDefaultHostname("my-device");
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["hostname"] = "esp32-device";
    wifiManager.applyConfig(section);
    EXPECT_STREQ("my-device", wifiManager.getHostname());
    EXPECT_STREQ("my-device", wifiManager.getAPSSID());
}

// SoftAP uses default AP SSID ("ESP32-Setup") when unconfigured (issue #294)
TEST(WifiSetupModeTest, SoftAPUsesDefaultAPSSIDWhenUnconfigured) {
    WiFi.resetTestStubs();
    WifiManager wifi;
    EXPECT_STREQ(WifiManager::DEFAULT_AP_SSID, wifi.getAPSSID());
    wifi.begin();
    wifi.tick(2000);   // Init -> ApStarting
    wifi.tick(2001);   // ApStarting -> Ready (AP up)
    EXPECT_TRUE(wifi.getAPMode());
    EXPECT_STREQ("ESP32-Setup", wifi.getAPSSID());
    EXPECT_EQ("ESP32-Setup", WiFi.last_softAP_ssid);
}

// SoftAP uses app-supplied hostname as SSID (issue #294)
TEST(WifiSetupModeTest, SoftAPUsesAppSuppliedHostname) {
    WiFi.resetTestStubs();
    WifiManager wifi;
    wifi.setDefaultHostname("weather-station");
    wifi.begin();
    wifi.tick(2000);   // Init -> ApStarting
    wifi.tick(2001);   // ApStarting -> Ready (AP up)
    EXPECT_TRUE(wifi.getAPMode());
    EXPECT_STREQ("weather-station", wifi.getAPSSID());
    EXPECT_EQ("weather-station", WiFi.last_softAP_ssid);
}

// onStaConnected initialises mDNS with the effective hostname (issue #232)
TEST_F(WifiManagerTest, OnStaConnectedInitialisesMDNS) {
    MDNS.end();
    wifiManager.setDefaultHostname("sensor-node");
    wifiManager.resetConfig();
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["ssid"] = "HomeNet";
    wifiManager.applyConfig(section);
    wifiManager.begin();
    wifiManager.tick(2000); // Init -> StaConnecting
    wifiManager.tick(2001); // StaConnecting -> connected -> onStaConnected
    EXPECT_TRUE(MDNS.began);
    EXPECT_EQ("sensor-node", MDNS.hostname);
    EXPECT_EQ("http", MDNS.service);
    EXPECT_EQ("tcp", MDNS.proto);
    EXPECT_EQ(80, MDNS.port);
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

// ---------------------------------------------------------------------------
// SoftAP provisioning race & HTTP route tests (issue #332)
//
// Note: test/stubs/platform/WiFi.h stubs WL_CONNECTED and WiFi.mode() as
// no-ops, and FreeRTOSMutex is a no-op stub on native. These tests exercise
// the cross-core flag/staging sequencing deterministically by pumping
// WifiManager::tick() mid-KeyManager encryption and mid-response->send()
// during POST /api/wifi/connect, POST /api/wifi/forget, and updateWiFi().
// ---------------------------------------------------------------------------

namespace {

class MockWifiHttpRequest : public PsychicRequest {
public:
    explicit MockWifiHttpRequest(std::string bodyStr = "", size_t overrideLen = 0)
        : _body(std::move(bodyStr)), _overrideLen(overrideLen) {}
    const char* bodyCStr() override { return _body.c_str(); }
    size_t contentLength() override { return _overrideLen > 0 ? _overrideLen : _body.size(); }
private:
    std::string _body;
    size_t _overrideLen;
};

class MockWifiHttpResponse : public PsychicResponse {
public:
    std::function<void()> onSend;
    int lastCode = 0;
    std::string lastContentType;
    std::string lastBody;

    esp_err_t send(int code, const char* contentType, const char* content) override {
        lastCode = code;
        lastContentType = contentType ? contentType : "";
        lastBody = content ? content : "";
        if (onSend) {
            onSend();
        }
        return ESP_OK;
    }
};

class EncryptHookDeviceCrypto : public DeviceCrypto {
public:
    std::function<void()> onEncrypt;
    bool failEncrypt = false;

    std::unique_ptr<char[]> encrypt(const char* plaintext, size_t inputLen, size_t* outLen) override {
        if (onEncrypt) {
            onEncrypt();
        }
        if (failEncrypt || !plaintext || inputLen == 0) {
            if (outLen) *outLen = 0;
            return nullptr;
        }
        std::string enc = "ENC:" + std::string(plaintext, inputLen);
        if (outLen) *outLen = enc.length();
        char* buf = new char[enc.length() + 1];
        strcpy(buf, enc.c_str());
        return std::unique_ptr<char[]>(buf);
    }

    std::unique_ptr<char[]> decrypt(const char* ciphertext, size_t inputLen, size_t* outLen) override {
        if (!ciphertext || inputLen == 0) {
            if (outLen) *outLen = 0;
            return nullptr;
        }
        std::string ct(ciphertext, inputLen);
        if (ct.rfind("ENC:", 0) == 0) {
            std::string pt = ct.substr(4);
            if (outLen) *outLen = pt.length();
            char* buf = new char[pt.length() + 1];
            strcpy(buf, pt.c_str());
            return std::unique_ptr<char[]>(buf);
        }
        if (outLen) *outLen = 0;
        return nullptr;
    }
};

class WifiProvisioningTest : public ::testing::Test {
protected:
    void SetUp() override {
        WiFi.resetTestStubs();
        std::error_code ec;
        std::filesystem::create_directories(ConfigRoot::getEtcPath(), ec);
        std::filesystem::create_directories(ConfigRoot::getModPath(), ec);
        std::filesystem::remove(ConfigRoot::getEtcFilePath("keys.json").c_str(), ec);
        std::filesystem::remove(ConfigRoot::getModFilePath("wifi").c_str(), ec);
        std::filesystem::remove(ConfigRoot::getEtcFilePath("manifest.json").c_str(), ec);
        std::filesystem::remove(ConfigRoot::getModFilePath("manifest.json").c_str(), ec);

        keyManager.begin(&crypto);
        wifi.setKeyManager(&keyManager);
        wifi.setConfigManager(&configManager);
        wifi.setEventBus(&bus);
        configManager.registerModule(&wifi);
        configManager.beginAll();
        wifi.registerRoutes(server);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(ConfigRoot::getEtcFilePath("keys.json").c_str(), ec);
        std::filesystem::remove_all(ConfigRoot::getModFilePath("wifi").c_str(), ec);
        std::filesystem::remove(ConfigRoot::getEtcFilePath("manifest.json").c_str(), ec);
        std::filesystem::remove(ConfigRoot::getModFilePath("manifest.json").c_str(), ec);
    }

    PsychicHttpRequestCallback findRoute(const char* uri, int method) {
        for (const auto& r : server.routes) {
            if (r.uri == uri && r.method == method) {
                return r.handler;
            }
        }
        return nullptr;
    }

    EncryptHookDeviceCrypto crypto;
    KeyManager keyManager;
    ConfigManager configManager;
    EventBus bus;
    PsychicHttpServer server;
    WifiManager wifi;
};

} // namespace

TEST_F(WifiProvisioningTest, PostConnectDoesNotTearDownSoftApMidSaveOrMidResponse) {
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_CONNECTED, &EventCounter::onConnected, &counter);

    wifi.begin();
    wifi.tick(2000);   // Init -> ApStarting
    wifi.tick(2001);   // ApStarting -> Ready (first-boot setup AP active)
    ASSERT_TRUE(wifi.getAPMode());
    ASSERT_TRUE(wifi.isSetupMode());

    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);

    bool encryptHookRan = false;
    bool apAliveDuringEncrypt = false;
    bool credsHiddenDuringEncrypt = false;
    bool setupModeDuringEncrypt = false;

    crypto.onEncrypt = [&]() {
        encryptHookRan = true;
        // Simulate Core 1 tick() running while Core 0 is inside KeyManager flash I/O
        wifi.tick(2500);
        wifi.tick(2501);
        apAliveDuringEncrypt = wifi.getAPMode();
        credsHiddenDuringEncrypt = !wifi.hasCredentials();
        setupModeDuringEncrypt = wifi.isSetupMode();
    };

    bool sendHookRan = false;
    bool apAliveDuringSend = false;
    bool credsHiddenDuringSend = false;

    MockWifiHttpRequest req("{\"ssid\":\"HomeNet\",\"pass\":\"secret123\"}");
    MockWifiHttpResponse resp;
    resp.onSend = [&]() {
        sendHookRan = true;
        // Simulate Core 1 tick() running while Core 0 sends HTTP 200 over SoftAP
        wifi.tick(2600);
        wifi.tick(2601);
        apAliveDuringSend = wifi.getAPMode();
        credsHiddenDuringSend = !wifi.hasCredentials();
    };

    esp_err_t err = connectHandler(&req, &resp);
    EXPECT_EQ(ESP_OK, err);
    EXPECT_EQ(200, resp.lastCode);
    EXPECT_NE(resp.lastBody.find("Credentials saved. Restarting..."), std::string::npos);

    EXPECT_TRUE(encryptHookRan);
    EXPECT_TRUE(apAliveDuringEncrypt);
    EXPECT_TRUE(credsHiddenDuringEncrypt);
    EXPECT_TRUE(setupModeDuringEncrypt);

    EXPECT_TRUE(sendHookRan);
    EXPECT_TRUE(apAliveDuringSend);
    EXPECT_TRUE(credsHiddenDuringSend);

    // Credentials are now committed in RAM and persisted on disk
    EXPECT_STREQ("HomeNet", wifi.getSSID());
    EXPECT_TRUE(wifi.hasCredentials());
    EXPECT_TRUE(wifi.hasPassword());
    EXPECT_FALSE(wifi.isSetupMode());
    EXPECT_TRUE(wifi.getAPMode());
    EXPECT_EQ("secret123", keyManager.getKeyById("wifi_password"));

    // Verify a fresh WifiManager loading from persisted ConfigManager sees HomeNet
    WifiManager reloaded;
    ConfigManager reloadCm;
    reloaded.setKeyManager(&keyManager);
    reloaded.setConfigManager(&reloadCm);
    reloadCm.registerModule(&reloaded);
    reloadCm.beginAll();
    EXPECT_STREQ("HomeNet", reloaded.getSSID());
    EXPECT_TRUE(reloaded.hasPassword());

    // Next tick() executes deferred restart (without tearing down SoftAP via _reconnectRequested)
    wifi.tick(2700);
    EXPECT_TRUE(wifi.getAPMode());
    EXPECT_EQ(0, counter.connected);
}

TEST_F(WifiProvisioningTest, PostConnectEmptyPasswordPreservesStoredPasswordAndSchedulesRestart) {
    wifi.updateWiFi("OldNet", "oldpass123");
    wifi.tick(2000); // consume _reconnectRequested from updateWiFi

    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);

    MockWifiHttpRequest req("{\"ssid\":\"NewNet\",\"pass\":\"\"}");
    MockWifiHttpResponse resp;
    bool ssidWasOldDuringSend = false;
    resp.onSend = [&]() {
        ssidWasOldDuringSend = (std::string(wifi.getSSID()) == "OldNet");
    };

    esp_err_t err = connectHandler(&req, &resp);
    EXPECT_EQ(ESP_OK, err);
    EXPECT_EQ(200, resp.lastCode);
    EXPECT_TRUE(ssidWasOldDuringSend);
    EXPECT_STREQ("NewNet", wifi.getSSID());
    EXPECT_TRUE(wifi.hasPassword());
    EXPECT_EQ("oldpass123", keyManager.getKeyById("wifi_password"));
}

TEST_F(WifiProvisioningTest, PostConnectRejectsInvalidPayloadsWithoutMutatingState) {
    wifi.begin();
    wifi.tick(2000);
    wifi.tick(2001);
    ASSERT_TRUE(wifi.isSetupMode());

    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);

    {
        MockWifiHttpRequest req("{\"ssid\":\"HomeNet\"}", 2048);
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req, &resp));
        EXPECT_EQ(413, resp.lastCode);
    }
    {
        MockWifiHttpRequest req("");
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req, &resp));
        EXPECT_EQ(400, resp.lastCode);
    }
    {
        MockWifiHttpRequest req("{not-json");
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req, &resp));
        EXPECT_EQ(400, resp.lastCode);
    }
    {
        MockWifiHttpRequest req("{\"ssid\":\"\",\"pass\":\"secret\"}");
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req, &resp));
        EXPECT_EQ(400, resp.lastCode);
    }

    EXPECT_FALSE(wifi.hasCredentials());
    EXPECT_TRUE(wifi.isSetupMode());
    wifi.tick(2500);
    EXPECT_TRUE(wifi.getAPMode());
}

TEST_F(WifiProvisioningTest, PostForgetStagesErasureUntilHttpResponseCompletes) {
    wifi.updateWiFi("HomeNet", "secret123");
    wifi.tick(2000); // consume _reconnectRequested

    auto forgetHandler = findRoute("/api/wifi/forget", HTTP_POST);
    ASSERT_NE(forgetHandler, nullptr);

    bool credsStillPresentDuringSend = false;
    bool persistedEmptyDuringSend = false;
    bool keyDeletedDuringSend = false;

    MockWifiHttpRequest req("{}");
    MockWifiHttpResponse resp;
    resp.onSend = [&]() {
        wifi.tick(2500);
        credsStillPresentDuringSend = wifi.hasCredentials();
        keyDeletedDuringSend = keyManager.getKeyById("wifi_password").empty();

        WifiManager probe;
        ConfigManager probeCm;
        probe.setKeyManager(&keyManager);
        probe.setConfigManager(&probeCm);
        probeCm.registerModule(&probe);
        probeCm.beginAll();
        persistedEmptyDuringSend = !probe.hasCredentials();
    };

    esp_err_t err = forgetHandler(&req, &resp);
    EXPECT_EQ(ESP_OK, err);
    EXPECT_EQ(200, resp.lastCode);
    EXPECT_TRUE(credsStillPresentDuringSend);
    EXPECT_TRUE(persistedEmptyDuringSend);
    EXPECT_TRUE(keyDeletedDuringSend);
    EXPECT_FALSE(wifi.hasCredentials());
    EXPECT_FALSE(wifi.hasPassword());

    // Subsequent tick() executes deferred restart rather than emitting AP_STARTED
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_AP_STARTED, &EventCounter::onApStarted, &counter);
    wifi.tick(3000);
    EXPECT_EQ(0, counter.apStarted);
}

TEST_F(WifiProvisioningTest, UpdateWiFiPreservesDeferredReconnectWithoutMidSaveApTeardown) {
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_CONNECTED, &EventCounter::onConnected, &counter);

    wifi.begin();
    wifi.tick(2000);
    wifi.tick(2001);
    ASSERT_TRUE(wifi.isSetupMode());

    bool encryptHookRan = false;
    bool apAliveMidSave = false;
    crypto.onEncrypt = [&]() {
        encryptHookRan = true;
        wifi.tick(2500);
        wifi.tick(2501);
        apAliveMidSave = wifi.getAPMode();
    };

    wifi.updateWiFi("HomeNet", "secret123");
    EXPECT_TRUE(encryptHookRan);
    EXPECT_TRUE(apAliveMidSave);
    EXPECT_STREQ("HomeNet", wifi.getSSID());
    EXPECT_FALSE(wifi.isSetupMode());
    EXPECT_TRUE(wifi.getAPMode());

    wifi.tick(3000); // deferred reconnect now executes cleanly
    EXPECT_FALSE(wifi.getAPMode());
    wifi.tick(5000); // Init -> STA attempt
    wifi.tick(5001); // StaConnecting -> connected (stub)
    EXPECT_TRUE(wifi.isReady());
    EXPECT_EQ(1, counter.connected);
}

TEST_F(WifiProvisioningTest, StaConnectingGuardSkipsConnectAndInactivityDuringProvisioning) {
    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_CONNECTED, &EventCounter::onConnected, &counter);

    wifi.updateWiFi("OldNet", "oldpass123");
    wifi.tick(1000); // consume _reconnectRequested -> Init
    wifi.begin();
    wifi.tick(2000); // Init -> StaConnecting

    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);

    int connectedDuringEncrypt = -1;
    bool ssidOldDuringEncrypt = false;
    crypto.onEncrypt = [&]() {
        // WiFi.status() == WL_CONNECTED in native stub, but StaConnecting must
        // skip onStaConnected while provisioning is in flight.
        wifi.tick(2500);
        connectedDuringEncrypt = counter.connected;
        ssidOldDuringEncrypt = (std::string(wifi.getSSID()) == "OldNet");
    };

    int connectedDuringSend = -1;
    MockWifiHttpRequest req("{\"ssid\":\"NewNet\",\"pass\":\"newpass123\"}");
    MockWifiHttpResponse resp;
    resp.onSend = [&]() {
        wifi.tick(2600);
        connectedDuringSend = counter.connected;
    };

    EXPECT_EQ(ESP_OK, connectHandler(&req, &resp));
    EXPECT_EQ(200, resp.lastCode);
    EXPECT_EQ(0, connectedDuringEncrypt);
    EXPECT_TRUE(ssidOldDuringEncrypt);
    EXPECT_EQ(0, connectedDuringSend);
    EXPECT_STREQ("NewNet", wifi.getSSID());

    // Next tick() executes deferred restart, not onStaConnected
    wifi.tick(2700);
    EXPECT_EQ(0, counter.connected);
}

TEST_F(WifiProvisioningTest, FallbackApInactivityGuardSkippedWhileProvisioning) {
    wifi.begin();
    wifi.tick(2000);
    wifi.tick(2001); // Setup AP ready at t=2001 (_lastApiRequestTime = 2001)
    ASSERT_TRUE(wifi.getAPMode());

    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);

    bool apAlivePastTimeout = false;
    crypto.onEncrypt = [&]() {
        // Advance beyond 1h inactivity timeout while provisioning is in progress
        wifi.tick(4000000);
        wifi.tick(4000001);
        apAlivePastTimeout = wifi.getAPMode();
    };

    MockWifiHttpRequest req("{\"ssid\":\"HomeNet\",\"pass\":\"secret123\"}");
    MockWifiHttpResponse resp;
    EXPECT_EQ(ESP_OK, connectHandler(&req, &resp));
    EXPECT_TRUE(apAlivePastTimeout);
}

TEST_F(WifiProvisioningTest, McpWifiForgetErasesCredentialsAndDefersRestart) {
    MCPToolRegistry registry;
    wifi.registerMCPTools(registry);
    const MCPToolEntry* forgetTool = registry.findTool("wifi_forget");
    ASSERT_NE(forgetTool, nullptr);

    wifi.updateWiFi("HomeNet", "secret123");
    wifi.tick(2000); // consume _reconnectRequested

    EventCounter counter;
    bus.subscribe(WifiEvents::WIFI_AP_STARTED, &EventCounter::onApStarted, &counter);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resDoc;
    JsonObject res = resDoc.to<JsonObject>();
    forgetTool->handler(forgetTool->context, args, res);

    EXPECT_TRUE(res["success"].as<bool>());
    EXPECT_TRUE(res["isError"].isNull());
    EXPECT_FALSE(wifi.hasCredentials());
    EXPECT_FALSE(wifi.hasPassword());
    EXPECT_TRUE(keyManager.getKeyById("wifi_password").empty());

    // Subsequent tick runs deferred restart rather than ApStarting
    wifi.tick(3000);
    EXPECT_EQ(0, counter.apStarted);
}

TEST_F(WifiProvisioningTest, PersistenceFailureFailsClosedWithoutRamCommitOrReboot) {
    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    auto forgetHandler = findRoute("/api/wifi/forget", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);
    ASSERT_NE(forgetHandler, nullptr);

    // 1. KeyManager encryption failure during handleConnect
    crypto.failEncrypt = true;
    {
        MockWifiHttpRequest req("{\"ssid\":\"HomeNet\",\"pass\":\"secret123\"}");
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req, &resp));
        EXPECT_EQ(500, resp.lastCode);
    }
    crypto.failEncrypt = false;
    EXPECT_FALSE(wifi.hasCredentials());
    EXPECT_FALSE(wifi.hasPassword());
    {
        JsonDocument secDoc;
        JsonObject sec = secDoc.to<JsonObject>();
        wifi.serializeConfig(sec);
        EXPECT_STREQ("", sec["ssid"].as<const char*>());
    }

    // 2. ConfigManager save failure during handleConnect (replace wifi.json with directory)
    {
        std::error_code ec;
        std::filesystem::remove(ConfigRoot::getModFilePath("wifi").c_str(), ec);
        std::filesystem::create_directory(ConfigRoot::getModFilePath("wifi").c_str(), ec);
        MockWifiHttpRequest req("{\"ssid\":\"HomeNet\",\"pass\":\"secret123\"}");
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req, &resp));
        EXPECT_EQ(500, resp.lastCode);
        EXPECT_FALSE(wifi.hasCredentials());
        std::filesystem::remove_all(ConfigRoot::getModFilePath("wifi").c_str(), ec);
    }

    // 3. Seed valid credentials, then fail ConfigManager during handleForget & MCP wifi_forget
    wifi.updateWiFi("StableNet", "stablepass");
    wifi.tick(2000);
    ASSERT_TRUE(wifi.hasCredentials());

    {
        std::error_code ec;
        std::filesystem::remove(ConfigRoot::getModFilePath("wifi").c_str(), ec);
        std::filesystem::create_directory(ConfigRoot::getModFilePath("wifi").c_str(), ec);

        MockWifiHttpRequest req("{}");
        MockWifiHttpResponse resp;
        EXPECT_EQ(ESP_FAIL, forgetHandler(&req, &resp));
        EXPECT_EQ(500, resp.lastCode);
        EXPECT_STREQ("StableNet", wifi.getSSID());
        EXPECT_TRUE(wifi.hasCredentials());

        MCPToolRegistry registry;
        wifi.registerMCPTools(registry);
        const MCPToolEntry* forgetTool = registry.findTool("wifi_forget");
        ASSERT_NE(forgetTool, nullptr);
        JsonDocument argsDoc, resDoc;
        JsonObject args = argsDoc.to<JsonObject>();
        JsonObject res = resDoc.to<JsonObject>();
        forgetTool->handler(forgetTool->context, args, res);
        EXPECT_FALSE(res["success"].as<bool>());
        EXPECT_TRUE(res["isError"].as<bool>());
        EXPECT_STREQ("StableNet", wifi.getSSID());

        std::filesystem::remove_all(ConfigRoot::getModFilePath("wifi").c_str(), ec);
    }
}

TEST_F(WifiProvisioningTest, PostForgetSucceedsForOpenNetworkWithoutPasswordKey) {
    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    auto forgetHandler = findRoute("/api/wifi/forget", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);
    ASSERT_NE(forgetHandler, nullptr);

    MockWifiHttpRequest connReq("{\"ssid\":\"OpenCafe\",\"pass\":\"\"}");
    MockWifiHttpResponse connResp;
    EXPECT_EQ(ESP_OK, connectHandler(&connReq, &connResp));
    EXPECT_EQ(200, connResp.lastCode);
    wifi.tick(2500); // clear _restartRequested

    MockWifiHttpRequest forgetReq("{}");
    MockWifiHttpResponse forgetResp;
    EXPECT_EQ(ESP_OK, forgetHandler(&forgetReq, &forgetResp));
    EXPECT_EQ(200, forgetResp.lastCode);
    EXPECT_FALSE(wifi.hasCredentials());
}

TEST_F(WifiProvisioningTest, ConcurrentRequestsRejectedWhileRestartPendingAndOversizedPasswordBounded) {
    auto connectHandler = findRoute("/api/wifi/connect", HTTP_POST);
    auto forgetHandler = findRoute("/api/wifi/forget", HTTP_POST);
    ASSERT_NE(connectHandler, nullptr);
    ASSERT_NE(forgetHandler, nullptr);

    // 70-char password exceeds char _wifiPass[65] (64 chars max); KeyManager must store
    // the exact 64-char truncated value matching _wifiPass.
    std::string longPass(70, 'A');
    std::string expectedPass(64, 'A');
    MockWifiHttpRequest req(std::string("{\"ssid\":\"HomeNet\",\"pass\":\"") + longPass + "\"}");
    MockWifiHttpResponse resp;
    EXPECT_EQ(ESP_OK, connectHandler(&req, &resp));
    EXPECT_EQ(200, resp.lastCode);
    EXPECT_EQ(expectedPass, keyManager.getKeyById("wifi_password"));

    // While _restartRequested is still pending (before tick() runs), follow-up
    // connect/forget/MCP/updateWiFi calls are rejected cleanly.
    {
        MockWifiHttpRequest req2("{\"ssid\":\"OtherNet\",\"pass\":\"pass1234\"}");
        MockWifiHttpResponse resp2;
        EXPECT_EQ(ESP_FAIL, connectHandler(&req2, &resp2));
        EXPECT_EQ(503, resp2.lastCode);
    }
    {
        MockWifiHttpRequest fReq("{}");
        MockWifiHttpResponse fResp;
        EXPECT_EQ(ESP_FAIL, forgetHandler(&fReq, &fResp));
        EXPECT_EQ(503, fResp.lastCode);
    }
    {
        MCPToolRegistry registry;
        wifi.registerMCPTools(registry);
        const MCPToolEntry* forgetTool = registry.findTool("wifi_forget");
        ASSERT_NE(forgetTool, nullptr);
        JsonDocument argsDoc, resDoc;
        JsonObject args = argsDoc.to<JsonObject>();
        JsonObject res = resDoc.to<JsonObject>();
        forgetTool->handler(forgetTool->context, args, res);
        EXPECT_TRUE(res["isError"].as<bool>());
    }
    wifi.updateWiFi("IgnoredNet", "ignored123");
    EXPECT_STREQ("HomeNet", wifi.getSSID());
}

TEST_F(WifiProvisioningTest, CopyAccessorsAndTestConnectionNullGuard) {
    wifi.setDefaultHostname("my-sensor");
    wifi.updateWiFi("MyLongNetworkName", "secret123");
    wifi.tick(2000);

    char buf[33] = {};
    wifi.copySSID(buf, sizeof(buf));
    EXPECT_STREQ("MyLongNetworkName", buf);
    wifi.copyHostname(buf, sizeof(buf));
    EXPECT_STREQ("my-sensor", buf);
    wifi.copyDefaultHostname(buf, sizeof(buf));
    EXPECT_STREQ("my-sensor", buf);
    wifi.copyAPSSID(buf, sizeof(buf));
    EXPECT_STREQ("my-sensor", buf);

    char tiny[5] = {};
    wifi.copySSID(tiny, sizeof(tiny));
    EXPECT_STREQ("MyLo", tiny);

    // Null/zero-length buffer guards must not crash
    wifi.copySSID(nullptr, 10);
    wifi.copyHostname(buf, 0);
    wifi.copyDefaultHostname(nullptr, 0);
    wifi.copyAPSSID(buf, 0);

    std::string ipOut;
    EXPECT_FALSE(wifi.testConnection(nullptr, "pass", ipOut));
    EXPECT_FALSE(wifi.testConnection("", "pass", ipOut));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

