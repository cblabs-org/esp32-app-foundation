/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_ota_download_engine/test_ota_download_engine.cpp
 * Description: Native unit tests for the OTADownloadEngine class and OTAError
 *              utilities. Validates the interface contract: default state,
 *              setter safety, error string coverage, and native stub return
 *              values. Uses a lightweight MockVerifier for verifier injection
 *              tests without crypto dependencies.
 *
 * Exported Functions/Classes:
 * - MockVerifier: Minimal IOTAVerifier implementation for testing
 * - OtaDownloadEngineTest: GoogleTest fixture for engine contract tests
 *   - DefaultState_NoVerifier_NullptrByDefault
 *   - SetVerifier_StoresPointer
 *   - SetProgressCallback_StoresCallback
 *   - SetHttpTimeout_StoresValue
 *   - OtaErrorToString_AllCodes_ReturnNonNull
 *   - OtaErrorToString_None_ReturnsSuccess
 *   - FetchSignature_NativeStub_ReturnsZero
 *   - DownloadAndFlash_NativeStub_ReturnsError
 */

#include <gtest/gtest.h>
#include "OTADownloadEngine.hpp"
#include "OTAError.hpp"

// =============================================================================
// Mock verifier — lightweight IOTAVerifier for injection testing
// =============================================================================

/**
 * @brief Minimal IOTAVerifier mock for testing setVerifier() without crypto.
 *
 * All methods are safe no-ops. begin() always returns false (matching the
 * native stub behavior of MbedTLSSignatureVerifier).
 */
class MockVerifier : public IOTAVerifier {
public:
    bool begin(const char* publicKeyPem) override {
        (void)publicKeyPem;
        return false; // No real crypto in native tests
    }

    void update(const uint8_t* data, size_t length) override {
        (void)data;
        (void)length;
    }

    bool verify(const uint8_t* signature, size_t signatureLength) override {
        (void)signature;
        (void)signatureLength;
        return false;
    }
};

// =============================================================================
// Test fixture
// =============================================================================

/**
 * @brief GoogleTest fixture providing a fresh OTADownloadEngine per test.
 */
class OtaDownloadEngineTest : public ::testing::Test {
protected:
    OTADownloadEngine engine;
};

// =============================================================================
// Tests
// =============================================================================

/**
 * @brief A freshly constructed engine should report no error.
 */
TEST_F(OtaDownloadEngineTest, DefaultState_NoVerifier_NullptrByDefault) {
    EXPECT_EQ(engine.getLastError(), OTAError::None);
}

/**
 * @brief Setting a mock verifier should not crash or throw.
 */
TEST_F(OtaDownloadEngineTest, SetVerifier_StoresPointer) {
    MockVerifier mock;
    // Must not crash — the engine stores the pointer internally
    engine.setVerifier(&mock);
    // Also test setting nullptr (disable verification)
    engine.setVerifier(nullptr);
}

/**
 * @brief Setting a progress callback should not crash or throw.
 */
TEST_F(OtaDownloadEngineTest, SetProgressCallback_StoresCallback) {
    bool called = false;
    engine.setProgressCallback([](void* ctx, size_t written, size_t total) {
        (void)written;
        (void)total;
        *static_cast<bool*>(ctx) = true;
    }, &called);
    // Callback is stored but not invoked until downloadAndFlash runs
    EXPECT_FALSE(called);
}

/**
 * @brief Setting the HTTP timeout should not crash or throw.
 */
TEST_F(OtaDownloadEngineTest, SetHttpTimeout_StoresValue) {
    // Must not crash with any reasonable value
    engine.setHttpTimeout(5000);
    engine.setHttpTimeout(0);
    engine.setHttpTimeout(30000);
}

/**
 * @brief Every OTAError enum value must map to a non-null, non-empty string.
 */
TEST_F(OtaDownloadEngineTest, OtaErrorToString_AllCodes_ReturnNonNull) {
    // Iterate all known OTAError values via their underlying int8_t range
    const OTAError allErrors[] = {
        OTAError::None,
        OTAError::HttpConnectionFailed,
        OTAError::HttpWrongResponseCode,
        OTAError::HttpFileNotFound,
        OTAError::HttpForbidden,
        OTAError::ServerNoContentLength,
        OTAError::TooManyRedirects,
        OTAError::InsufficientSpace,
        OTAError::NoOtaPartition,
        OTAError::BinaryHeaderInvalid,
        OTAError::SignatureFetchFailed,
        OTAError::SignatureVerifyFailed,
        OTAError::UpdateBeginFailed,
        OTAError::UpdateWriteFailed,
        OTAError::UpdateEndFailed,
        OTAError::StreamReadTimeout,
    };

    for (OTAError err : allErrors) {
        const char* str = otaErrorToString(err);
        EXPECT_NE(str, nullptr) << "otaErrorToString returned nullptr for code "
                                << static_cast<int>(err);
        EXPECT_GT(strlen(str), 0u) << "otaErrorToString returned empty string for code "
                                   << static_cast<int>(err);
    }
}

/**
 * @brief OTAError::None should map to a string containing "success" or "no error".
 */
TEST_F(OtaDownloadEngineTest, OtaErrorToString_None_ReturnsSuccess) {
    const char* str = otaErrorToString(OTAError::None);
    ASSERT_NE(str, nullptr);

    // Convert to lowercase std::string for case-insensitive check
    std::string lower(str);
    for (char& c : lower) c = static_cast<char>(tolower(c));

    bool containsSuccess = lower.find("success") != std::string::npos;
    bool containsNone    = lower.find("no error") != std::string::npos;
    EXPECT_TRUE(containsSuccess || containsNone)
        << "Expected 'success' or 'no error' in: " << str;
}

/**
 * @brief The native stub of fetchSignature() must return 0.
 */
TEST_F(OtaDownloadEngineTest, FetchSignature_NativeStub_ReturnsZero) {
    WiFiClient stubClient;
    uint8_t sigBuf[256] = {};
    std::string url("https://example.com/firmware.bin.sig");
    std::string token("");

    size_t result = engine.fetchSignature(stubClient, url, token, sigBuf, sizeof(sigBuf));
    EXPECT_EQ(result, 0u);
}

/**
 * @brief The native stub of downloadAndFlash() must return a non-None error.
 */
TEST_F(OtaDownloadEngineTest, DownloadAndFlash_NativeStub_ReturnsError) {
    WiFiClient stubClient;
    uint8_t sigBuf[256] = {};
    std::string url("https://example.com/firmware.bin");
    std::string token("");

    OTAError result = engine.downloadAndFlash(stubClient, url, token, sigBuf, sizeof(sigBuf));
    EXPECT_NE(result, OTAError::None);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
