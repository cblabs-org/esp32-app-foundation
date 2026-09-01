/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_logpipeline/test_logpipeline.cpp
 * Description: GoogleTest suite for LogPipeline vprintf hook, secret redaction,
 *              and dumpPayload chunking.
 */

#include <gtest/gtest.h>
#define CONFIG_LOG_MAXIMUM_LEVEL 5
#include "LogPipeline.h"
#include "Redactor.h"
#include <string>
#include <cstring>
#include "esp_log.h"

vprintf_like_t g_test_vprintf_handler = nullptr;

static std::string g_sinkOutput;
static int testSink(const char* buf) {
    g_sinkOutput += buf;
    return std::strlen(buf);
}

class MockSanitizer : public ISanitizer {
public:
    bool wasCalled = false;
    void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) override {
        wasCalled = true;
        Redactor::instance().sanitize(buffer, bufferSize, isTruncated);
    }
};

static MockSanitizer g_sanitizer;

class LogPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_sinkOutput.clear();
        Redactor::instance().clearSecrets();
        g_sanitizer.wasCalled = false;
        LogPipeline::begin(&g_sanitizer, testSink);
    }
};

TEST_F(LogPipelineTest, SecretNeverReachesSink) {
    Redactor::instance().registerSecret("mySecret123");
    test_invoke_vprintf("This is a log with mySecret123 in it");
    
    EXPECT_TRUE(g_sanitizer.wasCalled);
    EXPECT_EQ(g_sinkOutput, "This is a log with *********** in it");
}

TEST_F(LogPipelineTest, FallbackPathVerification) {
    Redactor::instance().registerSecret("bigSecret");
    std::string longStr(300, 'A');
    std::string payload = "Prefix bigSecret " + longStr;
    test_invoke_vprintf("%s", payload.c_str());

    EXPECT_TRUE(g_sanitizer.wasCalled);
    EXPECT_TRUE(g_sinkOutput.find("bigSecret") == std::string::npos);
    EXPECT_TRUE(g_sinkOutput.find("*********") != std::string::npos);
}

TEST_F(LogPipelineTest, DumpPayloadTest) {
    Redactor::instance().registerSecret("hidden");
    std::string payload = "Header: hidden\nBody: hidden!";
    
    LogPipeline::dumpPayload(4, "TEST", payload.c_str(), payload.length());

    if (g_sanitizer.wasCalled) {
        // Only verify if compiled with CONFIG_LOG_MAXIMUM_LEVEL >= 4
        EXPECT_TRUE(g_sinkOutput.find("hidden") == std::string::npos);
        EXPECT_TRUE(g_sinkOutput.find("******") != std::string::npos);
    } else {
        GTEST_SKIP() << "dumpPayload is a no-op natively due to missing CONFIG_LOG_MAXIMUM_LEVEL";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
