/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_mcpServer/test_mcpReadOnly_default.cpp
 * Description: Unit test verifying that the MCP server's readOnly configuration
 *              defaults to true when no explicit configuration has been applied.
 */

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "MCPServer.hpp"

TEST(MCPServerSecurityTest, McpReadOnlyDefaultsToTrue) {
    MCPServer server;
    
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    
    // We haven't called begin() or applyConfig(), so it's in default state
    server.serializeConfig(obj);
    
    EXPECT_TRUE(obj.containsKey("readOnly"));
    EXPECT_TRUE(obj["readOnly"].as<bool>());
}
