/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_mcpServer/test_mcpServer.cpp
 * Description: GoogleTest unit-test suite for the MCP server module.
 *              Covers McpToolRegistry (add/find/size), mcpBuiltinTools
 *              (registration count, tool existence, handler execution),
 *              McpServer JSON-RPC dispatch (initialize, ping, tools/list,
 *              tools/call, parse error, method not found), and Application
 *              integration (registerMcpContributor pre/post lock, null guard).
 *
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required). NATIVE_BUILD is defined by the [env:native]
 *              environment in platformio.ini.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "mcpToolRegistry.hpp"
#include "mcpServer.hpp"
#include "mcpBuiltinTools.hpp"
#include "interfaces/IMcpContributor.hpp"

// =============================================================================
// Test Group 1: McpToolRegistry — Core registry mechanics
// =============================================================================

/** A freshly constructed registry must report size 0. */
TEST(McpToolRegistryTest, StartsEmpty) {
    McpToolRegistry registry;
    EXPECT_EQ(registry.size(), 0u);
}

/** Adding one tool must increment the size to 1. */
TEST(McpToolRegistryTest, AddToolIncreasesSize) {
    McpToolRegistry registry;
    registry.addTool("test_tool", "A test tool",
        [](JsonObject& params, JsonObject& result) {
            (void)params;
            result["ok"] = true;
        }
    );
    EXPECT_EQ(registry.size(), 1u);
}

/** A registered tool must be findable by its exact name. */
TEST(McpToolRegistryTest, FindRegisteredTool) {
    McpToolRegistry registry;
    registry.addTool("my_tool", "My tool",
        [](JsonObject& params, JsonObject& result) {
            (void)params;
            result["ok"] = true;
        }
    );
    const McpToolEntry* found = registry.findTool("my_tool");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "my_tool");
}

/** Searching for an unregistered tool name must return nullptr. */
TEST(McpToolRegistryTest, FindUnknownToolReturnsNull) {
    McpToolRegistry registry;
    EXPECT_EQ(registry.findTool("bogus"), nullptr);
}

/** A tool's handler must execute correctly when invoked directly. */
TEST(McpToolRegistryTest, ToolHandlerExecutes) {
    McpToolRegistry registry;

    bool handlerCalled = false;
    registry.addTool("flag_tool", "Sets a flag",
        [&handlerCalled](JsonObject& params, JsonObject& result) {
            (void)params;
            handlerCalled = true;
            result["executed"] = true;
        }
    );

    const McpToolEntry* tool = registry.findTool("flag_tool");
    ASSERT_NE(tool, nullptr);

    // Invoke the handler directly
    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(args, resultObj);

    EXPECT_TRUE(handlerCalled);
    EXPECT_TRUE(resultObj["executed"].as<bool>());
}

/** Adding multiple tools must make all of them findable. */
TEST(McpToolRegistryTest, MultipleToolsRegistered) {
    McpToolRegistry registry;
    auto noop = [](JsonObject& p, JsonObject& r) { (void)p; (void)r; };

    registry.addTool("tool_a", "Tool A", noop);
    registry.addTool("tool_b", "Tool B", noop);
    registry.addTool("tool_c", "Tool C", noop);

    EXPECT_EQ(registry.size(), 3u);
    EXPECT_NE(registry.findTool("tool_a"), nullptr);
    EXPECT_NE(registry.findTool("tool_b"), nullptr);
    EXPECT_NE(registry.findTool("tool_c"), nullptr);
}

/** getTools() must return a vector whose size matches size(). */
TEST(McpToolRegistryTest, GetToolsReturnsAll) {
    McpToolRegistry registry;
    auto noop = [](JsonObject& p, JsonObject& r) { (void)p; (void)r; };

    registry.addTool("x", "X", noop);
    registry.addTool("y", "Y", noop);

    EXPECT_EQ(registry.getTools().size(), registry.size());
}

// =============================================================================
// Test Group 2: McpBuiltinTools — Built-in tool registration and execution
// =============================================================================

/** registerAll() with null configManager must register at least 7 tools. */
TEST(McpBuiltinToolsTest, RegistersExpectedToolCount) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_GE(registry.size(), 7u);
}

/** system_info tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, SystemInfoToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("system_info"), nullptr);
}

/** get_config tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, GetConfigToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("get_config"), nullptr);
}

/** set_config tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, SetConfigToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("set_config"), nullptr);
}

/** reboot tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, RebootToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("reboot"), nullptr);
}

/** get_logs tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, GetLogsToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("get_logs"), nullptr);
}

/** list_tools tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, ListToolsToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("list_tools"), nullptr);
}

/** capture_display tool must be findable after registerAll() even if display is null. */
TEST(McpBuiltinToolsTest, CaptureDisplayToolExists) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("capture_display"), nullptr);
}

/** system_info handler must return firmware_version in its result. */
TEST(McpBuiltinToolsTest, SystemInfoReturnsData) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);

    const McpToolEntry* tool = registry.findTool("system_info");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(args, resultObj);

    // Must contain firmware_version (the APP_FOUNDATION_VERSION macro)
    EXPECT_TRUE(resultObj["firmware_version"].is<const char*>());
    EXPECT_STREQ(resultObj["firmware_version"].as<const char*>(), APP_FOUNDATION_VERSION);
}

/** get_config with null configManager must return an error in result. */
TEST(McpBuiltinToolsTest, GetConfigWithoutManager) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);

    const McpToolEntry* tool = registry.findTool("get_config");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(args, resultObj);

    EXPECT_TRUE(resultObj["error"].is<const char*>());
    EXPECT_TRUE(resultObj["isError"].as<bool>());
}

/** capture_display with null display must return an error in result. */
TEST(McpBuiltinToolsTest, CaptureDisplayWithoutDisplay) {
    McpToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr); // No display info passed

    const McpToolEntry* tool = registry.findTool("capture_display");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(args, resultObj);

    EXPECT_TRUE(resultObj["error"].is<const char*>());
    EXPECT_STREQ(resultObj["error"].as<const char*>(), "No display registered");
    EXPECT_TRUE(resultObj["isError"].as<bool>());
}

/** capture_display with a valid display buffer must return base64 data. */
TEST(McpBuiltinToolsTest, CaptureDisplayWithDisplay) {
    McpToolRegistry registry;

    // Create a small synthetic framebuffer (4 bytes)
    uint8_t buffer[] = { 0xAA, 0xBB, 0xCC, 0xDD };
    DisplayInfo display;
    display.framebuffer     = buffer;
    display.framebufferSize = sizeof(buffer);
    display.width           = 16;
    display.height          = 2;

    mcpBuiltinTools::registerAll(registry, nullptr, &display);

    const McpToolEntry* tool = registry.findTool("capture_display");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(args, resultObj);

    // Must contain format, width, height, and data fields
    EXPECT_STREQ(resultObj["format"].as<const char*>(), "xbm");
    EXPECT_EQ(resultObj["width"].as<int>(), 16);
    EXPECT_EQ(resultObj["height"].as<int>(), 2);
    EXPECT_TRUE(resultObj["data"].is<const char*>());
    // Base64 of 4 bytes should be 8 chars (e.g. "qrvM3Q==")
    EXPECT_GT(strlen(resultObj["data"].as<const char*>()), 0u);
}

// =============================================================================
// Test Group 3: McpServer — JSON-RPC dispatch via processLineForTest()
// =============================================================================

/**
 * @brief Helper: Feed a JSON-RPC request to the server and capture the response.
 *
 * Clears the MockSerial output buffer, processes the line via the native
 * test harness, and returns the full serialised response string.
 */
static std::string processAndCapture(McpServer& server, const char* jsonLine) {
    Serial.flush(); // Clear any prior output
    server.processLineForTest(jsonLine);
    return Serial.flush();
}

/** initialize must return protocolVersion in the result. */
TEST(McpServerTest, InitializeReturnsCapabilities) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":1,"method":"initialize"})");

    // Parse the response JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_STREQ(doc["jsonrpc"].as<const char*>(), "2.0");
    EXPECT_EQ(doc["id"].as<int>(), 1);
    EXPECT_TRUE(doc["result"]["protocolVersion"].is<const char*>());
    EXPECT_STREQ(doc["result"]["protocolVersion"].as<const char*>(), "2024-11-05");
}

/** ping must return an empty result object. */
TEST(McpServerTest, PingReturnsEmptyResult) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":2,"method":"ping"})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_EQ(doc["id"].as<int>(), 2);
    // result must be present and be an object
    EXPECT_TRUE(doc["result"].is<JsonObject>());
}

/** tools/list must return a tools array with at least the built-in tools. */
TEST(McpServerTest, ToolsListReturnsTools) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":3,"method":"tools/list"})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_EQ(doc["id"].as<int>(), 3);
    JsonArray tools = doc["result"]["tools"].as<JsonArray>();
    EXPECT_GE(tools.size(), 7u); // 7 built-in tools
}

/** tools/call with an unknown tool name must return an error. */
TEST(McpServerTest, ToolsCallUnknownToolReturnsError) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"nonexistent_tool"}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_EQ(doc["id"].as<int>(), 4);
    EXPECT_TRUE(doc["error"].is<JsonObject>());
    EXPECT_EQ(doc["error"]["code"].as<int>(), -32602);
}

/** tools/call with a known tool must return a result with content. */
TEST(McpServerTest, ToolsCallKnownToolReturnsResult) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"system_info"}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_EQ(doc["id"].as<int>(), 5);
    EXPECT_TRUE(doc["result"].is<JsonObject>());
    // The result should contain content array with a text entry
    JsonArray content = doc["result"]["content"].as<JsonArray>();
    ASSERT_GE(content.size(), 1u);
    EXPECT_STREQ(content[0]["type"].as<const char*>(), "text");
    // The inner text should be parseable JSON containing firmware_version
    const char* innerText = content[0]["text"].as<const char*>();
    ASSERT_NE(innerText, nullptr);
    JsonDocument innerDoc;
    ASSERT_FALSE(deserializeJson(innerDoc, innerText));
    EXPECT_STREQ(innerDoc["firmware_version"].as<const char*>(), APP_FOUNDATION_VERSION);
}

/** Invalid JSON input must return parse error code -32700. */
TEST(McpServerTest, InvalidJsonReturnsParseError) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        "this is not json at all{{{");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_TRUE(doc["error"].is<JsonObject>());
    EXPECT_EQ(doc["error"]["code"].as<int>(), -32700);
}

/** An unknown method must return method-not-found error code -32601. */
TEST(McpServerTest, UnknownMethodReturnsError) {
    McpServer server;
    std::vector<IMcpContributor*> contributors;
    server.begin(contributors, nullptr);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":6,"method":"bogus/method"})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_EQ(doc["id"].as<int>(), 6);
    EXPECT_TRUE(doc["error"].is<JsonObject>());
    EXPECT_EQ(doc["error"]["code"].as<int>(), -32601);
}

/** A contributor-registered tool must be callable through the server dispatch. */
TEST(McpServerTest, ContributorToolCallable) {
    // Create a mock contributor that registers a "echo_test" tool
    class EchoContributor : public IMcpContributor {
    public:
        void registerMcpTools(McpToolRegistry& registry) override {
            registry.addTool("echo_test", "Echo test tool",
                [](JsonObject& params, JsonObject& result) {
                    (void)params;
                    result["echo"] = "hello";
                }
            );
        }
    };

    EchoContributor contributor;
    std::vector<IMcpContributor*> contributors = { &contributor };

    McpServer server;
    server.begin(contributors, nullptr);

    // Verify the tool exists in the registry
    EXPECT_NE(server.getRegistry().findTool("echo_test"), nullptr);

    // Call the tool through JSON-RPC dispatch
    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"echo_test"}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err);

    EXPECT_EQ(doc["id"].as<int>(), 7);
    EXPECT_FALSE(doc["result"]["isError"].as<bool>());
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
