#include <gtest/gtest.h>

#include "core/ILogger.hpp"
#include "fakes/FakeLogger.hpp"

using namespace mnemis;

TEST(DebugTest, FakeLoggerStoresLevelAndMessage) {
    tests::FakeLogger logger;
    logger.log(core::LogLevel::Debug, "debug message");

    ASSERT_EQ(logger.logs.size(), 1u);
    EXPECT_EQ(logger.logs[0].level, core::LogLevel::Debug);
    EXPECT_EQ(logger.logs[0].message, "debug message");
}

TEST(DebugTest, FakeLoggerPreservesLogOrder) {
    tests::FakeLogger logger;
    logger.log(core::LogLevel::Info, "first");
    logger.log(core::LogLevel::Warning, "second");
    logger.log(core::LogLevel::Error, "third");

    ASSERT_EQ(logger.logs.size(), 3u);
    EXPECT_EQ(logger.logs[0].message, "first");
    EXPECT_EQ(logger.logs[1].message, "second");
    EXPECT_EQ(logger.logs[2].message, "third");
}