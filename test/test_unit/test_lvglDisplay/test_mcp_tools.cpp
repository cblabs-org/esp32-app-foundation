/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * Module: test/test_unit/test_lvglDisplay/test_mcp_tools.cpp
 * Description: Unit tests for LVGLDisplayModule IMCPContributor implementation,
 *              covering tool discovery, mutability flags, parameters parsing,
 *              and tool contracts (display_status, display_control).
 *
 * Note: capture_display is registered by DisplayServiceHelper (hardware-only,
 *       guarded behind #ifndef UNIT_TEST). Brightness in display_status is
 *       populated by populateCommonMCPStatus() via the helper. These are
 *       tested on-device / via portal web tests.
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "LVGLDisplay.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include "MCPToolRegistry.hpp"
#include "ConfigManager.hpp"

class LVGLMcpToolsTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    LVGLDisplayModule display{adapter};
    MCPToolRegistry registry;

    void SetUp() override {
        display.registerMCPTools(registry);
    }

    JsonDocument callTool(const char* toolName,
                          std::function<void(JsonObject&)> paramSetup = nullptr) {
        const MCPToolEntry* tool = registry.findTool(toolName);
        JsonDocument resultDoc;
        if (!tool) return resultDoc;

        JsonDocument paramsDoc;
        JsonObject params = paramsDoc.to<JsonObject>();
        if (paramSetup) paramSetup(params);

        JsonObject result = resultDoc.to<JsonObject>();
        tool->handler(tool->context, params, result);
        return resultDoc;
    }
};

// =============================================================================
// Test Group: Tool Registration & Discovery
// =============================================================================

TEST_F(LVGLMcpToolsTest, ToolRegistrationAndMutability) {
    // Post-#88: module registers display_status + display_control only;
    // capture_display is registered by DisplayServiceHelper (hardware-only)
    EXPECT_EQ(registry.size(), 2);

    const MCPToolEntry* statusTool = registry.findTool("display_status");
    ASSERT_NE(statusTool, nullptr);
    EXPECT_FALSE(statusTool->mutatesState);

    const MCPToolEntry* controlTool = registry.findTool("display_control");
    ASSERT_NE(controlTool, nullptr);
    EXPECT_TRUE(controlTool->mutatesState);
}

// =============================================================================
// Test Group: display_status Contract
// =============================================================================

TEST_F(LVGLMcpToolsTest, DisplayStatusStructureAndDefaults) {
    JsonDocument doc = callTool("display_status");
    JsonObject result = doc.as<JsonObject>();

    // brightness is populated by populateCommonMCPStatus() via DisplayServiceHelper
    // (hardware-only, guarded behind #ifndef UNIT_TEST) — not asserted on native

    EXPECT_TRUE(result.containsKey("rotation"));
    EXPECT_EQ(result["rotation"].as<int>(), 0);

    EXPECT_TRUE(result.containsKey("screensaverEnabled"));
    EXPECT_TRUE(result["screensaverEnabled"].as<bool>());

    EXPECT_TRUE(result.containsKey("screensaver"));
    JsonObject ss = result["screensaver"].as<JsonObject>();
    EXPECT_TRUE(ss["enabled"].as<bool>());
    EXPECT_STREQ(ss["state"].as<const char*>(), "active");
    EXPECT_EQ(ss["dimTimeout"].as<unsigned long>(), 150000UL);
    EXPECT_EQ(ss["blankTimeout"].as<unsigned long>(), 300000UL);

    EXPECT_TRUE(result.containsKey("resolution"));
    JsonObject res = result["resolution"].as<JsonObject>();
    EXPECT_EQ(res["width"].as<int>(), 240);
    EXPECT_EQ(res["height"].as<int>(), 240);

    EXPECT_TRUE(result.containsKey("activeScreen"));
    EXPECT_STREQ(result["activeScreen"].as<const char*>(), "application");
}

// =============================================================================
// Test Group: display_control Contract
// =============================================================================

TEST_F(LVGLMcpToolsTest, DisplayControlMutations) {
    // 1. Mutate brightness and rotation
    JsonDocument doc1 = callTool("display_control", [](JsonObject& params) {
        params["brightness"] = 180;
        params["rotation"] = 90;
    });
    EXPECT_STREQ(doc1["status"].as<const char*>(), "ok");
    EXPECT_EQ(display.getBrightness(), 180);
    EXPECT_EQ(display.getRotation(), 90);

    // 2. Mutate screensaver timeouts and enabled flag
    JsonDocument doc2 = callTool("display_control", [](JsonObject& params) {
        params["screensaverEnabled"] = false;
        params["dimTimeout"] = 60000;
        params["blankTimeout"] = 120000;
    });
    EXPECT_STREQ(doc2["status"].as<const char*>(), "ok");
    EXPECT_FALSE(display.getScreensaverEnabled());
    EXPECT_EQ(display.getDimTimeout(), 60000UL);
    EXPECT_EQ(display.getBlankTimeout(), 120000UL);

    // 3. Test action wake / sleep
    JsonDocument docWake = callTool("display_control", [](JsonObject& params) {
        params["action"] = "wake";
    });
    EXPECT_STREQ(docWake["status"].as<const char*>(), "ok");

    JsonDocument docSleep = callTool("display_control", [](JsonObject& params) {
        params["action"] = "sleep";
    });
    EXPECT_STREQ(docSleep["status"].as<const char*>(), "ok");

    // Verify updated status output (brightness omitted — populated by helper on hardware)
    JsonDocument docStatus = callTool("display_status");
    EXPECT_EQ(docStatus["rotation"].as<int>(), 90);
    EXPECT_FALSE(docStatus["screensaverEnabled"].as<bool>());
}

// capture_display contract: now registered by DisplayServiceHelper (hardware-only).
// Covered by portal web tests + on-device verification.
