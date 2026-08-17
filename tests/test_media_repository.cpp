#include <gtest/gtest.h>
#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"
#include "database/repositories/SQLiteMediaRepository.hpp"
#include "fakes/FakeLogger.hpp"

using namespace mnemis::database;
using namespace mnemis::core::models;

class MediaRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(conn.open(":memory:").isSuccess());
        MigrationManager migrator(conn, logger);
        ASSERT_TRUE(migrator.runMigrations().isSuccess());
        repo = std::make_unique<repositories::SQLiteMediaRepository>(conn);
    }

    void TearDown() override {
        repo.reset();
        conn.close();
    }

    mnemis::tests::FakeLogger logger;
    DatabaseConnection conn;
    std::unique_ptr<repositories::SQLiteMediaRepository> repo;
};

TEST_F(MediaRepositoryTest, CRUDOperations) {
    MediaItem item;
    item.mediaId = "test_id_1";
    item.path = "/test/path.mp4";
    item.canonicalPath = "/test/path.mp4";
    item.fileName = "path.mp4";
    item.extension = "mp4";
    item.mimeType = "video/mp4";
    item.mediaType = MediaType::Video;
    item.fileSize = 1024;
    item.modifiedTime = 1000;
    item.createdTime = 1000;
    item.title = "Test Video";

    // Create
    auto addRes = repo->add(item);
    EXPECT_TRUE(addRes.isSuccess()) << (addRes.isError() ? addRes.error().message : "");

    // Read
    auto getRes = repo->getById("test_id_1");
    EXPECT_TRUE(getRes.isSuccess());
    ASSERT_TRUE(getRes.value().has_value());
    EXPECT_EQ(getRes.value()->title, "Test Video");

    // Update
    item.title = "Updated Video";
    auto updateRes = repo->update(item);
    EXPECT_TRUE(updateRes.isSuccess());

    auto getRes2 = repo->getById("test_id_1");
    EXPECT_EQ(getRes2.value()->title, "Updated Video");

    // Delete
    auto delRes = repo->remove("test_id_1");
    EXPECT_TRUE(delRes.isSuccess());

    auto getRes3 = repo->getById("test_id_1");
    EXPECT_TRUE(getRes3.isSuccess());
    EXPECT_FALSE(getRes3.value().has_value());
}

TEST_F(MediaRepositoryTest, Pagination) {
    for (int i = 1; i <= 25; ++i) {
        MediaItem item;
        item.mediaId = "id_" + std::to_string(i);
        item.path = "/path/" + std::to_string(i);
        item.canonicalPath = "/path/" + std::to_string(i);
        item.fileName = std::to_string(i);
        item.extension = "ext";
        item.mimeType = "type";
        item.mediaType = MediaType::Video;
        item.fileSize = 0;
        item.modifiedTime = 0;
        item.createdTime = i; // sort order test
        ASSERT_TRUE(repo->add(item).isSuccess());
    }

    auto listRes = repo->list(1, 10);
    EXPECT_TRUE(listRes.isSuccess());
    EXPECT_EQ(listRes.value().size(), 10);
    EXPECT_EQ(listRes.value()[0].mediaId, "id_25"); // desc order by createdTime

    auto listRes2 = repo->list(3, 10);
    EXPECT_TRUE(listRes2.isSuccess());
    EXPECT_EQ(listRes2.value().size(), 5);
}

TEST_F(MediaRepositoryTest, DuplicateCanonicalPathFails) {
    MediaItem item;
    item.mediaId = "id1";
    item.path = "/test/path";
    item.canonicalPath = "/test/path";
    item.fileName = "path";
    item.extension = "ext";
    item.mimeType = "type";
    item.mediaType = MediaType::Video;
    item.fileSize = 0;
    item.modifiedTime = 0;
    item.createdTime = 0;
    EXPECT_TRUE(repo->add(item).isSuccess());

    MediaItem item2 = item;
    item2.mediaId = "id2";
    auto addRes2 = repo->add(item2);
    EXPECT_TRUE(addRes2.isError()); // Constraint violation
}
