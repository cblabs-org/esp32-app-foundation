#include <gtest/gtest.h>
#include "webServer.hpp"
#include "interfaces/IWebContributor.hpp"


class MockContributor : public IWebContributor {
public:
    bool registered = false;
    void registerRoutes(AsyncWebServer& server) override {
        registered = true;
    }
};

TEST(WebServerManagerTest, InitializationAndDispatch) {
    WebServerManager manager;
    
    MockContributor c1;
    MockContributor c2;
    std::vector<IWebContributor*> contributors = { &c1, &c2 };
    
    manager.begin(contributors);
    
    EXPECT_TRUE(c1.registered);
    EXPECT_TRUE(c2.registered);
    
#ifdef NATIVE_BUILD
    EXPECT_EQ(AsyncWebServer::lastMaxBodySize, 4096);
#endif
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
