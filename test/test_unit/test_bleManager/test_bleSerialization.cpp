/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleSerialization.cpp
 * Description: Unit tests for BLE JSON serialization, enum string conversions,
 *              address redaction guarantees, MCP tool registration, and tab metadata.
 *              Authoritative: RFC #225 rev 2 §12, §13, §15.
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "BLEManager.hpp"
#include "BLESerialization.hpp"
#include "MCPToolRegistry.hpp"
#include "interfaces/ITabRegistry.hpp"
#include <PsychicHttp.h>

namespace {

class MockTabRegistry : public ITabRegistry {
public:
    struct TabInfo {
        std::string title;
        int order = 0;
        std::string iconSvg;
        std::string scriptPath;
    };
    std::vector<TabInfo> tabs;

    void addTab(const char* name,
                const char* /*parent*/ = nullptr,
                int         order  = 100,
                const char* icon   = nullptr,
                const char* script = nullptr) override {
        TabInfo info;
        info.title = name ? name : "";
        info.order = order;
        info.iconSvg = icon ? icon : "";
        info.scriptPath = script ? script : "";
        tabs.push_back(info);
    }
};

class DummyClient : public IBLEClientProvider {
public:
    DummyClient(const char* id = "test_cli", const char* label = "Test Client")
        : _id(id), _label(label) {}

    const char* getProviderId() const override { return _id.c_str(); }
    const char* getDeviceTypeLabel() const override { return _label.c_str(); }
    size_t getTargetServiceUUIDs(uint16_t*, size_t) const override { return 0; }
    size_t getSubscriptions(BLESubscriptionSpec*, size_t) const override { return 0; }
    void onConnected(const BLEStoredDevice&) override {}
    void onNotification(const BLENotification&) override {}
    void onDisconnected(BLEDisconnectReason) override {}
    bool acceptScanResult(const BLEScanResult&) const override { return true; }

private:
    std::string _id;
    std::string _label;
};

} // namespace

TEST(BLESerializationTest, EnumToStringConversions) {
    EXPECT_STREQ(BLESerialization::stateToString(BLEManagerState::Disabled), "Disabled");
    EXPECT_STREQ(BLESerialization::stateToString(BLEManagerState::Idle), "Idle");
    EXPECT_STREQ(BLESerialization::stateToString(BLEManagerState::Starting), "Starting");
    EXPECT_STREQ(BLESerialization::stateToString(BLEManagerState::Active), "Active");
    EXPECT_STREQ(BLESerialization::stateToString(BLEManagerState::Paused), "Paused");
    EXPECT_STREQ(BLESerialization::stateToString(BLEManagerState::Error), "Error");

    EXPECT_STREQ(BLESerialization::addressTypeToString(BLEPeerAddress::Type::Public), "Public");
    EXPECT_STREQ(BLESerialization::addressTypeToString(BLEPeerAddress::Type::RandomStatic), "RandomStatic");
    EXPECT_STREQ(BLESerialization::addressTypeToString(BLEPeerAddress::Type::RandomResolvable), "RandomResolvable");
    EXPECT_STREQ(BLESerialization::addressTypeToString(BLEPeerAddress::Type::RandomNonResolvable), "RandomNonResolvable");
    EXPECT_STREQ(BLESerialization::addressTypeToString(BLEPeerAddress::Type::Unknown), "Unknown");

    EXPECT_STREQ(BLESerialization::reconnectPhaseToString(BLEReconnectPolicy::Phase::Idle), "Idle");
    EXPECT_STREQ(BLESerialization::reconnectPhaseToString(BLEReconnectPolicy::Phase::Fast), "Fast");
    EXPECT_STREQ(BLESerialization::reconnectPhaseToString(BLEReconnectPolicy::Phase::Slow), "Slow");
    EXPECT_STREQ(BLESerialization::reconnectPhaseToString(BLEReconnectPolicy::Phase::Exhausted), "Exhausted");
}

TEST(BLESerializationTest, ParseAddressType) {
    BLEPeerAddress::Type outType;

    EXPECT_TRUE(BLESerialization::parseAddressType("Public", outType));
    EXPECT_EQ(outType, BLEPeerAddress::Type::Public);

    EXPECT_TRUE(BLESerialization::parseAddressType("public", outType));
    EXPECT_EQ(outType, BLEPeerAddress::Type::Public);

    EXPECT_TRUE(BLESerialization::parseAddressType("RandomStatic", outType));
    EXPECT_EQ(outType, BLEPeerAddress::Type::RandomStatic);

    EXPECT_TRUE(BLESerialization::parseAddressType("RandomResolvable", outType));
    EXPECT_EQ(outType, BLEPeerAddress::Type::RandomResolvable);

    EXPECT_TRUE(BLESerialization::parseAddressType("RandomNonResolvable", outType));
    EXPECT_EQ(outType, BLEPeerAddress::Type::RandomNonResolvable);

    EXPECT_FALSE(BLESerialization::parseAddressType("InvalidType", outType));
    EXPECT_FALSE(BLESerialization::parseAddressType(nullptr, outType));
}

TEST(BLESerializationTest, SerializeScanResultsUnredactedVsRedacted) {
    BLEScanResult results[2];
    
    // Result 1
    ASSERT_TRUE(BLEPeerAddress::parse("AA:BB:CC:DD:EE:FF", BLEPeerAddress::Type::Public, results[0].address));
    strncpy(results[0].name, "TestSensor", sizeof(results[0].name) - 1);
    results[0].rssi = -65;
    results[0].appearance = 0x0340;
    results[0].connectable = true;
    results[0].uuid16Count = 1;
    results[0].uuids16[0] = 0x180D;

    // Result 2
    ASSERT_TRUE(BLEPeerAddress::parse("11:22:33:44:55:66", BLEPeerAddress::Type::RandomStatic, results[1].address));
    results[1].name[0] = '\0';
    results[1].rssi = -80;
    results[1].connectable = false;
    results[1].uuid16Count = 0;

    // Unredacted (REST)
    {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        BLESerialization::serializeScanResults(arr, results, 2, /*redactAddresses=*/false);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_STREQ(arr[0]["address"], "AA:BB:CC:DD:EE:FF");
        EXPECT_STREQ(arr[0]["addressRedacted"], "**:**:**:**:EE:FF");
        EXPECT_STREQ(arr[0]["addressType"], "Public");
        EXPECT_STREQ(arr[0]["name"], "TestSensor");
        EXPECT_EQ(arr[0]["rssi"].as<int>(), -65);
        EXPECT_TRUE(arr[0]["connectable"].as<bool>());
        EXPECT_EQ(arr[0]["uuids16"][0].as<uint16_t>(), 0x180D);

        EXPECT_STREQ(arr[1]["address"], "11:22:33:44:55:66");
        EXPECT_STREQ(arr[1]["addressRedacted"], "**:**:**:**:55:66");
        EXPECT_STREQ(arr[1]["name"], "");
        EXPECT_FALSE(arr[1]["connectable"].as<bool>());
    }

    // Redacted (MCP)
    {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        BLESerialization::serializeScanResults(arr, results, 2, /*redactAddresses=*/true);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_STREQ(arr[0]["address"], "**:**:**:**:EE:FF");
        EXPECT_STREQ(arr[0]["addressRedacted"], "**:**:**:**:EE:FF");
        EXPECT_STREQ(arr[1]["address"], "**:**:**:**:55:66");
        EXPECT_STREQ(arr[1]["addressRedacted"], "**:**:**:**:55:66");
    }
}

TEST(BLESerializationTest, SerializeStatusFullAndRedacted) {
    BLEManager mgr;
    DummyClient client("hrm", "Heart Rate Monitor");
    EXPECT_TRUE(mgr.registerClientProvider(&client));

    // Store a paired device for client
    BLEPeerAddress devAddr;
    ASSERT_TRUE(BLEPeerAddress::parse("AA:BB:CC:DD:11:22", BLEPeerAddress::Type::Public, devAddr));
    EXPECT_TRUE(mgr.pair("hrm", devAddr));

    // Connect a simulated peripheral peer
    BLEQueueMessage connMsg;
    connMsg.type = BLEQueueMessage::Type::PeerConnected;
    connMsg.handle = 1;
    connMsg.params.minIntervalUnits = 24;
    connMsg.params.maxIntervalUnits = 24;
    connMsg.params.latency = 0;
    connMsg.params.supervisionTimeoutUnits = 200;
    connMsg.mtu = 512;
    connMsg.boolVal1 = true; // encrypted
    ASSERT_TRUE(BLEPeerAddress::parse("00:11:22:33:44:55", BLEPeerAddress::Type::Public, connMsg.address));
    EXPECT_TRUE(mgr.postTestQueueMessage(connMsg));
    mgr.tick(100);

    // 1. Unredacted status (REST)
    {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        BLESerialization::serializeStatus(root, mgr, /*redactAddresses=*/false);

        EXPECT_STREQ(root["state"], "Idle");
        EXPECT_FALSE(root["isScanning"].as<bool>());
        EXPECT_FALSE(root["isAdvertising"].as<bool>());

        // Verify advertising budget object exists
        EXPECT_TRUE(root["advertising"]["budget"].is<JsonObject>());
        EXPECT_TRUE(root["advertising"]["primaryAdvHex"].is<const char*>());

        // Verify peripheral connection (unredacted)
        JsonArray conns = root["connections"];
        ASSERT_EQ(conns.size(), 1);
        EXPECT_EQ(conns[0]["handle"].as<int>(), 1);
        EXPECT_STREQ(conns[0]["address"], "00:11:22:33:44:55");
        EXPECT_STREQ(conns[0]["addressRedacted"], "**:**:**:**:44:55");
        EXPECT_EQ(conns[0]["mtu"].as<int>(), 512);
        EXPECT_TRUE(conns[0]["encrypted"].as<bool>());

        // Verify client provider stored device (unredacted)
        JsonArray devs = root["devices"];
        ASSERT_EQ(devs.size(), 1);
        EXPECT_STREQ(devs[0]["providerId"], "hrm");
        EXPECT_STREQ(devs[0]["label"], "Heart Rate Monitor");
        EXPECT_FALSE(devs[0]["connected"].as<bool>());
        EXPECT_STREQ(devs[0]["stored"]["address"], "AA:BB:CC:DD:11:22");
        EXPECT_STREQ(devs[0]["stored"]["addressRedacted"], "**:**:**:**:11:22");
        EXPECT_STREQ(devs[0]["stored"]["addressType"], "Public");

        // Verify counters
        EXPECT_EQ(root["counters"]["droppedEvents"].as<uint32_t>(), 0);
        EXPECT_EQ(root["counters"]["scanInhibitCount"].as<uint32_t>(), 0);
    }

    // 2. Redacted status (MCP)
    {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        BLESerialization::serializeStatus(root, mgr, /*redactAddresses=*/true);

        // Connections must be redacted
        JsonArray conns = root["connections"];
        ASSERT_EQ(conns.size(), 1);
        EXPECT_STREQ(conns[0]["address"], "**:**:**:**:44:55");
        EXPECT_STREQ(conns[0]["addressRedacted"], "**:**:**:**:44:55");

        // Stored device must be redacted
        JsonArray devs = root["devices"];
        ASSERT_EQ(devs.size(), 1);
        EXPECT_STREQ(devs[0]["stored"]["address"], "**:**:**:**:11:22");
        EXPECT_STREQ(devs[0]["stored"]["addressRedacted"], "**:**:**:**:11:22");
    }
}

TEST(BLESerializationTest, RegisterTabsMetadata) {
    BLEManager mgr;
    MockTabRegistry registry;
    mgr.registerTabs(registry);

    ASSERT_EQ(registry.tabs.size(), 1);
    EXPECT_EQ(registry.tabs[0].title, "Bluetooth");
    EXPECT_EQ(registry.tabs[0].order, 15);
    EXPECT_EQ(registry.tabs[0].scriptPath, "/ble.js");
    EXPECT_NE(registry.tabs[0].iconSvg.find("polyline"), std::string::npos);
}

TEST(BLESerializationTest, RegisterMCPToolsAndExecution) {
    BLEManager mgr;
    DummyClient client("hrm", "Heart Rate Monitor");
    EXPECT_TRUE(mgr.registerClientProvider(&client));

    MCPToolRegistry registry;
    mgr.registerMCPTools(registry);

    const auto* statusTool = registry.findTool("ble_status");
    const auto* scanTool = registry.findTool("ble_scan");
    const auto* pairTool = registry.findTool("ble_pair");
    const auto* forgetTool = registry.findTool("ble_forget");

    ASSERT_NE(statusTool, nullptr);
    ASSERT_NE(scanTool, nullptr);
    ASSERT_NE(pairTool, nullptr);
    ASSERT_NE(forgetTool, nullptr);

    // MutatesState flags check
    EXPECT_FALSE(statusTool->mutatesState);
    EXPECT_TRUE(scanTool->mutatesState);
    EXPECT_TRUE(pairTool->mutatesState);
    EXPECT_TRUE(forgetTool->mutatesState);

    // Execute ble_status via registry handler
    {
        JsonDocument paramsDoc;
        JsonObject params = paramsDoc.to<JsonObject>();
        JsonDocument resultDoc;
        JsonObject result = resultDoc.to<JsonObject>();

        statusTool->handler(statusTool->context, params, result);
        EXPECT_STREQ(result["state"], "Idle");
        EXPECT_TRUE(result["devices"].is<JsonArray>());
    }

    // Execute ble_scan with unknown provider -> should report error
    {
        JsonDocument paramsDoc;
        JsonObject params = paramsDoc.to<JsonObject>();
        params["providerId"] = "unknown_cli";
        JsonDocument resultDoc;
        JsonObject result = resultDoc.to<JsonObject>();

        scanTool->handler(scanTool->context, params, result);
        EXPECT_TRUE(result["isError"].as<bool>());
        EXPECT_STREQ(result["error"], "unknown providerId");
    }

    // Execute ble_pair with invalid address -> should report error
    {
        JsonDocument paramsDoc;
        JsonObject params = paramsDoc.to<JsonObject>();
        params["providerId"] = "hrm";
        params["address"] = "invalid_mac";
        params["addressType"] = "Public";
        JsonDocument resultDoc;
        JsonObject result = resultDoc.to<JsonObject>();

        pairTool->handler(pairTool->context, params, result);
        EXPECT_TRUE(result["isError"].as<bool>());
        EXPECT_STREQ(result["error"], "invalid address");
    }

    // Execute ble_forget with unknown provider -> should report error
    {
        JsonDocument paramsDoc;
        JsonObject params = paramsDoc.to<JsonObject>();
        params["providerId"] = "unknown_cli";
        JsonDocument resultDoc;
        JsonObject result = resultDoc.to<JsonObject>();

        forgetTool->handler(forgetTool->context, params, result);
        EXPECT_TRUE(result["isError"].as<bool>());
        EXPECT_STREQ(result["error"], "unknown providerId");
    }
}

class MockPsychicRequest : public PsychicRequest {
public:
    std::string _body;
    const char* bodyCStr() override { return _body.c_str(); }
    std::string body() override { return _body; }
};

class MockPsychicResponse : public PsychicResponse {
public:
    int code = 0;
    std::string contentType;
    std::string content;
    esp_err_t send(int c, const char* ct, const char* cnt) override {
        code = c;
        contentType = ct ? ct : "";
        content = cnt ? cnt : "";
        return ESP_OK;
    }
};

TEST(BLESerializationTest, RegisterRoutesAndDispatch) {
    BLEManager mgr;
    DummyClient client("hrm", "Heart Rate Monitor");
    EXPECT_TRUE(mgr.registerClientProvider(&client));
    EXPECT_EQ(mgr.getMaxPostBody(), 512);

    PsychicHttpServer server;
    mgr.registerRoutes(server);

    auto findRoute = [&](const std::string& uri, int method) -> PsychicHttpServer::Route* {
        for (auto& r : server.routes) {
            if (r.uri == uri && r.method == method) return &r;
        }
        return nullptr;
    };

    auto* statusRoute = findRoute("/api/ble/status", HTTP_GET);
    auto* scanRoute = findRoute("/api/ble/scan", HTTP_POST);
    auto* scanResRoute = findRoute("/api/ble/scan/results", HTTP_GET);
    auto* pairRoute = findRoute("/api/ble/pair", HTTP_POST);
    auto* forgetRoute = findRoute("/api/ble/forget", HTTP_POST);

    ASSERT_NE(statusRoute, nullptr);
    ASSERT_NE(scanRoute, nullptr);
    ASSERT_NE(scanResRoute, nullptr);
    ASSERT_NE(pairRoute, nullptr);
    ASSERT_NE(forgetRoute, nullptr);

    // Test GET /api/ble/status
    {
        MockPsychicRequest req;
        MockPsychicResponse resp;
        statusRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 200);
        EXPECT_EQ(resp.contentType, "application/json");
        EXPECT_NE(resp.content.find("\"state\":\"Idle\""), std::string::npos);
    }

    // Test POST /api/ble/scan with empty body -> 400
    {
        MockPsychicRequest req;
        MockPsychicResponse resp;
        scanRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 400);
    }

    // Test POST /api/ble/scan with unknown provider -> 400
    {
        MockPsychicRequest req;
        req._body = "{\"providerId\":\"unknown\"}";
        MockPsychicResponse resp;
        scanRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 400);
    }

    // Test POST /api/ble/pair with unknown provider -> 404
    {
        MockPsychicRequest req;
        req._body = "{\"providerId\":\"unknown\",\"address\":\"AA:BB:CC:DD:EE:FF\",\"addressType\":\"Public\"}";
        MockPsychicResponse resp;
        pairRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 404);
    }

    // Test POST /api/ble/pair with invalid address -> 400
    {
        MockPsychicRequest req;
        req._body = "{\"providerId\":\"hrm\",\"address\":\"invalid\",\"addressType\":\"Public\"}";
        MockPsychicResponse resp;
        pairRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 400);
    }

    // Test POST /api/ble/forget with unknown provider -> 404
    {
        MockPsychicRequest req;
        req._body = "{\"providerId\":\"unknown\"}";
        MockPsychicResponse resp;
        forgetRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 404);
    }

    // Test GET /api/ble/scan/results -> 200
    {
        MockPsychicRequest req;
        MockPsychicResponse resp;
        scanResRoute->handler(&req, &resp);
        EXPECT_EQ(resp.code, 200);
        EXPECT_EQ(resp.contentType, "application/json");
    }
}
