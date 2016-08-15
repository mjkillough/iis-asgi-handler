#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_httpserv.h"
#include "mock_ResponsePump.h"
#include "mock_HttpRequestHandler.h"


using ::testing::MatcherCast;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrEq;
using ::testing::_;


class WaitForResponseStepTest : public ::testing::Test
{
public:
    WaitForResponseStepTest()
        : handler(response_pump, &http_context), reply_channel("http.request!blah"),
        step(handler, reply_channel)
    {
        ON_CALL(http_context, GetResponse())
            .WillByDefault(Return(&response));

        // These functions are called often, but only some tests are interested in
        // them. The interested tests will EXPECT_CALL.
        ON_CALL(response, SetStatus(_, _, _, _, _, _))
            .WillByDefault(Return(S_OK));
    }

    std::string GetDummyResponse(
        int status, std::vector<std::tuple<std::string, std::string>> headers, std::string content,
        bool more_content = false
    ) const
    {
        AsgiHttpResponseMsg msg;
        msg.status = status;
        msg.headers = headers;
        msg.content = content;
        msg.more_content = more_content;
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, msg);
        return std::string(buffer.data(), buffer.size());
    }

    NiceMock<MockIHttpContext> http_context;
    MockResponsePump response_pump;
    MockHttpRequestHandler handler;
    NiceMock<MockIHttpResponse> response;
    std::string reply_channel;
    WaitForResponseStep step;
};


TEST_F(WaitForResponseStepTest, EnterAddsCallbackAndReturnsAsyncPending)
{
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .Times(1);

    EXPECT_EQ(kStepAsyncPending, step.Enter());
}


TEST_F(WaitForResponseStepTest, CallbackCallsPostCompletion)
{
    ResponsePump::ResponseChannelCallback callback;
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .WillOnce(SaveArg<1>(&callback));

    EXPECT_EQ(kStepAsyncPending, step.Enter());

    EXPECT_CALL(http_context, PostCompletion(0))
        .Times(1);

    callback(GetDummyResponse(200, { } , ""));
}


TEST_F(WaitForResponseStepTest, OnAsyncCompletionReturnsGotoNextIfBody)
{
    ResponsePump::ResponseChannelCallback callback;
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .WillOnce(SaveArg<1>(&callback));

    step.Enter();

    callback(GetDummyResponse(200, { }, "some body"));

    EXPECT_EQ(kStepGotoNext, step.OnAsyncCompletion(S_OK, 0));
}


TEST_F(WaitForResponseStepTest, OnAsyncCompletionReturnsFinishIfNoBody)
{
    ResponsePump::ResponseChannelCallback callback;
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .WillOnce(SaveArg<1>(&callback));

    step.Enter();

    callback(GetDummyResponse(200, { }, ""));

    EXPECT_EQ(kStepFinishRequest, step.OnAsyncCompletion(S_OK, 0));
}


TEST_F(WaitForResponseStepTest, OnAsyncCompletionReturnsGotoNextIfNoBodyButMoreChunks)
{
    ResponsePump::ResponseChannelCallback callback;
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .WillOnce(SaveArg<1>(&callback));

    step.Enter();

    callback(GetDummyResponse(200, {}, "", true));

    EXPECT_EQ(kStepGotoNext, step.OnAsyncCompletion(S_OK, 0));
}


TEST_F(WaitForResponseStepTest, OnAsyncCompletionSetsStatus)
{
    ResponsePump::ResponseChannelCallback callback;
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .WillOnce(SaveArg<1>(&callback));

    step.Enter();

    // SetStatus should be called with the status.
    {
        callback(GetDummyResponse(404, { }, ""));

        EXPECT_CALL(response, SetStatus(404, _, _, _, _, _))
            .WillOnce(Return(S_OK));

        step.OnAsyncCompletion(S_OK, 0);
    }

    // SetStatus should not be called if the status is 0 (missing in msgpack).
    // This happens for chunks in streaming responses.
    {
        callback(GetDummyResponse(0, { }, ""));

        EXPECT_CALL(response, SetStatus(_, _, _, _, _, _))
            .Times(0);

        step.OnAsyncCompletion(S_OK, 0);
    }
}


TEST_F(WaitForResponseStepTest, OnAsyncCompletionSetsHeaders)
{
    ResponsePump::ResponseChannelCallback callback;
    EXPECT_CALL(response_pump, AddChannel(reply_channel, _))
        .WillOnce(SaveArg<1>(&callback));

    step.Enter();

    // SetHeader should be called for each header.
    {
        callback(GetDummyResponse(404, {
            std::make_tuple("header1", "value1"),
            std::make_tuple("header2", "value-2")
        }, ""));

        EXPECT_CALL(response, SetHeader(MatcherCast<PCSTR>(StrEq("header1")), StrEq("value1"), 6, TRUE))
            .Times(1);
        EXPECT_CALL(response, SetHeader(MatcherCast<PCSTR>(StrEq("header2")), StrEq("value-2"), 7, TRUE))
            .Times(1);

        step.OnAsyncCompletion(S_OK, 0);
    }

    // If there are no headers, SetHeader should not be called at all.
    // This happens for chunks in streaming responses.
    {
        callback(GetDummyResponse(404, { }, ""));

        EXPECT_CALL(response, SetHeader(MatcherCast<PCSTR>(_), _, _, _))
            .Times(0);

        step.OnAsyncCompletion(S_OK, 0);
    }
}
