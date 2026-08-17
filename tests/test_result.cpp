#include <gtest/gtest.h>
#include "core/Result.hpp"

using namespace mnemis::core;

TEST(ResultTest, VoidSuccess) {
    Result<void> res;
    EXPECT_TRUE(res.isSuccess());
    EXPECT_FALSE(res.isError());
}

TEST(ResultTest, VoidError) {
    Result<void> res(Error{404, "Not Found"});
    EXPECT_FALSE(res.isSuccess());
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error().code, 404);
    EXPECT_EQ(res.error().message, "Not Found");
}

TEST(ResultTest, ValueSuccess) {
    Result<int> res(42);
    EXPECT_TRUE(res.isSuccess());
    EXPECT_FALSE(res.isError());
    EXPECT_EQ(res.value(), 42);
}

TEST(ResultTest, ValueError) {
    Result<int> res(Error{500, "Server Error"});
    EXPECT_FALSE(res.isSuccess());
    EXPECT_TRUE(res.isError());
    EXPECT_EQ(res.error().code, 500);
    EXPECT_EQ(res.error().message, "Server Error");
}
