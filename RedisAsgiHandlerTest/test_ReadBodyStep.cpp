#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_httpserv.h"
#include "mock_ResponsePump.h"
#include "mock_HttpRequestHandler.h"


using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SaveArg;
using ::testing::_;


class ReadBodyStepTest : public ::testing::Test
{
public:
    ReadBodyStepTest()
        : handler(response_pump, &http_context), msg(std::make_unique<AsgiHttpRequestMsg>()),
          step(handler, msg)
    {
        ON_CALL(http_context, GetRequest())
            .WillByDefault(Return(&request));
    }

    NiceMock<MockIHttpContext> http_context;
    MockResponsePump response_pump;
    MockHttpRequestHandler handler;
    MockIHttpRequest request;
    std::unique_ptr<AsgiHttpRequestMsg> msg;
    ReadBodyStep step;
};


TEST_F(ReadBodyStepTest, EnterDoesNothingForNoBody)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillRepeatedly(Return(0));
    // We shouldn't get as far as calling ReadEntityBody()
    EXPECT_CALL(request, ReadEntityBody(_, _, _, _, _))
        .Times(0);

    EXPECT_EQ(kStepGotoNext, step.Enter());
}


TEST_F(ReadBodyStepTest, EnterFinishesRequestOnError)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(request, ReadEntityBody(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(E_INVALIDARG));

    EXPECT_EQ(kStepFinishRequest, step.Enter());
}


// Handle ReadEntityBody() completion synchronously.
TEST_F(ReadBodyStepTest, EnterHandlesSynchronousRead)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));

    EXPECT_CALL(request, ReadEntityBody(_, _, _, _, _))
        .Times(1)
        .WillOnce(DoAll(
            SetArgPointee<3>(1), // bytes_read
            SetArgPointee<4>(FALSE), // completion_expected
            Return(S_OK)
        ));

    EXPECT_EQ(kStepGotoNext, step.Enter());
}


// Continues to read body if ReadEntityBody() completes synchronously
// and there's more data to read. Subseqeunt calls to ReadEntityBody()
// give a pointer to a different place in the buffer.
TEST_F(ReadBodyStepTest, EnterContinuesToRead)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillOnce(Return(2))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));

    VOID* buffer_ptr1 = nullptr;
    VOID* buffer_ptr2 = nullptr;

    EXPECT_CALL(request, ReadEntityBody(_, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SaveArg<0>(&buffer_ptr1),
            SetArgPointee<3>(1), // bytes_read
            SetArgPointee<4>(FALSE), // completion_expected
            Return(S_OK)
        ))
        .WillOnce(DoAll(
            SaveArg<0>(&buffer_ptr2),
            SetArgPointee<3>(1), // bytes_read
            SetArgPointee<4>(FALSE), // completion_expected
            Return(S_OK)
        ));

    EXPECT_EQ(kStepGotoNext, step.Enter());
    EXPECT_EQ(1, (char*)buffer_ptr2 - (char*)buffer_ptr1);
}


// Returns kStepAsyncPending when ReadEntityBody() doesn't complete synchronously.
TEST_F(ReadBodyStepTest, EnterReturnsAsyncPending)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillRepeatedly(Return(1));

    // Calls OnAsyncCompletion() if read finishes inline, does not call
    // ReadBodyEntity() again if OnAsyncCompletion() != kStepRerun
    EXPECT_CALL(request, ReadEntityBody(_, _, _, _, _))
        .Times(1)
        .WillOnce(DoAll(
            SetArgPointee<4>(TRUE), // completion_expected
            Return(S_OK)
        ));

    EXPECT_EQ(kStepAsyncPending, step.Enter());
}


TEST_F(ReadBodyStepTest, OnAsyncCompletionFinishesRequestOnError)
{
    EXPECT_EQ(kStepFinishRequest, step.OnAsyncCompletion(E_ACCESSDENIED, 1));
}


// Should return kStepRerun when there's more data to read.
TEST_F(ReadBodyStepTest, OnAsyncCompletionMoreData)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillRepeatedly(Return(1));

    EXPECT_EQ(kStepRerun, step.OnAsyncCompletion(S_OK, 1));
}


// Should goto next step when there's no more data to read.
TEST_F(ReadBodyStepTest, OnAsyncCompletionNoMoreData)
{
    EXPECT_CALL(request, GetRemainingEntityBytes())
        .WillRepeatedly(Return(0));

    EXPECT_EQ(kStepGotoNext, step.OnAsyncCompletion(S_OK, 1));
}
