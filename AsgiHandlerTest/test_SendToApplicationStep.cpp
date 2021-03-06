#include <ppltasks.h>
#include <condition_variable>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_httpserv.h"
#include "mock_ResponsePump.h"
#include "mock_HttpRequestHandler.h"


using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;


class SendToApplicationStepTest : public ::testing::Test
{
public:
    SendToApplicationStepTest()
        : handler(response_pump, &http_context), msg(std::make_unique<AsgiHttpRequestMsg>()),
          step(handler, msg)
    { }

    MockIHttpContext http_context;
    MockResponsePump response_pump;
    MockHttpRequestHandler handler;
    std::unique_ptr<AsgiHttpRequestMsg> msg;
    SendToApplicationStep step;
};


ACTION_P(SetConditionVariable, condition)
{
    condition->notify_all();
    return S_OK;
}


TEST_F(SendToApplicationStepTest, SendToChannelAndReturnsAsyncPending)
{
    std::condition_variable condition;
    std::mutex mutex;

    EXPECT_CALL(handler.channels, Send("http.request", _))
        .Times(1);

    EXPECT_EQ(kStepAsyncPending, step.Enter());

    EXPECT_CALL(http_context, PostCompletion(0))
        .WillOnce(SetConditionVariable(&condition));

    {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock);
    }
}


// This test will become more interesting if/when OnAsyncCompletion() has
// to handle errors or cancellation.
TEST_F(SendToApplicationStepTest, OnAsyncCompletionReturnsGotoNext)
{
    EXPECT_EQ(kStepGotoNext, step.OnAsyncCompletion(S_OK, 0));
}
