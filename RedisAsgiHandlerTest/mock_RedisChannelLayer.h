#pragma once

#include "gmock/gmock.h"

#include "RedisChannelLayer.h"


class AsgiHttpRequestMsg;


class MockRedisChannelLayer : public RedisChannelLayer
{
public:
    MOCK_CONST_METHOD1(NewChannel, std::string(std::string));
    MOCK_METHOD2(Send, concurrency::task<void>(const std::string& channel, AsgiHttpRequestMsg& msg));
    MOCK_METHOD2(ReceiveMany, std::tuple<std::string, std::string>(const std::vector<std::string>& channels, bool blocking));
};
