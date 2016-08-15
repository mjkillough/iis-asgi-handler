#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_httpserv.h"
#include "mock_ResponsePump.h"
#include "mock_HttpRequestHandler.h"


using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;


class FlushResponseStepTest : public ::testing::Test
{
public:
    FlushResponseStepTest()
        : handler(response_pump, &http_context), step(handler, "channel")
    {
        ON_CALL(http_context, GetResponse())
            .WillByDefault(Return(&response));
    }

    NiceMock<MockIHttpContext> http_context;
    MockResponsePump response_pump;
    MockHttpRequestHandler handler;
    MockIHttpResponse response;
    FlushResponseStep step;
};


TEST_F(FlushResponseStepTest, ReturnedFinishedOnError)
{
    EXPECT_CALL(response, Flush(true, true, _, _))
        .WillOnce(Return(E_ACCESSDENIED));

    EXPECT_EQ(kStepFinishRequest, step.Enter());
}


TEST_F(FlushResponseStepTest, ReturnsAsyncPending)
{
    auto msg = std::make_unique<AsgiHttpResponseMsg>();
    EXPECT_CALL(response, Flush(true, true, _, _))
        .WillOnce(DoAll(
            SetArgPointee<3>(TRUE), // completion_expected
            Return(S_OK)
        ));


    EXPECT_EQ(kStepAsyncPending, step.Enter());
}


// If we repeatedly get completion_expected==FALSE, we might
// finish this step without ever explicitly calling OnAsyncCompletion.
TEST_F(FlushResponseStepTest, HandlesCompletionExpectedFalse)
{
    // Right now OnAsyncCompletion() will always return kStepFinishRequest,
    // as we ignore the num_bytes. (There's nothing we could do with them,
    // as we do not know the full size of the response).
    EXPECT_CALL(response, Flush(true, true, _, _))
        .WillOnce(DoAll(
            SetArgPointee<2>(12), // num_bytes
            SetArgPointee<3>(FALSE), // completion_expected
            Return(S_OK)
        ));

    EXPECT_EQ(kStepGotoNext, step.Enter());
}
