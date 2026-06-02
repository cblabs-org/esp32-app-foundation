#include <gtest/gtest.h>
#include "GithubClient.hpp"

class GithubClientTest : public ::testing::Test {
};

TEST_F(GithubClientTest, ConstructionWithRepo) {
    GithubClient client("owner/repo");
    EXPECT_STREQ(client.getLastError(), "Initialized");
}

TEST_F(GithubClientTest, ConstructionWithRepoAndToken) {
    GithubClient client("owner/repo", "ghp_token123");
    EXPECT_STREQ(client.getLastError(), "Initialized");
}

TEST_F(GithubClientTest, GetLastErrorInitial) {
    GithubClient client("owner/repo");
    EXPECT_STREQ(client.getLastError(), "Initialized");
}

TEST_F(GithubClientTest, ReleaseIdInitialState) {
    GithubClient client("owner/repo");
    EXPECT_TRUE(client.releaseId.length() == 0);
}

TEST_F(GithubClientTest, ReleaseAssetsInitialState) {
    GithubClient client("owner/repo");
    EXPECT_EQ(client.releaseAssets, 0);
}

TEST_F(GithubClientTest, ConstructionWithEmptyRepo) {
    GithubClient client("");
    EXPECT_STREQ(client.getLastError(), "Initialized");
}

TEST_F(GithubClientTest, ConstructionWithEmptyToken) {
    GithubClient client("owner/repo", "");
    EXPECT_STREQ(client.getLastError(), "Initialized");
}

TEST_F(GithubClientTest, GetLatestReleaseOnNative) {
    GithubClient client("owner/repo");
    client.getLatestRelease();
    EXPECT_STREQ(client.getLastError(), "SUCCESS");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
