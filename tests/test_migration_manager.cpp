#include <gtest/gtest.h>
#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"
#include "fakes/FakeLogger.hpp"

using namespace mnemis::database;
using namespace mnemis::core;

class MigrationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Open memory DB
        ASSERT_TRUE(conn.open(":memory:").isSuccess());
    }

    void TearDown() override {
        conn.close();
    }

    mnemis::tests::FakeLogger logger;
    DatabaseConnection conn;
};

TEST_F(MigrationManagerTest, ApplyMigrationsSuccessfully) {
    MigrationManager migrator(conn, logger);
    auto res = migrator.runMigrations();
    EXPECT_TRUE(res.isSuccess()) << (res.isError() ? res.error().message : "");

    // Verify table exists
    auto executeRes = conn.execute("SELECT media_id FROM media LIMIT 1;");
    EXPECT_TRUE(executeRes.isSuccess());

    // Verify version is 1
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(conn.getHandle(), "PRAGMA user_version;", -1, &stmt, nullptr);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 1);
    sqlite3_finalize(stmt);
}

TEST_F(MigrationManagerTest, Idempotency) {
    MigrationManager migrator(conn, logger);
    auto res1 = migrator.runMigrations();
    EXPECT_TRUE(res1.isSuccess());

    // Running again should do nothing but succeed
    auto res2 = migrator.runMigrations();
    EXPECT_TRUE(res2.isSuccess());
}
