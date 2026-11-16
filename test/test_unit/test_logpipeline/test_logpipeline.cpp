/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_logpipeline/test_logpipeline.cpp
 * Description: GoogleTest suite for the LogPipeline vprintf hook — secret
 *              redaction, oversize (heap) handling, truncation safety, and
 *              dumpPayload's straddle-free chunking.
 */

#include <gtest/gtest.h>
#include "LogPipeline.h"
#include "Redactor.h"
#include "StructuralScrubber.h"
#include "esp_log.h"
#include <string>
#include <cstring>

static std::string g_sinkOutput;
static int testSink(const char* buf) {
    g_sinkOutput += buf;
    return static_cast<int>(std::strlen(buf));
}

/// Records that sanitize() ran, and that it ran before anything reached the sink.
class MockSanitizer : public ISanitizer {
public:
    bool wasCalled = false;
    bool sawTruncatedFlag = false;
    size_t sinkLenAtFirstCall = SIZE_MAX;

    void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) override {
        if (!wasCalled) sinkLenAtFirstCall = g_sinkOutput.size();
        wasCalled = true;
        if (isTruncated) sawTruncatedFlag = true;
        Redactor::instance().sanitize(buffer, bufferSize, isTruncated);
    }
};

static MockSanitizer g_sanitizer;

class LogPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_sinkOutput.clear();
        Redactor::instance().clearSecrets();
        g_sanitizer = MockSanitizer{};
        LogPipeline::resetForTest();
        LogPipeline::begin(&g_sanitizer, testSink);
    }
};

TEST_F(LogPipelineTest, SecretNeverReachesSink) {
    Redactor::instance().registerSecret("mySecret123");
    test_invoke_vprintf("This is a log with mySecret123 in it");

    EXPECT_TRUE(g_sanitizer.wasCalled);
    EXPECT_EQ(g_sinkOutput, "This is a log with *********** in it");
}

TEST_F(LogPipelineTest, SanitizeRunsBeforeAnythingReachesTheSink) {
    Redactor::instance().registerSecret("mySecret123");
    test_invoke_vprintf("leading mySecret123");
    ASSERT_TRUE(g_sanitizer.wasCalled);
    // Nothing had been emitted at the moment sanitize() was first invoked.
    EXPECT_EQ(g_sanitizer.sinkLenAtFirstCall, 0u);
}

TEST_F(LogPipelineTest, SecretRegisteredAfterInstallIsStillRedacted) {
    // Exercises the lock-free publish path: register happens after begin().
    Redactor::instance().registerSecret("lateBoundSecret");
    test_invoke_vprintf("value=%s", "lateBoundSecret");
    EXPECT_EQ(g_sinkOutput.find("lateBoundSecret"), std::string::npos);
}

TEST_F(LogPipelineTest, OversizeLineUsesHeapBufferAndIsNotTruncated) {
    // Larger than the 256 B static buffer: the heap path must carry it intact.
    const std::string filler(1200, 'A');
    Redactor::instance().registerSecret("bigSecretValue");
    test_invoke_vprintf("%s bigSecretValue %s", filler.c_str(), filler.c_str());

    EXPECT_EQ(g_sinkOutput.find("bigSecretValue"), std::string::npos);
    EXPECT_NE(g_sinkOutput.find("**************"), std::string::npos);
    // Nothing was lost: both fillers survived, so this did not truncate at 256.
    EXPECT_GT(g_sinkOutput.size(), 2400u);
    EXPECT_EQ(LogPipeline::oversizeAllocFailures(), 0u);
}

TEST_F(LogPipelineTest, TruncatedLineDoesNotLeakSecretPrefix) {
    // A secret cut by truncation must not leave a matchable prefix behind.
    // Force truncation by exceeding the hard heap cap.
    Redactor::instance().registerSecret("TAILSECRETVALUE");
    const std::string filler(20000, 'B');
    test_invoke_vprintf("%s%s", filler.c_str(), "TAILSECRETVALUE");
    EXPECT_EQ(g_sinkOutput.find("TAILSEC"), std::string::npos);
}

TEST_F(LogPipelineTest, NullSanitizerDefaultsToRedactorRatherThanFailingOpen) {
    LogPipeline::resetForTest();
    LogPipeline::begin(nullptr, testSink);   // no sanitizer supplied
    Redactor::instance().registerSecret("defaultedSecret");
    g_sinkOutput.clear();
    test_invoke_vprintf("x=%s", "defaultedSecret");
    EXPECT_EQ(g_sinkOutput.find("defaultedSecret"), std::string::npos);
}

// --- dumpPayload -----------------------------------------------------------

TEST_F(LogPipelineTest, DumpPayloadRedactsSimplePayload) {
    Redactor::instance().registerSecret("hidden99");
    const std::string payload = "Header: hidden99\nBody: hidden99!";
    LogPipeline::dumpPayload(ESP_LOG_VERBOSE, "TEST", payload.c_str(), payload.size());

    EXPECT_TRUE(g_sanitizer.wasCalled);
    EXPECT_EQ(g_sinkOutput.find("hidden99"), std::string::npos);
    EXPECT_NE(g_sinkOutput.find("********"), std::string::npos);
}

TEST_F(LogPipelineTest, DumpPayloadDoesNotLeakSecretStraddlingAChunkBoundary) {
    // Regression: chunking each block independently emitted both halves of a
    // secret spanning the boundary, reassembling it in full in the output.
    const char* SECRET = "SUPERSECRET-BEARER-TOKEN-abcdef0123456789";
    Redactor::instance().registerSecret(SECRET);

    for (size_t offset : {200u, 240u, 250u, 255u, 256u, 260u, 500u, 511u, 512u}) {
        g_sinkOutput.clear();
        std::string payload(offset, 'A');
        payload += SECRET;
        payload += std::string(300, 'B');

        LogPipeline::dumpPayload(ESP_LOG_VERBOSE, "T", payload.c_str(), payload.size());

        EXPECT_EQ(g_sinkOutput.find(SECRET), std::string::npos)
            << "whole secret leaked at offset " << offset;
        EXPECT_EQ(g_sinkOutput.find(std::string(SECRET, 12)), std::string::npos)
            << "secret prefix leaked at offset " << offset;
    }
}

TEST_F(LogPipelineTest, DumpPayloadPreservesNonSecretContent) {
    Redactor::instance().registerSecret("zzzsecretzzz");
    std::string payload(700, 'C');
    LogPipeline::dumpPayload(ESP_LOG_VERBOSE, "T", payload.c_str(), payload.size());
    // Every non-secret byte survives — the window must not drop or duplicate data.
    size_t cs = 0;
    for (char c : g_sinkOutput) if (c == 'C') ++cs;
    EXPECT_EQ(cs, 700u);
}

TEST_F(LogPipelineTest, DumpPayloadHandlesEmbeddedNuls) {
    Redactor::instance().registerSecret("afterNulSecret");
    std::string payload = "before";
    payload.push_back('\0');
    payload += "afterNulSecret";
    LogPipeline::dumpPayload(ESP_LOG_VERBOSE, "T", payload.data(), payload.size());
    // The dump must not stop at the NUL, and must still redact what follows it.
    EXPECT_NE(g_sinkOutput.find("before"), std::string::npos);
    EXPECT_EQ(g_sinkOutput.find("afterNulSecret"), std::string::npos);
}

// --- structural scrubbing --------------------------------------------------

TEST_F(LogPipelineTest, StructuralScrubberCatchesUnregisteredBearerToken) {
    // The token was never registered — only structure can catch it.
    LogPipeline::resetForTest();
    LogPipeline::begin(&StructuralScrubber::instance(), testSink);
    g_sinkOutput.clear();
    test_invoke_vprintf("GET /x\r\nAuthorization: Bearer eyJhbGciOiJIUzI1NiJ9.PAYLOAD\r\n");
    EXPECT_EQ(g_sinkOutput.find("eyJhbGciOiJIUzI1NiJ9"), std::string::npos);
    EXPECT_NE(g_sinkOutput.find("Authorization"), std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
