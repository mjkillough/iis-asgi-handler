#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_httpserv.h"
#include "mock_ResponsePump.h"
#include "mock_HttpRequestHandler.h"


using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::_;


class WriteResponseStepTest : public ::testing::Test
{
public:
    WriteResponseStepTest()
        : handler(response_pump, &http_context)
    {
        ON_CALL(http_context, GetResponse())
            .WillByDefault(Return(&response));
    }

    NiceMock<MockIHttpContext> http_context;
    MockResponsePump response_pump;
    MockHttpRequestHandler handler;
    MockIHttpResponse response;
    // No step here, as each test will need to pass a msg in during construction
};


TEST_F(WriteResponseStepTest, ReturnedFinishedOnError)
{
    auto msg = std::make_unique<AsgiHttpResponseMsg>();
    msg->content = "some content";
    WriteResponseStep step(handler, std::move(msg));

    EXPECT_CALL(response, WriteEntityChunks(_, _, _, _, _, _))
        .WillOnce(Return(E_ACCESSDENIED));

    EXPECT_EQ(kStepFinishRequest, step.Enter());
}


TEST_F(WriteResponseStepTest, ReturnsAsyncPending)
{
    auto msg = std::make_unique<AsgiHttpResponseMsg>();
    msg->content = "some content";
    WriteResponseStep step(handler, std::move(msg));

    EXPECT_CALL(response, WriteEntityChunks(_, _, _, _, _, _))
        .WillOnce(DoAll(
            SetArgPointee<5>(TRUE), // completion_expected
            Return(S_OK)
        ));

    EXPECT_EQ(kStepAsyncPending, step.Enter());
}


// If we repeatedly get completion_expected==FALSE, we might
// finish this step without ever explicitly calling OnAsyncCompletion.
TEST_F(WriteResponseStepTest, RerunsEnterUntilFinished)
{
    auto msg = std::make_unique<AsgiHttpResponseMsg>();
    msg->content = "some content";
    WriteResponseStep step(handler, std::move(msg));

    // Right now OnAsyncCompletion() will always return kStepFinishRequest,
    // as we ignore the num_bytes. (See comments there).
    EXPECT_CALL(response, WriteEntityChunks(_, _, _, _, _, _))
        .WillOnce(DoAll(
            SetArgPointee<4>(12), // num_bytes
            SetArgPointee<5>(FALSE), // completion_expected
            Return(S_OK)
        ));

    EXPECT_EQ(kStepFinishRequest, step.Enter());
}


/* Skipped: OnAsyncCompletion() currently ignores num_bytes.
TEST_F(WriteResponseStepTest, OnAsyncCompletionMoreToWrite)
{
    auto msg = std::make_unique<AsgiHttpResponseMsg>();
    msg->content = "some content";
    WriteResponseStep step(handler, std::move(msg));

    EXPECT_EQ(kStepRerun, step.OnAsyncCompletion(S_OK, 4));
}
*/


TEST_F(WriteResponseStepTest, OnAsyncCompletionNoMoreToWrite)
{
    auto msg = std::make_unique<AsgiHttpResponseMsg>();
    msg->content = "some content";
    WriteResponseStep step(handler, std::move(msg));

    EXPECT_EQ(kStepFinishRequest, step.OnAsyncCompletion(S_OK, 12));
}
