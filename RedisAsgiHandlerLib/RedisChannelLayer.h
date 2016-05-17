#pragma once
#define WIN32_LEAN_AND_MEAN

#include <functional>
#include <iostream>
#include <cvt/wstring>
#include <codecvt>
#include <random>

#include <msgpack.hpp>
#include <hiredis.h>


typedef std::unique_ptr<redisReply, std::function<void(void*)>> RedisReply;


class RedisChannelLayer
{
public:
    RedisChannelLayer(std::string ip = "127.0.0.1", int port = 6379, std::string prefix = "asgi:");
    virtual ~RedisChannelLayer();

    virtual std::string NewChannel(std::string prefix);
    virtual void Send(const std::string& channel, const msgpack::sbuffer& buffer);
    virtual std::tuple<std::string, std::string> ReceiveMany(const std::vector<std::string>& channels, bool blocking = false);

protected:
    std::string GenerateRandomAscii(size_t length);

    template<typename... Args>
    RedisReply ExecuteRedisCommand(std::string format_string, Args... args)
    {
        auto reply = static_cast<redisReply*>(redisCommand(m_redis_ctx, format_string.c_str(), args...));
        RedisReply wrapped_reply(reply, freeReplyObject);
        return wrapped_reply;
    }

private:
    std::string m_prefix;
    int m_expiry; // seconds
    redisContext *m_redis_ctx;
    std::default_random_engine m_random_engine;
};
