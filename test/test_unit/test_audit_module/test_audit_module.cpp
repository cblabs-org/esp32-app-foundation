/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_audit_module/test_audit_module.cpp
 * Description: GoogleTest suite for AuditModule core engine (JSONL formatting,
 *              token-bucket rate limiting, truncation, redaction, and diagnostics).
 */

#include <gtest/gtest.h>
#include "AuditModule.hpp"
#include "Redactor.h"
#include <cstring>
#include <string>

class AuditModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        AuditModule::instance().clearAll();
    }
};

TEST_F(AuditModuleTest, FormatRecord_FullFields) {
    AuditRecord record{};
    record.t_ms = 12345;
    record.ts = 1717000000;
    record.seq = 0;
    strncpy(record.source, "mcp", sizeof(record.source));
    strncpy(record.action, "set_temp", sizeof(record.action));
    strncpy(record.targetId, "zone_1", sizeof(record.targetId));
    strncpy(record.details, "value=22", sizeof(record.details));

    char buffer[512];
    size_t len = AuditModule::formatRecord(record, 0xa1b2c3d4, "v0.0.12-dev.54", buffer, sizeof(buffer));

    EXPECT_GT(len, 0u);
    const char* expected = "{\"t_ms\":12345,\"ts\":1717000000,\"seq\":0,\"boot_id\":\"a1b2c3d4\",\"build\":\"v0.0.12-dev.54\",\"source\":\"mcp\",\"action\":\"set_temp\",\"target_id\":\"zone_1\",\"details\":\"value=22\"}";
    EXPECT_STREQ(buffer, expected);
}

TEST_F(AuditModuleTest, FormatRecord_OmitEmptyTargetId) {
    AuditRecord record{};
    record.t_ms = 100;
    record.ts = 1700000000;
    record.seq = 1;
    strncpy(record.source, "config", sizeof(record.source));
    strncpy(record.action, "save", sizeof(record.action));
    record.targetId[0] = '\0';
    strncpy(record.details, "section=wifi", sizeof(record.details));

    char buffer[512];
    size_t len = AuditModule::formatRecord(record, 0x12345678, "v1.0.0", buffer, sizeof(buffer));

    EXPECT_GT(len, 0u);
    const char* expected = "{\"t_ms\":100,\"ts\":1700000000,\"seq\":1,\"boot_id\":\"12345678\",\"build\":\"v1.0.0\",\"source\":\"config\",\"action\":\"save\",\"details\":\"section=wifi\"}";
    EXPECT_STREQ(buffer, expected);
}

TEST_F(AuditModuleTest, FormatRecord_OmitEmptyDetails) {
    AuditRecord record{};
    record.t_ms = 200;
    record.ts = 1700000001;
    record.seq = 2;
    strncpy(record.source, "auth", sizeof(record.source));
    strncpy(record.action, "login_success", sizeof(record.action));
    strncpy(record.targetId, "admin", sizeof(record.targetId));
    record.details[0] = '\0';

    char buffer[512];
    size_t len = AuditModule::formatRecord(record, 0xdeadbeef, "v1.0.0", buffer, sizeof(buffer));

    EXPECT_GT(len, 0u);
    const char* expected = "{\"t_ms\":200,\"ts\":1700000001,\"seq\":2,\"boot_id\":\"deadbeef\",\"build\":\"v1.0.0\",\"source\":\"auth\",\"action\":\"login_success\",\"target_id\":\"admin\"}";
    EXPECT_STREQ(buffer, expected);
}

TEST_F(AuditModuleTest, FormatRecord_OmitBothOptionalFields) {
    AuditRecord record{};
    record.t_ms = 300;
    record.ts = 0;
    record.seq = 3;
    strncpy(record.source, "system", sizeof(record.source));
    strncpy(record.action, "reboot", sizeof(record.action));
    record.targetId[0] = '\0';
    record.details[0] = '\0';

    char buffer[512];
    size_t len = AuditModule::formatRecord(record, 0x0000abcd, "v2.1.0", buffer, sizeof(buffer));

    EXPECT_GT(len, 0u);
    const char* expected = "{\"t_ms\":300,\"ts\":0,\"seq\":3,\"boot_id\":\"0000abcd\",\"build\":\"v2.1.0\",\"source\":\"system\",\"action\":\"reboot\"}";
    EXPECT_STREQ(buffer, expected);
}

TEST_F(AuditModuleTest, FormatRecord_FallbackBuildVersion) {
    AuditRecord record{};
    record.t_ms = 50;
    record.ts = 12345;
    record.seq = 4;
    strncpy(record.source, "ota", sizeof(record.source));
    strncpy(record.action, "start", sizeof(record.action));

    char buffer[512];
    size_t len = AuditModule::formatRecord(record, 0x00000001, nullptr, buffer, sizeof(buffer));

    EXPECT_GT(len, 0u);
    EXPECT_NE(std::string(buffer).find("\"build\":\"unknown\""), std::string::npos);

    len = AuditModule::formatRecord(record, 0x00000001, "", buffer, sizeof(buffer));
    EXPECT_GT(len, 0u);
    EXPECT_NE(std::string(buffer).find("\"build\":\"unknown\""), std::string::npos);
}

TEST_F(AuditModuleTest, RateLimiter_BurstCapacityAndExhaustion) {
    AuditModule& module = AuditModule::instance();
    module.resetRateLimiter();

    // 120 calls at t=1000 should all be allowed
    for (int i = 0; i < 120; ++i) {
        RateLimitAction action = module.checkRateLimit(1000);
        EXPECT_EQ(action, RateLimitAction::Allow) << "Failed on iteration " << i;
    }

    // Call 121 at same timestamp must be throttled (first time)
    RateLimitAction action121 = module.checkRateLimit(1000);
    EXPECT_EQ(action121, RateLimitAction::ThrottleFirst);

    // Call 122 at same timestamp must be throttled (subsequent)
    RateLimitAction action122 = module.checkRateLimit(1000);
    EXPECT_EQ(action122, RateLimitAction::ThrottleSubsequent);
}

TEST_F(AuditModuleTest, RateLimiter_RefillOverTime) {
    AuditModule& module = AuditModule::instance();
    module.resetRateLimiter();

    // Exhaust tokens at t=1000
    for (int i = 0; i < 120; ++i) {
        module.checkRateLimit(1000);
    }
    EXPECT_EQ(module.checkRateLimit(1000), RateLimitAction::ThrottleFirst);

    // 500 ms later: 120 / 60000 * 500 = 1.0 token refilled
    RateLimitAction actionAfter500ms = module.checkRateLimit(1500);
    EXPECT_EQ(actionAfter500ms, RateLimitAction::Allow);

    // Immediate next call at t=1500 is throttled again
    EXPECT_EQ(module.checkRateLimit(1500), RateLimitAction::ThrottleFirst);

    // 60 seconds later: full refill (120 tokens)
    RateLimitAction actionAfter60s = module.checkRateLimit(61500);
    EXPECT_EQ(actionAfter60s, RateLimitAction::Allow);

    // Can make remaining 119 calls
    for (int i = 0; i < 119; ++i) {
        EXPECT_EQ(module.checkRateLimit(61500), RateLimitAction::Allow);
    }

    // Call 121 is throttled
    EXPECT_EQ(module.checkRateLimit(61500), RateLimitAction::ThrottleFirst);
}

TEST_F(AuditModuleTest, FieldTruncationAndNullTermination) {
    AuditRecord record{};
    std::string longSource(60, 's');
    std::string longAction(70, 'a');
    std::string longTarget(70, 't');
    std::string longDetails(200, 'd');

    strncpy(record.source, longSource.c_str(), sizeof(record.source) - 1);
    record.source[sizeof(record.source) - 1] = '\0';

    strncpy(record.action, longAction.c_str(), sizeof(record.action) - 1);
    record.action[sizeof(record.action) - 1] = '\0';

    strncpy(record.targetId, longTarget.c_str(), sizeof(record.targetId) - 1);
    record.targetId[sizeof(record.targetId) - 1] = '\0';

    strncpy(record.details, longDetails.c_str(), sizeof(record.details) - 1);
    record.details[sizeof(record.details) - 1] = '\0';

    EXPECT_EQ(strlen(record.source), sizeof(record.source) - 1);
    EXPECT_EQ(strlen(record.action), sizeof(record.action) - 1);
    EXPECT_EQ(strlen(record.targetId), sizeof(record.targetId) - 1);
    EXPECT_EQ(strlen(record.details), sizeof(record.details) - 1);
}

TEST_F(AuditModuleTest, RedactionIntegrationInDetails) {
    Redactor::registerSecret("mySecretToken99");

    AuditRecord record{};
    record.t_ms = 500;
    record.ts = 1718000000;
    record.seq = 10;
    strncpy(record.source, "mcp", sizeof(record.source));
    strncpy(record.action, "tool_exec", sizeof(record.action));
    strncpy(record.details, "token=mySecretToken99,ok=1", sizeof(record.details));

    Redactor::redact(record.details, sizeof(record.details));
    EXPECT_STREQ(record.details, "token=***************,ok=1");

    char buffer[512];
    AuditModule::formatRecord(record, 0x11223344, "v1.0.0", buffer, sizeof(buffer));
    EXPECT_EQ(std::string(buffer).find("mySecretToken99"), std::string::npos);
    EXPECT_NE(std::string(buffer).find("***************"), std::string::npos);
}

TEST_F(AuditModuleTest, ClearAllResetsCounters) {
    AuditModule& module = AuditModule::instance();
    module.clearAll();

    EXPECT_EQ(module.recordCount(), 0u);
    EXPECT_EQ(module.dropCount(), 0u);
    EXPECT_EQ(module.throttleCount(), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
