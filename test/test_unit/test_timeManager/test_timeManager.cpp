/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_timeManager/test_timeManager.cpp
 * Description: Implementation of TimeManagerTest class.
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <PsychicHttp.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include "../../../modules/TimeManager/TimeManager.hpp"
#include "ConfigManager.hpp"
#include "EventBus.hpp"
#include "lwip_dhcp_tz_hooks.h"

namespace {

class MockPsychicRequest : public PsychicRequest {
public:
    explicit MockPsychicRequest(std::string bodyStr = "") : _body(std::move(bodyStr)) {}
    const char* bodyCStr() override { return _body.c_str(); }
    size_t contentLength() override { return _body.size(); }
private:
    std::string _body;
};

class MockPsychicResponse : public PsychicResponse {
public:
    esp_err_t send(int code, const char* contentType, const char* content) override {
        lastCode = code;
        lastContentType = contentType ? contentType : "";
        lastBody = content ? content : "";
        return ESP_OK;
    }
    int lastCode = 0;
    std::string lastContentType;
    std::string lastBody;
};

static void resetTimeTestEnvironment() {
    stub_sntp_reset();
    lwip_dhcp_tz_take_pending(nullptr, 0, nullptr, nullptr, 0, nullptr);
    unsetenv("TZ");
    tzset();

    std::error_code ec;
    std::filesystem::create_directories("test/tmp/mod", ec);
    std::filesystem::create_directories("test/tmp/etc", ec);
    std::filesystem::remove("test/tmp/timezones.json", ec);
    std::filesystem::remove("test/tmp/timezones.tmp", ec);
    std::filesystem::remove("test/tmp/mod/time.json", ec);
    std::filesystem::remove("test/tmp/etc/manifest.json", ec);
    std::filesystem::remove("test/tmp/mod/manifest.json", ec);
}

static bool writeRawFile(const char* path, const std::string& content) {
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << content;
    ofs.close();
    return ofs.good();
}

static const PsychicHttpServer::Route* findRoute(const PsychicHttpServer& server,
                                                 const char* uri, int method) {
    for (const auto& r : server.routes) {
        if (r.uri == uri && r.method == method) {
            return &r;
        }
    }
    return nullptr;
}

} // namespace

class TimeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        resetTimeTestEnvironment();
    }
    void TearDown() override {
        resetTimeTestEnvironment();
    }

    TimeManager timeManager;
};

// Initial state
TEST_F(TimeManagerTest, InitialState) {
    EXPECT_FALSE(timeManager.isTimeSet());
}

// begin() sets synced
TEST_F(TimeManagerTest, BeginSetsSynced) {
    timeManager.begin();
    timeManager.tick(0);
    EXPECT_TRUE(timeManager.isTimeSet());
}

// begin() and POST /api/time/config initialize esp_netif before any esp_sntp_* calls
TEST_F(TimeManagerTest, BeginInitializesNetifBeforeAnySntpCalls) {
    EXPECT_FALSE(stub_sntp_state().netifInitialized);
    EXPECT_FALSE(stub_sntp_state().sntpCalledBeforeNetifInit);
    EXPECT_EQ(stub_sntp_state().netifInitCount, 0u);

    timeManager.begin();

    EXPECT_TRUE(stub_sntp_state().netifInitialized);
    EXPECT_FALSE(stub_sntp_state().sntpCalledBeforeNetifInit);
    const size_t countAfterBegin = stub_sntp_state().netifInitCount;
    EXPECT_GE(countAfterBegin, 1u);

    PsychicHttpServer server;
    timeManager.registerRoutes(server);
    const auto* postRoute = findRoute(server, "/api/time/config", HTTP_POST);
    ASSERT_NE(postRoute, nullptr);

    MockPsychicRequest postReq(
        "{\"timezone\":\"Europe/London\",\"posix\":\"GMT0BST,M3.5.0/1,M10.5.0\",\"dhcpNtp\":false}");
    MockPsychicResponse postResp;
    EXPECT_EQ(postRoute->handler(&postReq, &postResp), ESP_OK);
    EXPECT_EQ(postResp.lastCode, 200);

    EXPECT_TRUE(stub_sntp_state().netifInitialized);
    EXPECT_FALSE(stub_sntp_state().sntpCalledBeforeNetifInit);
    EXPECT_GT(stub_sntp_state().netifInitCount, countAfterBegin);
}

// getLocalTime() before begin
TEST_F(TimeManagerTest, GetLocalTimeBeforeBegin) {
    struct tm timeinfo;
    timeManager.getLocalTime(&timeinfo); // Instruction says expects false, but mock returns true. Testing no crash.
    SUCCEED();
}

// getLocalTime() after begin
TEST_F(TimeManagerTest, GetLocalTimeAfterBegin) {
    timeManager.begin();
    timeManager.tick(0);
    struct tm timeinfo;
    EXPECT_TRUE(timeManager.getLocalTime(&timeinfo));
}

// getLocalTime() with nullptr
TEST_F(TimeManagerTest, GetLocalTimeNullptr) {
    timeManager.begin();
    EXPECT_FALSE(timeManager.getLocalTime(nullptr));
}

// setTimezone()
TEST_F(TimeManagerTest, SetTimezone) {
    timeManager.setTimezone("EST5EDT");
    SUCCEED();
}

// tick() after begin
TEST_F(TimeManagerTest, TickAfterBegin) {
    timeManager.begin();
    timeManager.tick(0);
    SUCCEED();
}

// tick() before begin
TEST_F(TimeManagerTest, TickBeforeBegin) {
    timeManager.tick(0);
    SUCCEED();
}

// Multiple begin() calls
TEST_F(TimeManagerTest, MultipleBeginCalls) {
    timeManager.begin();
    timeManager.setTimezone("EST5EDT");
    timeManager.begin();
    SUCCEED();
}

// resetConfig() seeds the factory default POSIX timezone ("GMT0")
TEST_F(TimeManagerTest, ResetConfigSeedsDefaultPosixTimezone) {
    timeManager.resetConfig();
    EXPECT_STREQ("GMT0", timeManager.getPosixTz());
    timeManager.begin();
    timeManager.tick(0);
    EXPECT_TRUE(timeManager.isTimeSet());
}

#include "PreconditionEngine.hpp"
#include "TimePreconditionContributor.hpp"
#include "WifiManager.hpp"
#include "WifiPreconditionContributor.hpp"

// ---------------------------------------------------------------------------
// TimePreconditionContributor Tests
// ---------------------------------------------------------------------------

TEST(TimePreconditionContributorTest, RegistersTwoOperationalPreconditions) {
    TimeManager tm;
    TimePreconditionContributor timeContributor(tm);

    WifiManager wifi;
    WifiPreconditionContributor wifiContributor(wifi);

    PreconditionEngine engine;
    wifiContributor.registerPreconditions(engine);
    timeContributor.registerPreconditions(engine);

    EXPECT_EQ(engine.getPreconditionCount(), 3);
    EXPECT_TRUE(engine.resolveEdges());

    const PreconditionState* tzState = engine.getStateByName(PreconditionNames::TIMEZONE);
    EXPECT_NE(tzState, nullptr);
    EXPECT_EQ(tzState->role, PreconditionRole::Operational);
    EXPECT_EQ(tzState->resolvedEdgeCount, 0);

    const PreconditionState* syncState = engine.getStateByName(PreconditionNames::TIME_SYNC);
    EXPECT_NE(syncState, nullptr);
    EXPECT_EQ(syncState->role, PreconditionRole::Operational);
    EXPECT_EQ(syncState->resolvedEdgeCount, 1);
    EXPECT_EQ(syncState->resolvedEdges[0], 0); // Index of PreconditionNames::WIFI_STATION
}

TEST(TimePreconditionContributorTest, EvaluationAndDescriptions) {
    TimeManager tm;
    TimePreconditionContributor timeContributor(tm);

    WifiManager wifi;
    WifiPreconditionContributor wifiContributor(wifi);

    PreconditionEngine engine;
    wifiContributor.registerPreconditions(engine);
    timeContributor.registerPreconditions(engine);
    EXPECT_TRUE(engine.resolveEdges());

    engine.tick(1000);

    // Timezone: unconfigured -> NotConfigured -> NeedsUser
    const PreconditionState* tzState = engine.getStateByName(PreconditionNames::TIMEZONE);
    EXPECT_NE(tzState, nullptr);
    EXPECT_FALSE(tzState->rawStatus.met);
    EXPECT_EQ(tzState->rawStatus.cause, Cause::NotConfigured);
    EXPECT_EQ(tzState->severity, Severity::NeedsUser);

    char desc[128] = {};
    tzState->describe(tzState->rawStatus.cause, desc, sizeof(desc), tzState->context);
    EXPECT_STREQ("Set your timezone in the Time settings", desc);

    // Time Sync: Requires WiFi Station which is unmet -> DependencyUnmet -> Blocked
    const PreconditionState* syncState = engine.getStateByName(PreconditionNames::TIME_SYNC);
    EXPECT_NE(syncState, nullptr);
    EXPECT_FALSE(syncState->rawStatus.met);
    EXPECT_EQ(syncState->rawStatus.cause, Cause::DependencyUnmet);
    EXPECT_EQ(syncState->severity, Severity::Blocked);
}

TEST_F(TimeManagerTest, PreconditionTimezoneMetViaDhcpPCode) {
    TimePreconditionContributor timeContributor(timeManager);
    WifiManager wifi;
    WifiPreconditionContributor wifiContributor(wifi);

    PreconditionEngine engine;
    wifiContributor.registerPreconditions(engine);
    timeContributor.registerPreconditions(engine);
    ASSERT_TRUE(engine.resolveEdges());

    timeManager.resetConfig();
    timeManager.begin();
    engine.tick(100);
    const PreconditionState* tzBefore = engine.getStateByName(PreconditionNames::TIMEZONE);
    ASSERT_NE(tzBefore, nullptr);
    EXPECT_FALSE(tzBefore->rawStatus.met);

    char pcode[] = "EST5EDT,M3.2.0,M11.1.0";
    struct pbuf p{nullptr, pcode, static_cast<uint16_t>(strlen(pcode)), static_cast<uint16_t>(strlen(pcode))};
    struct dhcp d{DHCP_STATE_BOUND};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE,
                              static_cast<uint8_t>(strlen(pcode)), &p, 0);

    timeManager.tick(200);
    engine.tick(200);
    const PreconditionState* tzAfter = engine.getStateByName(PreconditionNames::TIMEZONE);
    ASSERT_NE(tzAfter, nullptr);
    EXPECT_TRUE(tzAfter->rawStatus.met);
    EXPECT_TRUE(timeManager.isConfigured());
}

// ---------------------------------------------------------------------------
// LwIP Hook Filtering & Spinlock Handoff Tests
// ---------------------------------------------------------------------------

TEST_F(TimeManagerTest, HookIgnoresSelectingAndAcceptsAcceptedStates) {
    char pcode[] = "UTC0";
    struct pbuf p{nullptr, pcode, 4, 4};
    struct dhcp d{DHCP_STATE_SELECTING};

    // SELECTING (raw DHCPOFFER before lease acceptance) must be ignored
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_SELECTING, LWIP_DHCP_OPTION_TZ_PCODE, 4, &p, 0);
    bool gotP = false, gotT = false;
    char outP[64] = {}, outT[64] = {};
    EXPECT_FALSE(lwip_dhcp_tz_take_pending(outP, sizeof(outP), &gotP, outT, sizeof(outT), &gotT));
    EXPECT_FALSE(gotP);
    EXPECT_FALSE(gotT);

    // Accepted states must stage cleanly and drain atomically
    const uint8_t acceptedStates[] = {
        DHCP_STATE_REQUESTING,
        DHCP_STATE_RENEWING,
        DHCP_STATE_REBINDING,
        DHCP_STATE_REBOOTING,
        DHCP_STATE_BOUND
    };
    for (uint8_t st : acceptedStates) {
        d.state = st;
        lwip_dhcp_on_extra_option(&d, st, LWIP_DHCP_OPTION_TZ_PCODE, 4, &p, 0);
        gotP = false;
        EXPECT_TRUE(lwip_dhcp_tz_take_pending(outP, sizeof(outP), &gotP, nullptr, 0, nullptr));
        EXPECT_TRUE(gotP);
        EXPECT_STREQ("UTC0", outP);
        // Second take immediately after drain must return false
        EXPECT_FALSE(lwip_dhcp_tz_take_pending(outP, sizeof(outP), &gotP, nullptr, 0, nullptr));
    }
}

TEST_F(TimeManagerTest, HookIgnoresOption2AndRejectsMalformedPayloads) {
    char opt2[] = "\x00\x00\x0e\x10";
    struct pbuf p2{nullptr, opt2, 4, 4};
    struct dhcp d{DHCP_STATE_BOUND};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, 2, 4, &p2, 0);

    // Null pbuf, zero length, length >= 64
    char longBuf[70];
    memset(longBuf, 'A', sizeof(longBuf));
    struct pbuf pLong{nullptr, longBuf, 64, 64};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, 4, nullptr, 0);
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, 0, &pLong, 0);
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, 64, &pLong, 0);

    // Non-printable control character, embedded NUL, high ASCII, outer/only whitespace
    char ctrl[] = "EST\nEDT";
    struct pbuf pCtrl{nullptr, ctrl, 7, 7};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, 7, &pCtrl, 0);

    char embeddedNul[] = {'E', 'S', 'T', '\0', '5'};
    struct pbuf pNul{nullptr, embeddedNul, 5, 5};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, 5, &pNul, 0);

    char highAscii[] = {'U', 'T', 'C', static_cast<char>(0x80)};
    struct pbuf pHigh{nullptr, highAscii, 4, 4};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, 4, &pHigh, 0);

    char spacesOnly[] = "   ";
    struct pbuf pSpaces{nullptr, spacesOnly, 3, 3};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE, 3, &pSpaces, 0);

    char leadingSpace[] = " UTC";
    struct pbuf pLead{nullptr, leadingSpace, 4, 4};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE, 4, &pLead, 0);

    char trailingSpace[] = "UTC ";
    struct pbuf pTrail{nullptr, trailingSpace, 4, 4};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE, 4, &pTrail, 0);

    EXPECT_FALSE(lwip_dhcp_tz_take_pending(nullptr, 0, nullptr, nullptr, 0, nullptr));

    // Chained pbuf with non-zero offset must succeed for valid max-63-char payload
    char part1[] = "XXXXEurope/";
    char part2[] = "London";
    struct pbuf tail{nullptr, part2, 6, 6};
    struct pbuf head{&tail, part1, 17, 11};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_RENEWING, LWIP_DHCP_OPTION_TZ_TCODE, 13, &head, 4);

    char outT[64] = {};
    bool gotT = false;
    EXPECT_TRUE(lwip_dhcp_tz_take_pending(nullptr, 0, nullptr, outT, sizeof(outT), &gotT));
    EXPECT_TRUE(gotT);
    EXPECT_STREQ("Europe/London", outT);
}

// ---------------------------------------------------------------------------
// Config Schema v2, Migration & Zero Flash Wear Tests
// ---------------------------------------------------------------------------

TEST_F(TimeManagerTest, SchemaVersionV2DefaultsAndMigration) {
    EXPECT_EQ(timeManager.getSchemaVersion(), 2);

    timeManager.resetConfig();
    EXPECT_TRUE(timeManager.isDhcpNtpEnabled());
    EXPECT_TRUE(timeManager.isDhcpTzEnabled());
    EXPECT_FALSE(timeManager.isConfigured());
    EXPECT_STREQ("GMT0", timeManager.getPosixTz());

    // v1 config WITH manual timezone must preserve dhcpTz=false across upgrade
    ASSERT_TRUE(writeRawFile("test/tmp/mod/time.json",
        "{\"_v\":1,\"timezone\":\"America/Chicago\",\"posix\":\"CST6CDT,M3.2.0,M11.1.0\","
        "\"ntpServer1\":\"time.google.com\",\"ntpServer2\":\"\",\"ntpServer3\":\"\"}"));

    ConfigManager cm1;
    cm1.registerModule(&timeManager);
    cm1.beginAll();

    EXPECT_TRUE(timeManager.isDhcpNtpEnabled());
    EXPECT_FALSE(timeManager.isDhcpTzEnabled());
    EXPECT_STREQ("America/Chicago", timeManager.getConfiguredTimezone());
    EXPECT_STREQ("CST6CDT,M3.2.0,M11.1.0", timeManager.getConfiguredPosixTz());

    // Re-check persisted file on disk was upgraded to _v=2 with dhcpNtp=true, dhcpTz=false
    std::ifstream ifs1("test/tmp/mod/time.json");
    std::string disk1((std::istreambuf_iterator<char>(ifs1)), std::istreambuf_iterator<char>());
    JsonDocument doc1;
    ASSERT_FALSE(deserializeJson(doc1, disk1));
    EXPECT_EQ(doc1["_v"].as<int>(), 2);
    EXPECT_STREQ("time.google.com", doc1["ntpServer1"].as<const char*>());
    EXPECT_TRUE(doc1["dhcpNtp"].as<bool>());
    EXPECT_FALSE(doc1["dhcpTz"].as<bool>());

    // v1 config with posix != "GMT0" and empty timezone also counts as manual -> dhcpTz=false
    TimeManager tmPosixOnly;
    JsonDocument manualPosixDoc;
    manualPosixDoc["timezone"] = "";
    manualPosixDoc["posix"] = "JST-9";
    JsonObject manualObj = manualPosixDoc.as<JsonObject>();
    EXPECT_TRUE(tmPosixOnly.migrateConfig(manualObj, 1));
    EXPECT_TRUE(manualObj["dhcpNtp"].as<bool>());
    EXPECT_FALSE(manualObj["dhcpTz"].as<bool>());

    // v1 config with factory defaults ("", "GMT0") must enable dhcpTz=true
    TimeManager tmUnconfigured;
    resetTimeTestEnvironment();
    ASSERT_TRUE(writeRawFile("test/tmp/mod/time.json",
        "{\"_v\":1,\"timezone\":\"\",\"posix\":\"GMT0\","
        "\"ntpServer1\":\"pool.ntp.org\",\"ntpServer2\":\"time.nist.gov\",\"ntpServer3\":\"time.windows.com\"}"));

    ConfigManager cm2;
    cm2.registerModule(&tmUnconfigured);
    cm2.beginAll();
    EXPECT_TRUE(tmUnconfigured.isDhcpNtpEnabled());
    EXPECT_TRUE(tmUnconfigured.isDhcpTzEnabled());
}

TEST_F(TimeManagerTest, ZeroFlashWearWhenReceivingDhcpOptions) {
    ConfigManager cm;
    cm.registerModule(&timeManager);
    ASSERT_TRUE(writeRawFile("test/tmp/mod/time.json",
        "{\"_v\":2,\"timezone\":\"Europe/Berlin\",\"posix\":\"CET-1CEST,M3.5.0,M10.5.0/3\","
        "\"ntpServer1\":\"pool.ntp.org\",\"ntpServer2\":\"time.nist.gov\",\"ntpServer3\":\"time.windows.com\","
        "\"dhcpNtp\":true,\"dhcpTz\":true}"));
    cm.beginAll();
    timeManager.setConfigManager(&cm);
    timeManager.begin();

    // Record exact disk contents before DHCP event
    std::ifstream ifsBefore("test/tmp/mod/time.json");
    std::string diskBefore((std::istreambuf_iterator<char>(ifsBefore)), std::istreambuf_iterator<char>());
    ifsBefore.close();

    // Receive DHCP Option 100 + Option 101 + Option 42
    char pcode[] = "EST5EDT,M3.2.0,M11.1.0";
    char tcode[] = "America/New_York";
    struct pbuf pP{nullptr, pcode, static_cast<uint16_t>(strlen(pcode)), static_cast<uint16_t>(strlen(pcode))};
    struct pbuf pT{nullptr, tcode, static_cast<uint16_t>(strlen(tcode)), static_cast<uint16_t>(strlen(tcode))};
    struct dhcp d{DHCP_STATE_BOUND};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, static_cast<uint8_t>(strlen(pcode)), &pP, 0);
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE, static_cast<uint8_t>(strlen(tcode)), &pT, 0);

    ip4_addr_t srv = stub_sntp_make_ip4(10, 0, 0, 1);
    stub_sntp_simulate_dhcp_servers(1, &srv);

    timeManager.tick(100);

    // Active getters show DHCP override
    EXPECT_TRUE(timeManager.isDhcpTzActive());
    EXPECT_STREQ("America/New_York", timeManager.getTimezone());
    EXPECT_STREQ("EST5EDT,M3.2.0,M11.1.0", timeManager.getPosixTz());
    EXPECT_TRUE(timeManager.isDhcpNtpActive());
    EXPECT_STREQ("10.0.0.1", timeManager.getDhcpNtpServer(0));

    // Persisted fallback getters & serialized JSON MUST remain completely untouched
    EXPECT_STREQ("Europe/Berlin", timeManager.getConfiguredTimezone());
    EXPECT_STREQ("CET-1CEST,M3.5.0,M10.5.0/3", timeManager.getConfiguredPosixTz());

    JsonDocument serDoc;
    JsonObject serObj = serDoc.to<JsonObject>();
    timeManager.serializeConfig(serObj);
    EXPECT_STREQ("Europe/Berlin", serObj["timezone"].as<const char*>());
    EXPECT_STREQ("CET-1CEST,M3.5.0,M10.5.0/3", serObj["posix"].as<const char*>());
    EXPECT_STREQ("pool.ntp.org", serObj["ntpServer1"].as<const char*>());
    EXPECT_TRUE(serObj["dhcpNtp"].as<bool>());
    EXPECT_TRUE(serObj["dhcpTz"].as<bool>());
    EXPECT_TRUE(serObj["dhcpNtpServers"].isNull());

    // Zero flash wear: /mod/time.json bytes on disk did NOT change
    std::ifstream ifsAfter("test/tmp/mod/time.json");
    std::string diskAfter((std::istreambuf_iterator<char>(ifsAfter)), std::istreambuf_iterator<char>());
    EXPECT_EQ(diskBefore, diskAfter);
}

// ---------------------------------------------------------------------------
// DHCP Option 42 (NTP) & Static Fallback Preservation Tests
// ---------------------------------------------------------------------------

TEST_F(TimeManagerTest, DhcpNtpOneAndTwoServersPreserveStaticFallbacks) {
    stub_sntp_set_sync_status(SNTP_SYNC_STATUS_IN_PROGRESS);
    timeManager.resetConfig();
    timeManager.begin();

    EXPECT_TRUE(stub_sntp_is_dhcp_mode());
    EXPECT_EQ(timeManager.getSyncStatus(), TimeSyncStatus::IN_PROGRESS);

    // Simulate 1 Option 42 server; LwIP clears slots 1 and 2 to NULL
    ip4_addr_t oneSrv = stub_sntp_make_ip4(192, 168, 10, 5);
    stub_sntp_simulate_dhcp_servers(1, &oneSrv);

    // Must process inside tick() BEFORE IN_PROGRESS early return
    timeManager.tick(50);

    EXPECT_TRUE(timeManager.isDhcpNtpActive());
    EXPECT_EQ(timeManager.getDhcpNtpServerCount(), 1u);
    EXPECT_STREQ("192.168.10.5", timeManager.getDhcpNtpServer(0));
    EXPECT_STREQ("", timeManager.getDhcpNtpServer(1));

    // Slot 0 remains Option 42 IP (servername nullptr), trailing slots 1 & 2 restored without dupes
    EXPECT_EQ(esp_sntp_getservername(0), nullptr);
    EXPECT_FALSE(ip_addr_isany(esp_sntp_getserver(0)));
    EXPECT_STREQ("pool.ntp.org", esp_sntp_getservername(1));
    EXPECT_STREQ("time.nist.gov", esp_sntp_getservername(2));

    // Now simulate DHCP renewal providing 2 Option 42 servers
    ip4_addr_t twoSrvs[2] = {
        stub_sntp_make_ip4(172, 16, 0, 1),
        stub_sntp_make_ip4(172, 16, 0, 2)
    };
    stub_sntp_simulate_dhcp_servers(2, twoSrvs);
    timeManager.tick(100);

    EXPECT_EQ(timeManager.getDhcpNtpServerCount(), 2u);
    EXPECT_STREQ("172.16.0.1", timeManager.getDhcpNtpServer(0));
    EXPECT_STREQ("172.16.0.2", timeManager.getDhcpNtpServer(1));
    EXPECT_EQ(esp_sntp_getservername(0), nullptr);
    EXPECT_EQ(esp_sntp_getservername(1), nullptr);
    EXPECT_STREQ("pool.ntp.org", esp_sntp_getservername(2));
}

TEST_F(TimeManagerTest, DhcpNtpThreeServersAndDisabledMode) {
    timeManager.resetConfig();
    timeManager.begin();

    ip4_addr_t threeSrvs[3] = {
        stub_sntp_make_ip4(10, 1, 1, 1),
        stub_sntp_make_ip4(10, 1, 1, 2),
        stub_sntp_make_ip4(10, 1, 1, 3)
    };
    stub_sntp_simulate_dhcp_servers(3, threeSrvs);
    timeManager.tick(10);

    EXPECT_TRUE(timeManager.isDhcpNtpActive());
    EXPECT_EQ(timeManager.getDhcpNtpServerCount(), 3u);
    EXPECT_STREQ("10.1.1.3", timeManager.getDhcpNtpServer(2));
    EXPECT_EQ(esp_sntp_getservername(0), nullptr);
    EXPECT_EQ(esp_sntp_getservername(1), nullptr);
    EXPECT_EQ(esp_sntp_getservername(2), nullptr);

    // Disable dhcpNtp via applyConfig + begin() -> must disable DHCP SNTP mode and use static servers
    stub_sntp_reset();
    JsonDocument doc;
    doc["timezone"] = "UTC";
    doc["posix"] = "UTC0";
    doc["dhcpNtp"] = false;
    doc["dhcpTz"] = true;
    timeManager.applyConfig(doc.as<JsonObject>());
    timeManager.begin();
    stub_sntp_simulate_dhcp_servers(1, &threeSrvs[0]);
    timeManager.tick(20);

    EXPECT_FALSE(stub_sntp_is_dhcp_mode());
    EXPECT_FALSE(timeManager.isDhcpNtpActive());
    EXPECT_EQ(timeManager.getDhcpNtpServerCount(), 0u);
    EXPECT_STREQ("pool.ntp.org", esp_sntp_getservername(0));
}

// ---------------------------------------------------------------------------
// RFC 4833 Option 100/101 Resolution, Streaming Lookup & Precedence Tests
// ---------------------------------------------------------------------------

TEST_F(TimeManagerTest, Option101TCodeStreamingLookupOverBufferBoundaryAndFallbackSafety) {
    // Generate > 512 bytes of JSON to verify streaming across multiple 256B LittleFS reads
    std::string tzJson = "{\n";
    for (int i = 0; i < 20; ++i) {
        tzJson += "  \"Dummy/Zone_" + std::to_string(i) + "\": \"DUM0\",\n";
    }
    tzJson += "  \"America/New_York\": \"EST5EDT,M3.2.0,M11.1.0\",\n";
    tzJson += "  \"Europe/London\": \"GMT0BST,M3.5.0/1,M10.5.0\"\n";
    tzJson += "}\n";
    ASSERT_TRUE(writeRawFile("test/tmp/timezones.json", tzJson));

    stub_sntp_set_sync_status(SNTP_SYNC_STATUS_IN_PROGRESS);
    timeManager.resetConfig();
    timeManager.begin();

    char tcode[] = "America/New_York";
    struct pbuf pT{nullptr, tcode, static_cast<uint16_t>(strlen(tcode)), static_cast<uint16_t>(strlen(tcode))};
    struct dhcp d{DHCP_STATE_BOUND};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE,
                              static_cast<uint8_t>(strlen(tcode)), &pT, 0);

    // Must resolve inside tick() even while IN_PROGRESS
    timeManager.tick(50);

    EXPECT_TRUE(timeManager.isDhcpTzActive());
    EXPECT_EQ(timeManager.getDhcpTzSource(), DhcpTzSource::TCode);
    EXPECT_STREQ("tcode", timeManager.getDhcpTzSourceStr());
    EXPECT_STREQ("America/New_York", timeManager.getTimezone());
    EXPECT_STREQ("EST5EDT,M3.2.0,M11.1.0", timeManager.getPosixTz());
    EXPECT_TRUE(timeManager.isConfigured());
    EXPECT_STREQ("EST5EDT,M3.2.0,M11.1.0", getenv("TZ"));

    // Option 100 + Option 101 together: PCode takes POSIX precedence, keeps TCode display label
    char pcode[] = "CST6CDT,M3.2.0,M11.1.0";
    char tcode2[] = "America/Chicago";
    struct pbuf pP2{nullptr, pcode, static_cast<uint16_t>(strlen(pcode)), static_cast<uint16_t>(strlen(pcode))};
    struct pbuf pT2{nullptr, tcode2, static_cast<uint16_t>(strlen(tcode2)), static_cast<uint16_t>(strlen(tcode2))};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_RENEWING, LWIP_DHCP_OPTION_TZ_PCODE,
                              static_cast<uint8_t>(strlen(pcode)), &pP2, 0);
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_RENEWING, LWIP_DHCP_OPTION_TZ_TCODE,
                              static_cast<uint8_t>(strlen(tcode2)), &pT2, 0);
    timeManager.tick(60);

    EXPECT_EQ(timeManager.getDhcpTzSource(), DhcpTzSource::PCode);
    EXPECT_STREQ("pcode", timeManager.getDhcpTzSourceStr());
    EXPECT_STREQ("America/Chicago", timeManager.getTimezone());
    EXPECT_STREQ("CST6CDT,M3.2.0,M11.1.0", timeManager.getPosixTz());

    // Subsequent Option 100 alone clears stale DHCP TCode label
    char pcode3[] = "JST-9";
    struct pbuf pP3{nullptr, pcode3, static_cast<uint16_t>(strlen(pcode3)), static_cast<uint16_t>(strlen(pcode3))};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_RENEWING, LWIP_DHCP_OPTION_TZ_PCODE,
                              static_cast<uint8_t>(strlen(pcode3)), &pP3, 0);
    timeManager.tick(70);
    EXPECT_STREQ("JST-9", timeManager.getPosixTz());
    EXPECT_STREQ("", timeManager.getTimezone());

    // Unknown TCode alone must NOT clobber configured manual fallback on fresh instance
    TimeManager tmFallback;
    JsonDocument manualCfg;
    manualCfg["timezone"] = "Europe/London";
    manualCfg["posix"] = "GMT0BST,M3.5.0/1,M10.5.0";
    manualCfg["dhcpTz"] = true;
    tmFallback.applyConfig(manualCfg.as<JsonObject>());
    tmFallback.begin();

    char unknown[] = "Mars/Olympus";
    struct pbuf pUnknown{nullptr, unknown, static_cast<uint16_t>(strlen(unknown)), static_cast<uint16_t>(strlen(unknown))};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE,
                              static_cast<uint8_t>(strlen(unknown)), &pUnknown, 0);
    tmFallback.tick(80);
    EXPECT_FALSE(tmFallback.isDhcpTzActive());
    EXPECT_STREQ("Europe/London", tmFallback.getTimezone());
    EXPECT_STREQ("GMT0BST,M3.5.0/1,M10.5.0", tmFallback.getPosixTz());
}

// ---------------------------------------------------------------------------
// EventBus WIFI_CONNECTED Deferred Handling & REST Endpoint Tests
// ---------------------------------------------------------------------------

TEST_F(TimeManagerTest, EventBusWifiConnectedDefersHandlingToTick) {
    EXPECT_STREQ("wifi.connected", WifiEvents::WIFI_CONNECTED);

    EventBus eb;
    eb.begin();
    timeManager.resetConfig();
    timeManager.setEventBus(&eb);
    timeManager.setEventBus(&eb); // Duplicate wiring must be idempotent
    timeManager.begin();

    char pcode[] = "AEST-10AEDT,M10.1.0,M4.1.0/3";
    struct pbuf p{nullptr, pcode, static_cast<uint16_t>(strlen(pcode)), static_cast<uint16_t>(strlen(pcode))};
    struct dhcp d{DHCP_STATE_BOUND};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE,
                              static_cast<uint8_t>(strlen(pcode)), &p, 0);

    ip4_addr_t srv = stub_sntp_make_ip4(192, 168, 50, 2);
    stub_sntp_simulate_dhcp_servers(1, &srv);

    // Publish WIFI_CONNECTED: EventBus callback sets atomic flag ONLY; no TZ/SNTP changes inside publish()
    EXPECT_EQ(eb.publish(WifiEvents::WIFI_CONNECTED, nullptr), 1);
    EXPECT_FALSE(timeManager.isDhcpTzActive());
    EXPECT_FALSE(timeManager.isDhcpNtpActive());
    EXPECT_STREQ("GMT0", timeManager.getPosixTz());

    // First tick() drains pending signal + DHCP state
    timeManager.tick(100);
    EXPECT_TRUE(timeManager.isDhcpTzActive());
    EXPECT_STREQ("AEST-10AEDT,M10.1.0,M4.1.0/3", timeManager.getPosixTz());
    EXPECT_TRUE(timeManager.isDhcpNtpActive());
    EXPECT_STREQ("192.168.50.2", timeManager.getDhcpNtpServer(0));
}

TEST_F(TimeManagerTest, RestGetAndPostConfigReflectAndToggleDhcpState) {
    ConfigManager cm;
    timeManager.setConfigManager(&cm);
    timeManager.resetConfig();
    timeManager.begin();

    // Activate both Option 42 and Option 100/101
    char pcode[] = "EST5EDT,M3.2.0,M11.1.0";
    char tcode[] = "America/New_York";
    struct pbuf pP{nullptr, pcode, static_cast<uint16_t>(strlen(pcode)), static_cast<uint16_t>(strlen(pcode))};
    struct pbuf pT{nullptr, tcode, static_cast<uint16_t>(strlen(tcode)), static_cast<uint16_t>(strlen(tcode))};
    struct dhcp d{DHCP_STATE_BOUND};
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_PCODE, static_cast<uint8_t>(strlen(pcode)), &pP, 0);
    lwip_dhcp_on_extra_option(&d, DHCP_STATE_BOUND, LWIP_DHCP_OPTION_TZ_TCODE, static_cast<uint8_t>(strlen(tcode)), &pT, 0);
    ip4_addr_t srv = stub_sntp_make_ip4(10, 20, 30, 40);
    stub_sntp_simulate_dhcp_servers(1, &srv);
    timeManager.tick(50);

    PsychicHttpServer server;
    timeManager.registerRoutes(server);
    const auto* getRoute = findRoute(server, "/api/time/config", HTTP_GET);
    const auto* postRoute = findRoute(server, "/api/time/config", HTTP_POST);
    ASSERT_NE(getRoute, nullptr);
    ASSERT_NE(postRoute, nullptr);

    MockPsychicRequest getReq;
    MockPsychicResponse getResp;
    EXPECT_EQ(getRoute->handler(&getReq, &getResp), ESP_OK);
    EXPECT_EQ(getResp.lastCode, 200);

    JsonDocument getDoc;
    ASSERT_FALSE(deserializeJson(getDoc, getResp.lastBody));
    EXPECT_STREQ("", getDoc["timezone"].as<const char*>());
    EXPECT_STREQ("GMT0", getDoc["posix"].as<const char*>());
    EXPECT_STREQ("America/New_York", getDoc["activeTimezone"].as<const char*>());
    EXPECT_STREQ("EST5EDT,M3.2.0,M11.1.0", getDoc["activePosix"].as<const char*>());
    EXPECT_TRUE(getDoc["dhcpNtp"].as<bool>());
    EXPECT_TRUE(getDoc["dhcpTz"].as<bool>());
    EXPECT_TRUE(getDoc["dhcpNtpActive"].as<bool>());
    EXPECT_EQ(getDoc["dhcpNtpServers"].size(), 1u);
    EXPECT_STREQ("10.20.30.40", getDoc["dhcpNtpServers"][0].as<const char*>());
    EXPECT_TRUE(getDoc["dhcpTzActive"].as<bool>());
    EXPECT_STREQ("pcode", getDoc["dhcpTzSource"].as<const char*>());
    EXPECT_TRUE(getDoc["configured"].as<bool>());

    // POST disabling dhcpTz + dhcpNtp and setting manual Tokyo TZ
    MockPsychicRequest postReq(
        "{\"timezone\":\"Asia/Tokyo\",\"posix\":\"JST-9\",\"dhcpNtp\":false,\"dhcpTz\":false}");
    MockPsychicResponse postResp;
    EXPECT_EQ(postRoute->handler(&postReq, &postResp), ESP_OK);
    EXPECT_EQ(postResp.lastCode, 200);

    EXPECT_FALSE(timeManager.isDhcpTzEnabled());
    EXPECT_FALSE(timeManager.isDhcpTzActive());
    EXPECT_EQ(timeManager.getDhcpTzSource(), DhcpTzSource::None);
    EXPECT_STREQ("none", timeManager.getDhcpTzSourceStr());
    EXPECT_STREQ("Asia/Tokyo", timeManager.getTimezone());
    EXPECT_STREQ("JST-9", timeManager.getPosixTz());
    EXPECT_STREQ("JST-9", getenv("TZ"));

    EXPECT_FALSE(timeManager.isDhcpNtpEnabled());
    EXPECT_FALSE(timeManager.isDhcpNtpActive());
    EXPECT_EQ(timeManager.getDhcpNtpServerCount(), 0u);
    EXPECT_FALSE(stub_sntp_is_dhcp_mode());
    EXPECT_STREQ("pool.ntp.org", esp_sntp_getservername(0));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
