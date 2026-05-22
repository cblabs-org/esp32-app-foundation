/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_ota_verifier/test_ota_verifier.cpp
 * Description: Native unit tests for the IOtaVerifier interface contract.
 *              Exercises the MbedTLSSignatureVerifier stub implementation
 *              under NATIVE_BUILD to validate defensive guards: null key
 *              rejection, empty key rejection, verify-without-begin, and
 *              update-without-begin safety.
 *
 * Exported Functions/Classes:
 * - OtaVerifierTest: GoogleTest fixture for IOtaVerifier contract tests
 *   - InterfaceContract_BeginWithNullKey_ReturnsFalse
 *   - InterfaceContract_VerifyWithoutBegin_ReturnsFalse
 *   - InterfaceContract_UpdateWithoutBegin_NoOp
 *   - InterfaceContract_BeginWithEmptyKey_ReturnsFalse
 */

#include <gtest/gtest.h>
#include "MbedTLSSignatureVerifier.hpp"

/**
 * @brief Test fixture for IOtaVerifier interface contract tests.
 *
 * Each test creates a fresh MbedTLSSignatureVerifier instance to ensure
 * clean state isolation. Under NATIVE_BUILD, the stub implementation
 * returns false from begin() for any key (no crypto available), which
 * is the expected behaviour for these contract tests.
 */
class OtaVerifierTest : public ::testing::Test {
protected:
    MbedTLSSignatureVerifier verifier;
};

/**
 * @brief Calling begin(nullptr) must return false without crashing.
 */
TEST_F(OtaVerifierTest, InterfaceContract_BeginWithNullKey_ReturnsFalse) {
    EXPECT_FALSE(verifier.begin(nullptr));
}

/**
 * @brief Calling verify() without a prior successful begin() must return false.
 */
TEST_F(OtaVerifierTest, InterfaceContract_VerifyWithoutBegin_ReturnsFalse) {
    const uint8_t dummySig[] = {0x00, 0x01, 0x02, 0x03};
    EXPECT_FALSE(verifier.verify(dummySig, sizeof(dummySig)));
}

/**
 * @brief Calling update() without a prior successful begin() must not crash.
 *
 * This is a no-op safety test — the method should silently discard data
 * when the verifier has not been initialized.
 */
TEST_F(OtaVerifierTest, InterfaceContract_UpdateWithoutBegin_NoOp) {
    const uint8_t dummyData[] = {0xDE, 0xAD, 0xBE, 0xEF};
    // Must not crash or throw — no assertion on return value since update() is void
    verifier.update(dummyData, sizeof(dummyData));
}

/**
 * @brief Calling begin("") with an empty string must return false.
 */
TEST_F(OtaVerifierTest, InterfaceContract_BeginWithEmptyKey_ReturnsFalse) {
    EXPECT_FALSE(verifier.begin(""));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
