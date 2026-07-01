/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_ota_fail_closed/test_ota_fail_closed.cpp
 * Description: GoogleTest suite validating the fail-closed OTA security model.
 *              Tests the OTAError enum contract (integer values, string
 *              coverage, distinctness) and a pure-logic helper function
 *              shouldAbortOnMissingSig() that encapsulates the abort decision
 *              independently of hardware-guarded code paths.
 *
 * Exported Functions/Classes:
 * - shouldAbortOnMissingSig(): Testable encapsulation of fail-closed logic
 * - OTAFailClosedTest: GoogleTest fixture for fail-closed OTA verification
 *   - SignatureFetchFailed_HasExpectedValue
 *   - SignatureFetchFailed_HasNonNullString
 *   - SignatureVerifyFailed_IsDistinct
 *   - NativeStub_ReturnsHttpConnectionFailed
 *   - ShouldAbort_VerifierSet_NoSig
 *   - ShouldAbort_VerifierSet_ZeroLen
 *   - ShouldNotAbort_NoVerifier
 *   - ShouldNotAbort_VerifierSet_SigPresent
 */

#include <gtest/gtest.h>
#include "OTADownloadEngine.hpp"
#include "OTAError.hpp"

// =============================================================================
// Testable helper — encapsulates the fail-closed abort decision
// =============================================================================

/**
 * @brief Pure-logic encapsulation of the fail-closed signature check.
 *
 * Mirrors the conditional logic in OTADownloadEngine::downloadAndFlash()
 * step 10 and OTAUpdateManager::performUpdateTask() sig-URL checks.
 * Returns true when the OTA pipeline MUST abort (verifier is configured
 * but no valid signature data is available).
 *
 * @param verifierSet  True if an IOTAVerifier was injected via setVerifier().
 * @param sigPresent   True if sigBuf is non-null.
 * @param sigLen       Length of the signature data.
 * @return true if the pipeline should abort due to missing signature.
 */
bool shouldAbortOnMissingSig(bool verifierSet, bool sigPresent, size_t sigLen) {
    if (!verifierSet) return false;           // No verifier → dev mode, no abort
    if (!sigPresent || sigLen == 0) return true;  // Verifier set but sig missing → ABORT
    return false;                             // Verifier set + sig present → proceed
}

// =============================================================================
// Test fixture
// =============================================================================

/**
 * @brief GoogleTest fixture providing a fresh OTADownloadEngine per test.
 */
class OTAFailClosedTest : public ::testing::Test {
protected:
    OTADownloadEngine engine;
};

// =============================================================================
// Tests: OTAError enum contract
// =============================================================================

/**
 * @brief Verify OTAError::SignatureFetchFailed has the expected integer value (-10).
 */
TEST_F(OTAFailClosedTest, SignatureFetchFailed_HasExpectedValue) {
    EXPECT_EQ(static_cast<int>(OTAError::SignatureFetchFailed), -10);
}

/**
 * @brief Verify otaErrorToString(SignatureFetchFailed) returns a non-null string.
 */
TEST_F(OTAFailClosedTest, SignatureFetchFailed_HasNonNullString) {
    const char* str = otaErrorToString(OTAError::SignatureFetchFailed);
    ASSERT_NE(str, nullptr);
    EXPECT_GT(strlen(str), 0u);
}

/**
 * @brief Verify SignatureVerifyFailed is distinct from SignatureFetchFailed.
 */
TEST_F(OTAFailClosedTest, SignatureVerifyFailed_IsDistinct) {
    EXPECT_NE(OTAError::SignatureVerifyFailed, OTAError::SignatureFetchFailed);
    EXPECT_EQ(static_cast<int>(OTAError::SignatureVerifyFailed), -11);
    EXPECT_EQ(static_cast<int>(OTAError::SignatureFetchFailed), -10);
}

// NativeStub_ReturnsHttpConnectionFailed test was removed as part of PR-41
// because the native stub guards were removed, so the real implementation now runs.

// =============================================================================
// Tests: shouldAbortOnMissingSig() helper
// =============================================================================

/**
 * @brief When verifier is set but no signature buffer — must abort.
 */
TEST_F(OTAFailClosedTest, ShouldAbort_VerifierSet_NoSig) {
    EXPECT_TRUE(shouldAbortOnMissingSig(true, false, 0));
}

/**
 * @brief When verifier is set and sig buffer exists but sigLen is 0 — must abort.
 */
TEST_F(OTAFailClosedTest, ShouldAbort_VerifierSet_ZeroLen) {
    EXPECT_TRUE(shouldAbortOnMissingSig(true, true, 0));
}

/**
 * @brief When no verifier is configured — should NOT abort (dev mode).
 */
TEST_F(OTAFailClosedTest, ShouldNotAbort_NoVerifier) {
    EXPECT_FALSE(shouldAbortOnMissingSig(false, false, 0));
    EXPECT_FALSE(shouldAbortOnMissingSig(false, true, 0));
    EXPECT_FALSE(shouldAbortOnMissingSig(false, true, 256));
}

/**
 * @brief When verifier is set and valid signature is present — should NOT abort.
 */
TEST_F(OTAFailClosedTest, ShouldNotAbort_VerifierSet_SigPresent) {
    EXPECT_FALSE(shouldAbortOnMissingSig(true, true, 256));
    EXPECT_FALSE(shouldAbortOnMissingSig(true, true, 1));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
