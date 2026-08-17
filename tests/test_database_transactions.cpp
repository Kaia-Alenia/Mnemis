#include <gtest/gtest.h>
#include "database/DatabaseConnection.hpp"

using namespace mnemis::database;

class DatabaseTransactionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(conn.open(":memory:").isSuccess());
        ASSERT_TRUE(conn.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY);").isSuccess());
    }

    void TearDown() override {
        conn.close();
    }

    DatabaseConnection conn;
};

TEST_F(DatabaseTransactionsTest, CommitSavesData) {
    {
        TransactionGuard txn(conn);
        ASSERT_TRUE(txn.begin().isSuccess());
        EXPECT_TRUE(conn.execute("INSERT INTO test_table (id) VALUES (1);").isSuccess());
        EXPECT_TRUE(txn.commit().isSuccess());
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(conn.getHandle(), "SELECT COUNT(*) FROM test_table;", -1, &stmt, nullptr);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 1);
    sqlite3_finalize(stmt);
}

TEST_F(DatabaseTransactionsTest, DestructorRollbacksUncommitted) {
    {
        TransactionGuard txn(conn);
        ASSERT_TRUE(txn.begin().isSuccess());
        EXPECT_TRUE(conn.execute("INSERT INTO test_table (id) VALUES (2);").isSuccess());
        // No commit, goes out of scope -> rollback
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(conn.getHandle(), "SELECT COUNT(*) FROM test_table;", -1, &stmt, nullptr);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 0);
    sqlite3_finalize(stmt);
}
