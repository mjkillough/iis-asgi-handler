#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_Logger.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

TEST(LoggerTest, Basic)
{
    MockLogger logger;

    std::string output;
    EXPECT_CALL(logger, Log(_))
        .WillOnce(SaveArg<0>(&output));

    logger.debug() << "This is a test " << 1 << 2 << 3;

    EXPECT_EQ("This is a test 123", output);
}
