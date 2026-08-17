#include <gtest/gtest.h>
#include "core/ApplicationContext.hpp"
#include "fakes/FakeLogger.hpp"
#include "fakes/FakeConfig.hpp"
#include "mocks/MockDatabaseService.hpp"
#include "mocks/MockThumbnailEngine.hpp"

using namespace mnemis;
using ::testing::_;
using ::testing::Return;

TEST(DILifecycleTest, ContextInitializationAndShutdown) {
    auto logger = std::make_unique<tests::FakeLogger>();
    auto config = std::make_unique<tests::FakeConfig>();
    
    // We capture raw pointers to verify behaviors after transfer
    auto loggerPtr = logger.get();

    auto dbService = std::make_unique<tests::MockDatabaseService>();
    EXPECT_CALL(*dbService, connect(":memory:"))
        .WillOnce(Return(core::Result<void>()));
    EXPECT_CALL(*dbService, disconnect())
        .Times(::testing::AtLeast(1));

    auto thumbnailEngine = std::make_unique<tests::MockThumbnailEngine>();

    core::ApplicationContext context(
        std::move(logger),
        std::move(config),
        std::move(dbService),
        std::move(thumbnailEngine)
    );

    auto initResult = context.initialize();
    EXPECT_TRUE(initResult.isSuccess());

    // Verify logger received the init message
    bool foundInitMsg = false;
    for (const auto& log : loggerPtr->logs) {
        if (log.message.find("Initializing") != std::string::npos) {
            foundInitMsg = true;
            break;
        }
    }
    EXPECT_TRUE(foundInitMsg);

    // Shutdown is implicitly called on context destruction, 
    // but we can call it explicitly
    context.shutdown();
}

TEST(DILifecycleTest, DatabaseInitFailure) {
    auto logger = std::make_unique<tests::FakeLogger>();
    auto config = std::make_unique<tests::FakeConfig>();
    auto dbService = std::make_unique<tests::MockDatabaseService>();
    auto thumbnailEngine = std::make_unique<tests::MockThumbnailEngine>();

    EXPECT_CALL(*dbService, connect(":memory:"))
        .WillOnce(Return(core::Result<void>(core::Error{1, "DB Error"})));

    core::ApplicationContext context(
        std::move(logger),
        std::move(config),
        std::move(dbService),
        std::move(thumbnailEngine)
    );

    auto initResult = context.initialize();
    EXPECT_TRUE(initResult.isError());
    EXPECT_EQ(initResult.error().code, 1);
}
