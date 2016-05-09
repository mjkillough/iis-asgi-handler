#pragma once

#include "gmock/gmock.h"

#include "mock_Logger.h"
#include "mock_RedisChannelLayer.h"
#include "HttpRequestHandler.h"


using ::testing::NiceMock;


class MockHttpRequestHandler : public HttpRequestHandler
{
public:
    MockHttpRequestHandler(MockResponsePump& response_pump, MockIHttpContext* http_context)
        : HttpRequestHandler(response_pump, channels, logger, http_context)
    { }

    MockRedisChannelLayer channels;
    NiceMock<MockLogger> logger;
};
