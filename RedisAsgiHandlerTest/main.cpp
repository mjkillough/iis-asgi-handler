#include <iostream>

#include "gtest/gtest.h"
#include "gmock/gmock.h" 

TEST(Basic, Assert) {
    ASSERT_TRUE(false);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    auto result = RUN_ALL_TESTS();
    std::cin.get();
    return result;
}
