/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_structural_scrubber/test_structural_scrubber.cpp
 * Description: GoogleTest suite for StructuralScrubber — masks sensitive values
 *              by structure (header/JSON/query shape) rather than by value, so
 *              it covers secrets the device never registered.
 */

#include <gtest/gtest.h>
#include "StructuralScrubber.h"
#include <cstring>
#include <string>

static std::string scrub(const std::string& in) {
    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s", in.c_str());
    StructuralScrubber::instance().sanitize(buf, sizeof(buf));
    return std::string(buf);
}

TEST(StructuralScrubberTest, MasksAuthorizationHeaderValue) {
    const auto out = scrub("Authorization: Bearer eyJhbGciOi.SIGNATURE\r\nHost: x\r\n");
    EXPECT_EQ(out.find("eyJhbGciOi"), std::string::npos);
    EXPECT_NE(out.find("Authorization"), std::string::npos);
    EXPECT_NE(out.find("Host: x"), std::string::npos);   // unrelated headers survive
}

TEST(StructuralScrubberTest, MatchesHeaderNameCaseInsensitively) {
    EXPECT_EQ(scrub("authorization: tok_abcdef\r\n").find("tok_abcdef"), std::string::npos);
    EXPECT_EQ(scrub("AUTHORIZATION: tok_abcdef\r\n").find("tok_abcdef"), std::string::npos);
}

TEST(StructuralScrubberTest, CoversProxyAuthorizationViaSubstring) {
    EXPECT_EQ(scrub("Proxy-Authorization: Basic ZGVtbw==\r\n").find("ZGVtbw"), std::string::npos);
}

TEST(StructuralScrubberTest, MasksCookieAndSetCookie) {
    EXPECT_EQ(scrub("Cookie: session=abc123def\r\n").find("abc123def"), std::string::npos);
    EXPECT_EQ(scrub("Set-Cookie: sid=zzz999yyy; Path=/\r\n").find("zzz999yyy"), std::string::npos);
}

TEST(StructuralScrubberTest, MasksApiKeyHeaders) {
    EXPECT_EQ(scrub("X-API-Key: k_liveSecret\r\n").find("k_liveSecret"), std::string::npos);
    EXPECT_EQ(scrub("X-Auth-Token: t_liveSecret\r\n").find("t_liveSecret"), std::string::npos);
}

TEST(StructuralScrubberTest, MasksJsonValues) {
    const auto out = scrub(R"({"user":"bob","password":"hunter2xyz","id":7})");
    EXPECT_EQ(out.find("hunter2xyz"), std::string::npos);
    EXPECT_NE(out.find("bob"), std::string::npos);       // non-sensitive fields survive
}

TEST(StructuralScrubberTest, QuotedJsonKeysDoNotMaskTheWrongSpan) {
    // "token" must not match inside "access_token" and mask from the wrong offset.
    const auto out = scrub(R"({"access_token":"AAABBBCCC","note":"keepme"})");
    EXPECT_EQ(out.find("AAABBBCCC"), std::string::npos);
    EXPECT_NE(out.find("keepme"), std::string::npos);
}

TEST(StructuralScrubberTest, MasksQueryParameters) {
    const auto out = scrub("GET /v1/data?api_key=SECRETKEY123&page=2 HTTP/1.1");
    EXPECT_EQ(out.find("SECRETKEY123"), std::string::npos);
    EXPECT_NE(out.find("page=2"), std::string::npos);    // the next parameter survives
}

TEST(StructuralScrubberTest, HandlesEmptyValueWithoutHanging) {
    // Regression guard: an empty value must still advance the scan cursor.
    EXPECT_NO_FATAL_FAILURE(scrub("Cookie:\r\nCookie:\r\nAuthorization:\r\n"));
}

TEST(StructuralScrubberTest, SafeOnNullEmptyAndUnterminatedBuffers) {
    EXPECT_NO_FATAL_FAILURE(StructuralScrubber::instance().sanitize(nullptr, 0));
    EXPECT_NO_FATAL_FAILURE(StructuralScrubber::instance().sanitize(nullptr, 100));
    char empty[8] = {0};
    EXPECT_NO_FATAL_FAILURE(StructuralScrubber::instance().sanitize(empty, sizeof(empty)));

    // No NUL anywhere: the scan must stay inside bufferSize.
    char unterminated[32];
    std::memset(unterminated, 'X', sizeof(unterminated));
    std::memcpy(unterminated, "Cookie: v", 9);
    EXPECT_NO_FATAL_FAILURE(
        StructuralScrubber::instance().sanitize(unterminated, sizeof(unterminated)));
}

TEST(StructuralScrubberTest, LeavesUnrelatedTextUntouched) {
    const std::string plain = "I (1234) WifiManager: connected, rssi=-52 channel=6";
    EXPECT_EQ(scrub(plain), plain);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
