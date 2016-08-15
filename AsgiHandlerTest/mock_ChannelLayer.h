#pragma once

#include "gmock/gmock.h"

#include "ChannelLayer.h"


class AsgiHttpRequestMsg;


class MockChannelLayer : public ChannelLayer
{
public:
    MOCK_METHOD2(Send, void(const std::string& channel, const msgpack::sbuffer& msg));
    MOCK_METHOD2(ReceiveMany,
        std::tuple<std::string, std::string>(
            const std::vector<std::string>& channels, bool blocking
        )
    );
};
