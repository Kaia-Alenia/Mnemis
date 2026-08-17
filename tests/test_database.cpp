#include <gtest/gtest.h>
#include "database/SQLiteDatabaseService.hpp"
#include "fakes/FakeLogger.hpp"
#include "fakes/FakeConfig.hpp"
#include "core/models/MediaItem.hpp"
#include <stdexcept>

using namespace mnemis;

TEST(DatabaseTest, OpenAndCloseMemoryDB) {
    tests::FakeLogger logger;
    tests::FakeConfig config;

    database::SQLiteDatabaseService dbService(logger, config);

    // Should connect to memory DB successfully
    auto connectRes = dbService.connect(":memory:");
    EXPECT_TRUE(connectRes.isSuccess()) << (connectRes.isError() ? connectRes.error().message : "");

    // Should return error if already connected
    auto connectRes2 = dbService.connect(":memory:");
    EXPECT_TRUE(connectRes2.isError());

    // Domain specific method test
    auto& repo = dbService.getMediaRepository();
    // Test that the repo works, e.g., list is empty
    auto listRes = repo.list(1, 10);
    EXPECT_TRUE(listRes.isSuccess());
    EXPECT_EQ(listRes.value().size(), 0);

    dbService.disconnect();

    // Disconnected state
    EXPECT_THROW(dbService.getMediaRepository(), std::runtime_error);
}
