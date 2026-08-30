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
#include "Audit.h"
#include "Redactor.h"
#include "MCPToolRegistry.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include <PsychicHttp.h>
#include "interfaces/ITabRegistry.hpp"
#include <cstring>
#include <string>

class AuditModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        AuditModule::instance().clearAll();
        AuditModule::instance().resetConfig();
    }
    void callReadAuditFile(AuditModule& m, uint8_t fileIndex, size_t offset, size_t maxLines,
                           String& output, size_t& linesRead, bool& truncated, size_t& nextOffset) {
        m.readAuditFile(fileIndex, offset, maxLines, output, linesRead, truncated, nextOffset);
    }
    void setStarted(AuditModule& m, bool started) {
        m._started = started;
    }
    void setEnabled(AuditModule& m, bool enabled) {
        m._enabled = enabled;
    }
    void setDropCount(AuditModule& m, size_t drops) {
        m._dropCount.store(drops);
    }
    void setThrottleCount(AuditModule& m, size_t throttles) {
        m._throttleCount.store(throttles);
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

TEST_F(AuditModuleTest, ReadAuditFile_PaginationAndOffset) {
    AuditModule& module = AuditModule::instance();
    module.clearAll();

    // Write 3 records to file 0
    char path0[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AuditModule::AUDIT_FILE_PREFIX);
    FILE* f = fopen(path0, "wb");
    ASSERT_NE(f, nullptr);
    fputs("{\"seq\":0,\"action\":\"first\"}\n", f);
    fputs("{\"seq\":1,\"action\":\"second\"}\n", f);
    fputs("{\"seq\":2,\"action\":\"third\"}\n", f);
    fclose(f);

    String output;
    size_t linesRead = 0;
    bool truncated = false;
    size_t nextOffset = 0;

    // Page 1: maxLines = 2
    callReadAuditFile(module, 0, 0, 2, output, linesRead, truncated, nextOffset);
    EXPECT_EQ(linesRead, 2u);
    EXPECT_TRUE(truncated);
    EXPECT_GT(nextOffset, 0u);
    EXPECT_NE(std::string(output.c_str()).find("first"), std::string::npos);
    EXPECT_NE(std::string(output.c_str()).find("second"), std::string::npos);
    EXPECT_EQ(std::string(output.c_str()).find("third"), std::string::npos);

    // Page 2: start from nextOffset, maxLines = 2
    String outputPage2;
    size_t linesReadPage2 = 0;
    bool truncatedPage2 = false;
    size_t nextOffsetPage2 = 0;
    callReadAuditFile(module, 0, nextOffset, 2, outputPage2, linesReadPage2, truncatedPage2, nextOffsetPage2);
    EXPECT_EQ(linesReadPage2, 1u);
    EXPECT_FALSE(truncatedPage2);
    EXPECT_NE(std::string(outputPage2.c_str()).find("third"), std::string::npos);
}

TEST_F(AuditModuleTest, ReadAuditFile_DropTrailingIncompleteLine) {
    AuditModule& module = AuditModule::instance();
    module.clearAll();

    // Write 1 complete line and 1 incomplete line (no trailing newline)
    char path0[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AuditModule::AUDIT_FILE_PREFIX);
    FILE* f = fopen(path0, "wb");
    ASSERT_NE(f, nullptr);
    fputs("{\"seq\":0,\"action\":\"valid_complete\"}\n", f);
    fputs("{\"seq\":1,\"action\":\"broken_incomp", f); // Missing newline
    fclose(f);

    String output;
    size_t linesRead = 0;
    bool truncated = false;
    size_t nextOffset = 0;

    callReadAuditFile(module, 0, 0, 10, output, linesRead, truncated, nextOffset);
    EXPECT_EQ(linesRead, 1u);
    EXPECT_FALSE(truncated);
    EXPECT_NE(std::string(output.c_str()).find("valid_complete"), std::string::npos);
    EXPECT_EQ(std::string(output.c_str()).find("broken_incomp"), std::string::npos);
}

TEST_F(AuditModuleTest, ReadAuditFile_NonExistentFile) {
    AuditModule& module = AuditModule::instance();
    module.clearAll();

    String output;
    size_t linesRead = 0;
    bool truncated = false;
    size_t nextOffset = 0;

    // File index 0 (doesn't exist after clearAll)
    callReadAuditFile(module, 0, 0, 10, output, linesRead, truncated, nextOffset);
    EXPECT_EQ(linesRead, 0u);
    EXPECT_FALSE(truncated);
    EXPECT_EQ(output.length(), 0u);

    // Invalid file index (> 1)
    callReadAuditFile(module, 5, 0, 10, output, linesRead, truncated, nextOffset);
    EXPECT_EQ(linesRead, 0u);
    EXPECT_FALSE(truncated);
}

TEST_F(AuditModuleTest, MCPTools_RegistrationAndGetAuditLog) {
    AuditModule& module = AuditModule::instance();
    module.clearAll();

    MCPToolRegistry registry;
    module.registerMCPTools(registry);

    const MCPToolEntry* getTool = registry.findTool("get_audit_log");
    ASSERT_NE(getTool, nullptr);
    EXPECT_FALSE(getTool->mutatesState);

    const MCPToolEntry* clearTool = registry.findTool("clear_audit_log");
    ASSERT_NE(clearTool, nullptr);
    EXPECT_TRUE(clearTool->mutatesState);

    // Write 2 test records to file 0
    char path0[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AuditModule::AUDIT_FILE_PREFIX);
    FILE* f = fopen(path0, "wb");
    ASSERT_NE(f, nullptr);
    fputs("{\"t_ms\":100,\"action\":\"login\",\"seq\":0}\n", f);
    fputs("{\"t_ms\":200,\"action\":\"logout\",\"seq\":1}\n", f);
    fclose(f);

    // Call get_audit_log handler
    JsonDocument paramsDoc;
    JsonObject params = paramsDoc.to<JsonObject>();
    params["lines"] = 10;
    params["offset"] = 0;
    params["file"] = 0;

    JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    getTool->handler(getTool->context, params, result);

    EXPECT_EQ(result["lines_returned"].as<size_t>(), 2u);
    EXPECT_FALSE(result["truncated"].as<bool>());
    EXPECT_TRUE(result["records"].is<JsonArray>());
    JsonArray records = result["records"].as<JsonArray>();
    EXPECT_EQ(records.size(), 2u);
    EXPECT_STREQ(records[0]["action"].as<const char*>(), "login");
    EXPECT_STREQ(records[1]["action"].as<const char*>(), "logout");
}

TEST_F(AuditModuleTest, MCPTools_ClearAuditLog) {
    AuditModule& module = AuditModule::instance();
    module.clearAll();

    // Create file
    char path0[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AuditModule::AUDIT_FILE_PREFIX);
    FILE* f = fopen(path0, "wb");
    ASSERT_NE(f, nullptr);
    fputs("{\"seq\":0}\n", f);
    fclose(f);

    MCPToolRegistry registry;
    module.registerMCPTools(registry);
    const MCPToolEntry* clearTool = registry.findTool("clear_audit_log");
    ASSERT_NE(clearTool, nullptr);

    JsonDocument paramsDoc;
    JsonObject params = paramsDoc.to<JsonObject>();
    JsonDocument resultDoc;
    JsonObject result = resultDoc.to<JsonObject>();

    clearTool->handler(clearTool->context, params, result);
    EXPECT_TRUE(result["ok"].as<bool>());
    EXPECT_STREQ(result["message"].as<const char*>(), "Audit log cleared");

    // File should no longer exist
    FILE* check = fopen(path0, "rb");
    EXPECT_EQ(check, nullptr);
}

TEST_F(AuditModuleTest, WebRoutes_RegisterRoutesAndTabs) {
    AuditModule& module = AuditModule::instance();
    PsychicHttpServer server;
    module.registerRoutes(server);

    bool hasAuditGet = false;
    bool hasAuditClearPost = false;
    for (const auto& route : server.routes) {
        if (route.uri == "/api/system/audit" && route.method == HTTP_GET) {
            hasAuditGet = true;
        }
        if (route.uri == "/api/system/audit/clear" && route.method == HTTP_POST) {
            hasAuditClearPost = true;
        }
    }
    EXPECT_TRUE(hasAuditGet);
    EXPECT_TRUE(hasAuditClearPost);

    // Verify registerTabs does not add tabs
    struct MockTabRegistry : public ITabRegistry {
        size_t count = 0;
        using ITabRegistry::addTab;
        void addTab(const char* name, const char* parent, int order, const char* icon, const char* script) override {
            count++;
        }
    } tabReg;

    module.registerTabs(tabReg);
    EXPECT_EQ(tabReg.count, 0u);

    // Verify ApiPrefix and MaxPostBody
    EXPECT_STREQ(module.getApiPrefix(), "/api/system/audit");
    EXPECT_EQ(module.getMaxPostBody(), 4096u);
}

TEST_F(AuditModuleTest, Configurable_NamespaceVersionAndReset) {
    AuditModule& module = AuditModule::instance();
    module.resetConfig();

    EXPECT_STREQ(module.getConfigNamespace(), "audit");
    EXPECT_EQ(module.getSchemaVersion(), 1);

    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    module.serializeConfig(obj);
    EXPECT_TRUE(obj["enabled"].as<bool>());
    EXPECT_EQ(obj["rate_limit"].as<uint32_t>(), AuditModule::AUDIT_RATE_LIMIT_PER_MIN);

    // Apply custom config
    JsonDocument customDoc;
    JsonObject customObj = customDoc.to<JsonObject>();
    customObj["enabled"] = false;
    customObj["rate_limit"] = 60;
    module.applyConfig(customObj);

    EXPECT_FALSE(module.isEnabled());
    EXPECT_EQ(module.configuredRateLimit(), 60u);

    JsonDocument outDoc;
    JsonObject outObj = outDoc.to<JsonObject>();
    module.serializeConfig(outObj);
    EXPECT_FALSE(outObj["enabled"].as<bool>());
    EXPECT_EQ(outObj["rate_limit"].as<uint32_t>(), 60u);

    // Migrate config returns true
    EXPECT_TRUE(module.migrateConfig(outObj, 0));

    // Reset config
    module.resetConfig();
    EXPECT_TRUE(module.isEnabled());
    EXPECT_EQ(module.configuredRateLimit(), AuditModule::AUDIT_RATE_LIMIT_PER_MIN);
}

TEST_F(AuditModuleTest, Configurable_ApplyConfig_StateTransitions) {
    AuditModule& module = AuditModule::instance();
    module.begin();
    module.clearAll();
    module.resetConfig();

    // 1. Transition enabled -> disabled: should write audit_disabled record
    JsonDocument disableDoc;
    JsonObject disableObj = disableDoc.to<JsonObject>();
    disableObj["enabled"] = false;
    module.applyConfig(disableObj);
    EXPECT_FALSE(module.isEnabled());

    // 2. Normal log while disabled is suppressed (returns false, no drop increment)
    EXPECT_FALSE(module.log("app", "user_action"));
    EXPECT_EQ(module.dropCount(), 0u);

    // 3. Transition disabled -> enabled: should write audit_enabled record
    JsonDocument enableDoc;
    JsonObject enableObj = enableDoc.to<JsonObject>();
    enableObj["enabled"] = true;
    module.applyConfig(enableObj);
    EXPECT_TRUE(module.isEnabled());

    // 4. Normal log while enabled succeeds
    EXPECT_TRUE(module.log("app", "user_action_after_enable"));
}

TEST_F(AuditModuleTest, Log_DisabledSuppressionAndInternalBypass) {
    AuditModule& module = AuditModule::instance();
    module.begin();
    module.clearAll();
    module.resetConfig();

    // Disable logging
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["enabled"] = false;
    module.applyConfig(obj);
    EXPECT_FALSE(module.isEnabled());

    // External log is suppressed without drop count increment
    EXPECT_FALSE(module.log("mcp", "tool_call", "device1", "details"));
    EXPECT_EQ(module.dropCount(), 0u);

    // External sync log is also suppressed without drop count increment
    EXPECT_FALSE(module.auditSync("mcp", "tool_call_sync", "device1", "details"));
    EXPECT_EQ(module.dropCount(), 0u);

    // Internal audit record (source == "audit") bypasses enabled check
    EXPECT_TRUE(module.log("audit", "rate_limited", nullptr, "internal test"));
}

TEST_F(AuditModuleTest, PreconditionContributor_RegistrationAndEvaluation) {
    AuditModule& module = AuditModule::instance();
    module.resetConfig();
    module.clearAll();

    struct MockRegistry : public IPreconditionRegistry {
        std::string name;
        PreconditionRole role = PreconditionRole::Startup;
        EvalFn eval;
        DescribeFn describe;

        void add(const char* n,
                 PreconditionRole r,
                 EvalFn e,
                 DescribeFn d = nullptr,
                 const Edge* = nullptr,
                 uint8_t = 0,
                 const DampingPolicy* = nullptr,
                 const RetryPolicy* = nullptr) override {
            name = n ? n : "";
            role = r;
            eval = e;
            describe = d;
        }
    } registry;

    module.registerPreconditions(registry);

    EXPECT_EQ(registry.name, "audit_health");
    EXPECT_EQ(registry.role, PreconditionRole::Operational);
    ASSERT_TRUE(registry.eval != nullptr);
    ASSERT_TRUE(registry.describe != nullptr);

    // Case 1: Not started
    setStarted(module, false);
    PreconditionStatus status = registry.eval();
    EXPECT_FALSE(status.met);
    EXPECT_EQ(status.cause, Cause::InternalError);

    // Case 2: Started and enabled, clean (no drops, no throttles)
    setStarted(module, true);
    setEnabled(module, true);
    setDropCount(module, 0);
    setThrottleCount(module, 0);
    status = registry.eval();
    EXPECT_TRUE(status.met);
    EXPECT_EQ(status.cause, Cause::Met);

    // Case 3: Started but disabled in config
    setEnabled(module, false);
    status = registry.eval();
    EXPECT_FALSE(status.met);
    EXPECT_EQ(status.cause, Cause::Unavailable);

    // Case 4: Started, enabled, but degraded (drops > 0)
    setEnabled(module, true);
    setDropCount(module, 3);
    setThrottleCount(module, 0);
    status = registry.eval();
    EXPECT_TRUE(status.met);
    EXPECT_EQ(status.cause, Cause::FallbackEngaged);

    // Case 5: Started, enabled, but degraded (throttles > 0)
    setDropCount(module, 0);
    setThrottleCount(module, 2);
    status = registry.eval();
    EXPECT_TRUE(status.met);
    EXPECT_EQ(status.cause, Cause::FallbackEngaged);

    // Test describe function
    char buf[128];
    registry.describe(Cause::InternalError, buf, sizeof(buf));
    EXPECT_NE(std::string(buf).find("initialization failed"), std::string::npos);

    registry.describe(Cause::Unavailable, buf, sizeof(buf));
    EXPECT_NE(std::string(buf).find("disabled"), std::string::npos);

    registry.describe(Cause::FallbackEngaged, buf, sizeof(buf));
    EXPECT_NE(std::string(buf).find("degraded"), std::string::npos);

    registry.describe(Cause::Met, buf, sizeof(buf));
    EXPECT_NE(std::string(buf).find("active"), std::string::npos);
}

TEST_F(AuditModuleTest, MacroConvenience_AsyncAndSyncLogging) {
    AuditModule& module = AuditModule::instance();
    module.begin();
    module.clearAll();
    module.resetConfig();

    // Verify AUDIT and AUDIT_SYNC macros work when WITH_AUDIT is active
    AUDIT("auth", "login", "user1", "success");
    AUDIT_SYNC("system", "config_change", "audit", "rate_limit=120");

    EXPECT_EQ(module.dropCount(), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
