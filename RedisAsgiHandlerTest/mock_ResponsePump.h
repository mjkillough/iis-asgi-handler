#pragma once

#include "gmock/gmock.h"

#include "mock_Logger.h"
#include "mock_ChannelLayer.h"
#include "ResponsePump.h"


class MockResponsePump : public ResponsePump
{
public:
    MockResponsePump()
        : ResponsePump(MockLogger(), MockChannelLayer())
    { }

    MOCK_METHOD2(AddChannel, void(const std::string& channel, const ResponseChannelCallback& callback));
    MOCK_METHOD1(RemoveChannel, void(const std::string& channel));
};
