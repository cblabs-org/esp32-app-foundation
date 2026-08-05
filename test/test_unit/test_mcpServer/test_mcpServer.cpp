/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_mcpServer/test_mcpServer.cpp
 * Description: GoogleTest unit-test suite for the MCP server module.
 *              Covers MCPToolRegistry (add/find/size), mcpBuiltinTools
 *              (registration count, tool existence, handler execution),
 *              MCPServer JSON-RPC dispatch (initialize, ping, tools/list,
 *              tools/call, parse error, method not found), and Application
 *              integration (registerMcpContributor pre/post lock, null guard).
 *
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required). native builds is defined by the [env:native]
 *              environment in platformio.ini.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "MCPToolRegistry.hpp"
#include "MCPServer.hpp"
#include "MCPBuiltinTools.hpp"
#include "interfaces/IMCPContributor.hpp"

// =============================================================================
// Test Group 1: MCPToolRegistry — Core registry mechanics
// =============================================================================

/** A freshly constructed registry must report size 0. */
TEST(McpToolRegistryTest, StartsEmpty) {
    MCPToolRegistry registry;
    EXPECT_EQ(registry.size(), 0u);
}

/** Adding one tool must increment the size to 1. */
TEST(McpToolRegistryTest, AddToolIncreasesSize) {
    MCPToolRegistry registry;
    registry.addTool("test_tool", "A test tool",
        [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
            (void)params;
            result["ok"] = true;
        }
    );
    EXPECT_EQ(registry.size(), 1u);
}

/** A registered tool must be findable by its exact name. */
TEST(McpToolRegistryTest, FindRegisteredTool) {
    MCPToolRegistry registry;
    registry.addTool("my_tool", "My tool",
        [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
            (void)params;
            result["ok"] = true;
        }
    );
    const MCPToolEntry* found = registry.findTool("my_tool");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "my_tool");
}

/** Searching for an unregistered tool name must return nullptr. */
TEST(McpToolRegistryTest, FindUnknownToolReturnsNull) {
    MCPToolRegistry registry;
    EXPECT_EQ(registry.findTool("bogus"), nullptr);
}

/** A tool's handler must execute correctly when invoked directly. */
TEST(McpToolRegistryTest, ToolHandlerExecutes) {
    MCPToolRegistry registry;

    bool handlerCalled = false;
    registry.addTool("flag_tool", "Sets a flag",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            (void)params;
            *static_cast<bool*>(ctx) = true;
            result["executed"] = true;
        },
        &handlerCalled
    );

    const MCPToolEntry* tool = registry.findTool("flag_tool");
    ASSERT_NE(tool, nullptr);

    // Invoke the handler directly
    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(tool->context, args, resultObj);

    EXPECT_TRUE(handlerCalled);
    EXPECT_TRUE(resultObj["executed"].as<bool>());
}

/** Adding multiple tools must make all of them findable. */
TEST(McpToolRegistryTest, MultipleToolsRegistered) {
    MCPToolRegistry registry;
    auto noop = [](void* /*ctx*/, JsonObject& p, JsonObject& r) { (void)p; (void)r; };

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
    MCPToolRegistry registry;
    auto noop = [](void* /*ctx*/, JsonObject& p, JsonObject& r) { (void)p; (void)r; };

    registry.addTool("x", "X", noop);
    registry.addTool("y", "Y", noop);

    EXPECT_EQ(registry.getTools().size(), registry.size());
}

// =============================================================================
// Test Group 2: MCPBuiltinTools — Built-in tool registration and execution
// =============================================================================

/** registerAll() with null configManager must register at least 7 tools. */
TEST(McpBuiltinToolsTest, RegistersExpectedToolCount) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_GE(registry.size(), 7u);
}

/** system_info tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, SystemInfoToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("system_info"), nullptr);
}

/** get_config tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, GetConfigToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("get_config"), nullptr);
}

/** set_config tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, SetConfigToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("set_config"), nullptr);
}

/** reboot tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, RebootToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("reboot"), nullptr);
}

/** get_logs tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, GetLogsToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("get_logs"), nullptr);
}

/** list_tools tool must be findable after registerAll(). */
TEST(McpBuiltinToolsTest, ListToolsToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("list_tools"), nullptr);
}

/** capture_display tool must be findable after registerAll() even if display is null. */
TEST(McpBuiltinToolsTest, CaptureDisplayToolExists) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);
    EXPECT_NE(registry.findTool("capture_display"), nullptr);
}

/** system_info handler must return firmware_version in its result. */
TEST(McpBuiltinToolsTest, SystemInfoReturnsData) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);

    const MCPToolEntry* tool = registry.findTool("system_info");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(tool->context, args, resultObj);

    // Must contain firmware_version (the BUILD_VERSION macro)
    EXPECT_TRUE(resultObj["firmware_version"].is<const char*>());
    EXPECT_STREQ(resultObj["firmware_version"].as<const char*>(), BUILD_VERSION);
}

/** get_config with null configManager must return an error in result. */
TEST(McpBuiltinToolsTest, GetConfigWithoutManager) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);

    const MCPToolEntry* tool = registry.findTool("get_config");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(tool->context, args, resultObj);

    EXPECT_TRUE(resultObj["error"].is<const char*>());
    EXPECT_TRUE(resultObj["isError"].as<bool>());
}

/** capture_display with null display must return an error in result. */
TEST(McpBuiltinToolsTest, CaptureDisplayWithoutDisplay) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr); // No display info passed

    const MCPToolEntry* tool = registry.findTool("capture_display");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(tool->context, args, resultObj);

    EXPECT_TRUE(resultObj["error"].is<const char*>());
    EXPECT_STREQ(resultObj["error"].as<const char*>(), "No display registered");
    EXPECT_TRUE(resultObj["isError"].as<bool>());
}

/** capture_display with a valid display buffer must return base64 data. */
TEST(McpBuiltinToolsTest, CaptureDisplayWithDisplay) {
    MCPToolRegistry registry;

    // Create a small synthetic framebuffer (4 bytes)
    uint8_t buffer[] = { 0xAA, 0xBB, 0xCC, 0xDD };
    DisplayInfo display;
    display.framebuffer     = buffer;
    display.framebufferSize = sizeof(buffer);
    display.width           = 16;
    display.height          = 2;

    mcpBuiltinTools::registerAll(registry, nullptr, &display);

    const MCPToolEntry* tool = registry.findTool("capture_display");
    ASSERT_NE(tool, nullptr);

    JsonDocument argsDoc;
    JsonObject args = argsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject resultObj = resultDoc.to<JsonObject>();
    tool->handler(tool->context, args, resultObj);

    // Must contain format, width, height, and data fields
    EXPECT_STREQ(resultObj["format"].as<const char*>(), "u8g2-tile");
    EXPECT_EQ(resultObj["width"].as<int>(), 16);
    EXPECT_EQ(resultObj["height"].as<int>(), 2);
    EXPECT_TRUE(resultObj["data"].is<const char*>());
    // Base64 of 4 bytes should be 8 chars (e.g. "qrvM3Q==")
    EXPECT_GT(strlen(resultObj["data"].as<const char*>()), 0u);
}

// =============================================================================
// Test Group 3: MCPServer — JSON-RPC dispatch via processLineForTest()
// =============================================================================

/**
 * @brief Helper: Feed a JSON-RPC request to the server and capture the response.
 *
 * Clears the MockSerial output buffer, processes the line via the native
 * test harness, and returns the full serialised response string.
 */
static std::string processAndCapture(MCPServer& server, const char* jsonLine) {
    Serial.flush(); // Clear any prior output
    server.processLineForTest(jsonLine);
    return Serial.flush();
}

/**
 * @brief Test helper — set the server's runtime readOnly toggle via applyConfig().
 *
 * mcpReadOnly is no longer seeded from FrameworkConfig (WO2a moved it to be a
 * module-owned, persisted runtime toggle with a secure-by-default factory
 * value set in resetConfig()). Tests drive it directly via the IConfigurable
 * gateway, mirroring how ConfigManager would apply a persisted value.
 */
static void setReadOnly(MCPServer& server, bool readOnly) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["readOnly"] = readOnly;
    server.applyConfig(section);
}

/** initialize must return protocolVersion in the result. */
TEST(McpServerTest, InitializeReturnsCapabilities) {
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    EXPECT_STREQ(innerDoc["firmware_version"].as<const char*>(), BUILD_VERSION);
}

/** Invalid JSON input must return parse error code -32700. */
TEST(McpServerTest, InvalidJsonReturnsParseError) {
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    MCPServer server;
    std::vector<IMCPContributor*> contributors;
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
    class EchoContributor : public IMCPContributor {
    public:
        void registerMCPTools(MCPToolRegistry& registry) override {
            registry.addTool("echo_test", "Echo test tool",
                [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
                    (void)params;
                    result["echo"] = "hello";
                }
            );
        }
    };

    EchoContributor contributor;
    std::vector<IMCPContributor*> contributors = { &contributor };

    MCPServer server;
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

/**
 * Regression test for Issue #13: tool arguments must pass through the JSON-RPC
 * dispatch path to the handler.
 *
 * Before the fix, `_handleToolsCall` received `const JsonDocument& req` and
 * checked `req["params"]["arguments"].is<JsonObject>()`.  Because the subscript
 * on a const document yields a `JsonVariantConst`, ArduinoJson 7's
 * `Converter<JsonObject>::checkJson()` unconditionally returns false — the
 * arguments object was never copied, so every handler received an empty
 * `JsonObject`.  The fix changes the check to `is<JsonObjectConst>()`.
 *
 * This test will FAIL without the fix and pass with it.
 */
TEST(McpServerTest, ToolCallArgumentsReachHandler) {
    // Register a tool that reads a specific argument and echoes it into result
    class ArgEchoContributor : public IMCPContributor {
    public:
        void registerMCPTools(MCPToolRegistry& registry) override {
            registry.addTool("arg_echo", "Echoes the 'value' argument back",
                [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
                    // Read the "value" key sent by the client
                    if (params["value"].is<int>()) {
                        result["received"] = params["value"].as<int>();
                    } else {
                        // Signal that the argument was NOT received
                        result["received"]  = -1;
                        result["isError"]   = true;
                    }
                }
            );
        }
    };

    ArgEchoContributor contributor;
    std::vector<IMCPContributor*> contributors = { &contributor };

    MCPServer server;
    server.begin(contributors, nullptr);

    // Dispatch with a populated arguments object
    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":42,"method":"tools/call",)"
        R"("params":{"name":"arg_echo","arguments":{"value":99}}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err) << "Response JSON parse error: " << err.c_str();

    EXPECT_EQ(doc["id"].as<int>(), 42);

    // Parse the inner text payload
    const char* innerText = doc["result"]["content"][0]["text"].as<const char*>();
    ASSERT_NE(innerText, nullptr) << "Expected content[0].text in response";

    JsonDocument inner;
    ASSERT_FALSE(deserializeJson(inner, innerText))
        << "Inner result JSON parse error";

    // The handler must have received value=99; -1 means args were dropped
    EXPECT_EQ(inner["received"].as<int>(), 99)
        << "Handler received wrong value — arguments were likely dropped "
           "(regression: Issue #13, is<JsonObjectConst>() fix)";
}

// =============================================================================
// Test Group 4: Mutation Tagging & Security
// =============================================================================

/** set_config and reboot must have mutatesState == true; read-only tools must be false. */
TEST(McpSecurityTest, MutatesStateTaggedCorrectly) {
    MCPToolRegistry registry;
    mcpBuiltinTools::registerAll(registry, nullptr);

    // Mutating tools
    const MCPToolEntry* setConfig = registry.findTool("set_config");
    ASSERT_NE(setConfig, nullptr);
    EXPECT_TRUE(setConfig->mutatesState) << "set_config must be tagged as mutating";

    const MCPToolEntry* reboot = registry.findTool("reboot");
    ASSERT_NE(reboot, nullptr);
    EXPECT_TRUE(reboot->mutatesState) << "reboot must be tagged as mutating";

    // Read-only tools
    const MCPToolEntry* sysInfo = registry.findTool("system_info");
    ASSERT_NE(sysInfo, nullptr);
    EXPECT_FALSE(sysInfo->mutatesState) << "system_info must NOT be tagged as mutating";

    const MCPToolEntry* getConfig = registry.findTool("get_config");
    ASSERT_NE(getConfig, nullptr);
    EXPECT_FALSE(getConfig->mutatesState) << "get_config must NOT be tagged as mutating";

    const MCPToolEntry* getLogs = registry.findTool("get_logs");
    ASSERT_NE(getLogs, nullptr);
    EXPECT_FALSE(getLogs->mutatesState) << "get_logs must NOT be tagged as mutating";
}

/** In read-only mode, calling a mutating tool must return error -32000. */
TEST(McpSecurityTest, ReadOnlyBlocksMutatingTool) {
    std::vector<IMCPContributor*> contributors;

    MCPServer server;
    server.begin(contributors, nullptr);
    setReadOnly(server, true);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":10,"method":"tools/call","params":{"name":"reboot"}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err);

    EXPECT_EQ(doc["id"].as<int>(), 10);
    EXPECT_TRUE(doc["error"].is<JsonObject>()) << "Expected JSON-RPC error for mutating tool in read-only mode";
    EXPECT_EQ(doc["error"]["code"].as<int>(), -32000);
}

/** In read-only mode, calling a non-mutating tool must succeed. */
TEST(McpSecurityTest, ReadOnlyAllowsReadTool) {
    std::vector<IMCPContributor*> contributors;

    MCPServer server;
    server.begin(contributors, nullptr);
    setReadOnly(server, true);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":11,"method":"tools/call","params":{"name":"system_info"}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err);

    EXPECT_EQ(doc["id"].as<int>(), 11);
    EXPECT_FALSE(doc["error"].is<JsonVariant>()) << "Read-only tool should succeed in read-only mode";
}

/** In read-write mode, calling a mutating tool must succeed. */
TEST(McpSecurityTest, ReadWriteAllowsMutatingTool) {
    std::vector<IMCPContributor*> contributors;

    MCPServer server;
    server.begin(contributors, nullptr);
    setReadOnly(server, false);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":12,"method":"tools/call","params":{"name":"reboot"}})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err);

    EXPECT_EQ(doc["id"].as<int>(), 12);
    EXPECT_FALSE(doc["error"].is<JsonVariant>()) << "Mutating tool should succeed in read-write mode";
}

/** tools/list response must include mutatesState for each tool. */
TEST(McpSecurityTest, ToolsListIncludesMutatesState) {
    std::vector<IMCPContributor*> contributors;

    MCPServer server;
    server.begin(contributors, nullptr);
    setReadOnly(server, true);

    std::string response = processAndCapture(server,
        R"({"jsonrpc":"2.0","id":13,"method":"tools/list"})");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    ASSERT_FALSE(err);

    // Find the reboot tool and verify mutatesState is present and true
    JsonArray tools = doc["result"]["tools"].as<JsonArray>();
    bool found = false;
    for (JsonObject tool : tools) {
        if (strcmp(tool["name"].as<const char*>(), "reboot") == 0) {
            found = true;
            EXPECT_TRUE(tool["mutatesState"].as<bool>()) << "reboot should have mutatesState = true in tools/list";
            break;
        }
    }
    EXPECT_TRUE(found) << "reboot tool must appear in tools/list";
}

// =============================================================================
// Test Group 5: IConfigurable Persistence
// =============================================================================

/** applyConfig/serializeConfig round-trip preserves enabled and readOnly state. */
TEST(McpConfigTest, ConfigPersistenceRoundTrip) {
    std::vector<IMCPContributor*> contributors;
    MCPServer server;
    server.begin(contributors, nullptr);

    // Server starts enabled by default
    EXPECT_TRUE(server.isEnabled());

    // Serialize initial state
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    server.serializeConfig(section);
    EXPECT_TRUE(section["enabled"].as<bool>());

    // Simulate persisted config with enabled=false
    section["enabled"] = false;
    server.applyConfig(section);
    EXPECT_FALSE(server.isEnabled());

    // Verify re-serialization reflects the change
    JsonDocument verifyDoc;
    JsonObject verifySection = verifyDoc.to<JsonObject>();
    server.serializeConfig(verifySection);
    EXPECT_FALSE(verifySection["enabled"].as<bool>());
}

/** resetConfig() restores enabled=true and readOnly=true defaults. */
TEST(McpConfigTest, ResetRestoresDefaults) {
    std::vector<IMCPContributor*> contributors;
    MCPServer server;
    server.begin(contributors, nullptr);

    // Force non-default state
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    section["enabled"] = false;
    section["readOnly"] = false;
    server.applyConfig(section);
    EXPECT_FALSE(server.isEnabled());

    // Reset and verify defaults
    server.resetConfig();
    EXPECT_TRUE(server.isEnabled());

    // Verify via serialization
    JsonDocument verifyDoc;
    JsonObject verifySection = verifyDoc.to<JsonObject>();
    server.serializeConfig(verifySection);
    EXPECT_TRUE(verifySection["enabled"].as<bool>());
    EXPECT_TRUE(verifySection["readOnly"].as<bool>());
}

/** getConfigNamespace() must return "mcp". */
TEST(McpConfigTest, NamespaceIsMcp) {
    MCPServer server;
    EXPECT_STREQ(server.getConfigNamespace(), "mcp");
}

/** getSchemaVersion() must return 1. */
TEST(McpConfigTest, SchemaVersionIsOne) {
    MCPServer server;
    EXPECT_EQ(server.getSchemaVersion(), 1);
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
