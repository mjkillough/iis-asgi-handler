#include <gtest/gtest.h>

#include "ProcessPool.h"


// Makes the EscapeArgument() function accessible from the tests.
class ProcessPoolPublic : public ProcessPool
{
public:
    using ProcessPool::EscapeArgument;
};


TEST(ProcessPoolEscapeArgument, NoSpecialCharacters)
{
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"test"), L"test");
}

TEST(ProcessPoolEscapeArgument, Spaces)
{
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"a space"), L"\"a space\"");
}

TEST(ProcessPoolEscapeArgument, Quote)
{
    // Quotes should be escaped.
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"a \"test\""), L"\"a \\\"test\\\"\"");
}

TEST(ProcessPoolEscapeArgument, Backslashes)
{
    // Backslashes within the string should be escaped if followed by a quote.
    // (i.e. 1 backslash and a quote become 3 backslashes and a quote).
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"test \\\" str"), L"\"test \\\\\\\" str\"");

    // Backslashes within the string should not be escaped if not followed by
    // a quote.
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"test\\ str"), L"\"test\\ str\"");
    // Which means if there are no spaces, they shouldn't be escaped at all,
    // even if they appear at the end.
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"test\\"), L"test\\");

    // If the string ends in a backslash, these must all be escaped, but the
    // final " should not be escaped. (i.e. 2 becomes 4).
    EXPECT_EQ(ProcessPoolPublic::EscapeArgument(L"test str\\\\"), L"\"test str\\\\\\\\\"");
}
