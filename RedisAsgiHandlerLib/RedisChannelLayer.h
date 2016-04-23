#pragma once

#include <functional>
#include <iostream>
#include <cvt/wstring>
#include <codecvt>

#include <hiredis.h>


typedef std::unique_ptr<redisReply, std::function<void(void*)>> RedisReply;


// This is not a generic class. It is to be used by Receive*(), which gets
// an ARRAY from BLPOP, but wants to transfer ownership of the char* in the
// second element to its parent.
class RedisData
{
public:
    RedisData(redisReply *reply)
        : m_reply(reply)
    { }
    RedisData(RedisData&& other)
        : m_reply(nullptr)
    {
        std::swap(m_reply, other.m_reply);
    }
    RedisData(const RedisData&) = delete;
    
    // Takes ownership from a RedisReply.
    RedisData(RedisReply&& reply)
        : m_reply(nullptr)
    {
        m_reply = reply.release();
    }

    ~RedisData()
    {
        Delete();
    }

    char* get() const
    {
        // TODO: Assert we have type str?
        return m_reply->str;
    }

    int length() const
    {
        return m_reply->len;
    }

private:
    void Delete()
    {
        if (m_reply != nullptr) {
            freeReplyObject(m_reply);
            m_reply = nullptr;
        }
    }

    redisReply *m_reply;
};


class RedisChannelLayer
{
public:

    RedisChannelLayer(std::string ip = "127.0.0.1", int port = 6379, std::string prefix = "asgi:");
    ~RedisChannelLayer();

    std::string NewChannel(std::string prefix) const;

    // Where T is something that can be passed to msgpack::pack().
    template<typename T>
    void Send(std::string channel, T& msg)
    {
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, msg);

        Send(channel, buffer.data(), buffer.size());
    }
    void Send(std::string channel, char *data, size_t data_length);

    RedisData Receive(std::string channel, bool blocking = false)
    {
        return std::get<1>(ReceiveMany({ channel }, blocking));
    }
    std::tuple<std::string, RedisData> ReceiveMany(std::vector<std::string> channels, bool blocking = false);


private:

    std::string GenerateUuid();

    template<typename... Args>
    RedisReply ExecuteRedisCommand(std::string format_string, Args... args)
    {
        auto reply = static_cast<redisReply*>(redisCommand(m_redis_ctx, format_string.c_str(), args...));
        RedisReply wrapped_reply(reply, freeReplyObject);
        return wrapped_reply;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> m_utf8_conv;

    std::string m_prefix;
    int m_expiry; // seconds
    redisContext *m_redis_ctx;
};
