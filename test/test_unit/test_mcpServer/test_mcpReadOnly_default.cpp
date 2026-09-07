/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * SPDX-License-Identifier: Apache-2.0
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
