/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_mcpServer/test_displayMcpTools.cpp
 * Description: GoogleTest unit-test suite for the U8G2DisplayModule's MCP tool
 *              contributions. Validates that registerMCPTools() correctly registers
 *              display_status (read-safe) and display_control (mutating) tools,
 *              and that both tools produce correct JSON responses for various inputs.
 *
 *              Tests use a live U8G2DisplayModule instance (in native builds mode)
 *              and a standalone MCPToolRegistry — no MCPServer or Application wiring
 *              is needed.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#ifdef WITH_DISPLAY_U8G2
#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "MCPToolRegistry.hpp"
#include "U8G2DisplayModule.hpp"

// =============================================================================
// Mock IDisplayBoard — minimal concrete board for carousel state tests
// =============================================================================

/** @brief Minimal IDisplayBoard mock for verifying display_status carousel output. */
class DisplayMcpMockBoard : public IDisplayBoard {
public:
    const char* name = "MockBoard";

    /** @brief Render stub — no-op in native builds. */
    void render(U8G2& /*u8g2*/) override {}

    /** @brief Never expires. */
    bool isExpired() const override { return false; }

    /** @brief Return the configured board name. */
    const char* getBoardName() const override { return name; }
};

// =============================================================================
// Test Fixture: DisplayMcpToolsTest
// =============================================================================

/**
 * @brief Fixture that creates a U8G2DisplayModule + MCPToolRegistry and
 *        calls registerMCPTools() once during SetUp().
 *
 * Each test can invoke display_status or display_control via the registry's
 * findTool() → handler() path without requiring MCPServer plumbing.
 */
class DisplayMcpToolsTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;   ///< Fresh display module (native-safe)
    MCPToolRegistry registry;    ///< Standalone registry for tool registration

    void SetUp() override {
        display.registerMCPTools(registry);
    }

    /**
     * @brief Helper: invoke a registered tool by name with the given params.
     *
     * Allocates a temporary JsonDocument for params and result, calls the
     * handler, and returns the result document. Returns an empty document
     * if the tool is not found.
     *
     * @param toolName Name of the registered tool.
     * @param paramSetup Lambda that populates the params JsonObject.
     * @return JsonDocument containing the handler result.
     */
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
// Test Group: Tool Registration
// =============================================================================

/** registerMCPTools must register exactly two tools. */
TEST_F(DisplayMcpToolsTest, Registration_RegistersTwoTools) {
    EXPECT_EQ(registry.size(), 2u);
}

/** display_status must be findable in the registry. */
TEST_F(DisplayMcpToolsTest, Registration_DisplayStatusExists) {
    EXPECT_NE(registry.findTool("display_status"), nullptr);
}

/** display_control must be findable in the registry. */
TEST_F(DisplayMcpToolsTest, Registration_DisplayControlExists) {
    EXPECT_NE(registry.findTool("display_control"), nullptr);
}

/** display_status must be tagged as non-mutating (read-safe). */
TEST_F(DisplayMcpToolsTest, Registration_DisplayStatusIsReadOnly) {
    const MCPToolEntry* tool = registry.findTool("display_status");
    ASSERT_NE(tool, nullptr);
    EXPECT_FALSE(tool->mutatesState);
}

/** display_control must be tagged as mutating. */
TEST_F(DisplayMcpToolsTest, Registration_DisplayControlMutates) {
    const MCPToolEntry* tool = registry.findTool("display_control");
    ASSERT_NE(tool, nullptr);
    EXPECT_TRUE(tool->mutatesState);
}

// =============================================================================
// Test Group: display_status — Response Structure
// =============================================================================

/** display_status must return a "brightness" key. */
TEST_F(DisplayMcpToolsTest, Status_ContainsBrightness) {
    auto result = callTool("display_status");
    EXPECT_TRUE(result["brightness"].is<int>());
}

/** display_status must return a "flip" key. */
TEST_F(DisplayMcpToolsTest, Status_ContainsFlip) {
    auto result = callTool("display_status");
    EXPECT_TRUE(result["flip"].is<bool>());
}

/** display_status must return an "activeBoard" key. */
TEST_F(DisplayMcpToolsTest, Status_ContainsActiveBoard) {
    auto result = callTool("display_status");
    EXPECT_TRUE(result["activeBoard"].is<const char*>());
}

/** display_status must return a "screensaver" sub-object. */
TEST_F(DisplayMcpToolsTest, Status_ContainsScreensaverObject) {
    auto result = callTool("display_status");
    EXPECT_TRUE(result["screensaver"].is<JsonObject>());
}

/** display_status screensaver must contain "enabled" boolean. */
TEST_F(DisplayMcpToolsTest, Status_ScreensaverContainsEnabled) {
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_TRUE(ss["enabled"].is<bool>());
}

/** display_status screensaver must contain a "state" string. */
TEST_F(DisplayMcpToolsTest, Status_ScreensaverContainsState) {
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_TRUE(ss["state"].is<const char*>());
}

/** display_status screensaver must contain "dimTimeout". */
TEST_F(DisplayMcpToolsTest, Status_ScreensaverContainsDimTimeout) {
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_TRUE(ss["dimTimeout"].is<unsigned long>());
}

/** display_status screensaver must contain "blankTimeout". */
TEST_F(DisplayMcpToolsTest, Status_ScreensaverContainsBlankTimeout) {
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_TRUE(ss["blankTimeout"].is<unsigned long>());
}

/** display_status must return a "carousel" sub-object. */
TEST_F(DisplayMcpToolsTest, Status_ContainsCarouselObject) {
    auto result = callTool("display_status");
    EXPECT_TRUE(result["carousel"].is<JsonObject>());
}

/** display_status carousel must contain "boardCount". */
TEST_F(DisplayMcpToolsTest, Status_CarouselContainsBoardCount) {
    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_TRUE(car["boardCount"].is<int>());
}

/** display_status carousel must contain "rotationInterval". */
TEST_F(DisplayMcpToolsTest, Status_CarouselContainsRotationInterval) {
    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_TRUE(car["rotationInterval"].is<unsigned long>());
}

/** display_status carousel must contain "suspended". */
TEST_F(DisplayMcpToolsTest, Status_CarouselContainsSuspended) {
    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_TRUE(car["suspended"].is<bool>());
}

// =============================================================================
// Test Group: display_status — Default Values
// =============================================================================

/** A fresh display module must report default brightness (255). */
TEST_F(DisplayMcpToolsTest, Status_DefaultBrightness_Is255) {
    auto result = callTool("display_status");
    EXPECT_EQ(result["brightness"].as<int>(), 255);
}

/** A fresh display module must report no active board (no modal or carousel). */
TEST_F(DisplayMcpToolsTest, Status_DefaultActiveBoard_IsBootOrNone) {
    auto result = callTool("display_status");
    // In native build with no begin() called, visibleBoard() returns the
    // boot board or nullptr depending on _bootComplete state.
    const char* board = result["activeBoard"].as<const char*>();
    EXPECT_NE(board, nullptr);
}

/** A fresh display must report screensaver enabled by default. */
TEST_F(DisplayMcpToolsTest, Status_DefaultScreensaverEnabled) {
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_TRUE(ss["enabled"].as<bool>());
}

/** A fresh display must report screensaver state "active" (not dimmed/blanked). */
TEST_F(DisplayMcpToolsTest, Status_DefaultScreensaverState_IsActive) {
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_STREQ(ss["state"].as<const char*>(), "active");
}

/** A fresh display must report zero carousel boards. */
TEST_F(DisplayMcpToolsTest, Status_DefaultCarouselBoardCount_IsZero) {
    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_EQ(car["boardCount"].as<int>(), 0);
}

/** A fresh display carousel must not be suspended. */
TEST_F(DisplayMcpToolsTest, Status_DefaultCarouselSuspended_IsFalse) {
    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_FALSE(car["suspended"].as<bool>());
}

// =============================================================================
// Test Group: display_status — Reflects Carousel State
// =============================================================================

/** display_status boardCount must reflect added carousel boards. */
TEST_F(DisplayMcpToolsTest, Status_CarouselBoardCount_ReflectsAddedBoards) {
    DisplayMcpMockBoard boardA;
    DisplayMcpMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_EQ(car["boardCount"].as<int>(), 2);
}

/** display_status suspended must reflect carousel.suspend(). */
TEST_F(DisplayMcpToolsTest, Status_CarouselSuspended_ReflectsSuspend) {
    display.carousel.suspend();

    auto result = callTool("display_status");
    JsonObject car = result["carousel"];
    EXPECT_TRUE(car["suspended"].as<bool>());
}

// =============================================================================
// Test Group: display_control — Basic Operation
// =============================================================================

/** display_control with no params must return status "ok". */
TEST_F(DisplayMcpToolsTest, Control_EmptyParams_ReturnsOk) {
    auto result = callTool("display_control");
    EXPECT_STREQ(result["status"].as<const char*>(), "ok");
}

/** display_control screensaverEnabled=false must disable screensaver. */
TEST_F(DisplayMcpToolsTest, Control_DisableScreensaver_UpdatesStatus) {
    // Disable screensaver via control tool
    callTool("display_control", [](JsonObject& p) {
        p["screensaverEnabled"] = false;
    });

    // Verify via status tool
    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_FALSE(ss["enabled"].as<bool>());
}

/** display_control screensaverEnabled=true must re-enable screensaver. */
TEST_F(DisplayMcpToolsTest, Control_EnableScreensaver_UpdatesStatus) {
    // Disable then re-enable
    callTool("display_control", [](JsonObject& p) {
        p["screensaverEnabled"] = false;
    });
    callTool("display_control", [](JsonObject& p) {
        p["screensaverEnabled"] = true;
    });

    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_TRUE(ss["enabled"].as<bool>());
}

/** display_control dimTimeout must update the screensaver dim timeout. */
TEST_F(DisplayMcpToolsTest, Control_SetDimTimeout_UpdatesStatus) {
    callTool("display_control", [](JsonObject& p) {
        p["dimTimeout"] = 60000;
    });

    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_EQ(ss["dimTimeout"].as<unsigned long>(), 60000u);
}

/** display_control blankTimeout must update the screensaver blank timeout. */
TEST_F(DisplayMcpToolsTest, Control_SetBlankTimeout_UpdatesStatus) {
    callTool("display_control", [](JsonObject& p) {
        p["blankTimeout"] = 120000;
    });

    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_EQ(ss["blankTimeout"].as<unsigned long>(), 120000u);
}

/** display_control with multiple params must apply all at once. */
TEST_F(DisplayMcpToolsTest, Control_MultipleParams_AllApplied) {
    callTool("display_control", [](JsonObject& p) {
        p["screensaverEnabled"] = false;
        p["dimTimeout"] = 30000;
        p["blankTimeout"] = 90000;
    });

    auto result = callTool("display_status");
    JsonObject ss = result["screensaver"];
    EXPECT_FALSE(ss["enabled"].as<bool>());
    EXPECT_EQ(ss["dimTimeout"].as<unsigned long>(), 30000u);
    EXPECT_EQ(ss["blankTimeout"].as<unsigned long>(), 90000u);
}

/** display_control action=wake must not crash (no-op in native). */
TEST_F(DisplayMcpToolsTest, Control_WakeAction_DoesNotCrash) {
    auto result = callTool("display_control", [](JsonObject& p) {
        p["action"] = "wake";
    });
    EXPECT_STREQ(result["status"].as<const char*>(), "ok");
}

/** display_control action=sleep must not crash (no-op in native). */
TEST_F(DisplayMcpToolsTest, Control_SleepAction_DoesNotCrash) {
    auto result = callTool("display_control", [](JsonObject& p) {
        p["action"] = "sleep";
    });
    EXPECT_STREQ(result["status"].as<const char*>(), "ok");
}
#endif
