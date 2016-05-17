#include <iostream>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#include <hiredis.h>
#include <msgpack.hpp>

#include "RedisChannelLayer.h"
#include "AsgiHttpRequestMsg.h"


RedisChannelLayer::RedisChannelLayer(std::string ip, int port, std::string prefix)
    : m_redis_ctx(nullptr), m_prefix(prefix), m_expiry(60), m_random_engine(std::random_device{}())
{
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    m_redis_ctx = redisConnectWithTimeout(ip.c_str(), port, timeout);
    if (m_redis_ctx == nullptr || m_redis_ctx->err) {
        if (m_redis_ctx == nullptr) {
            std::cerr << "Connection error: " << m_redis_ctx->errstr << std::endl;
            redisFree(m_redis_ctx);
        } else {
            std::cerr << "Could not create redis context" << std::endl;
        }
        // TODO: Some king of exception handling scheme.
        ::exit(1);
    }
}


RedisChannelLayer::~RedisChannelLayer()
{
    redisFree(m_redis_ctx);
}

std::string RedisChannelLayer::NewChannel(std::string prefix)
{
    return prefix + GenerateRandomAscii(10);
}

void RedisChannelLayer::Send(const std::string& channel, const msgpack::sbuffer& buffer)
{
    // asgi_redis chooses to store the data in a random key, then add the key to
    // the channel. (This allows us to set the data to expire, which we do).
    std::string data_key = m_prefix + GenerateRandomAscii(10);
    ExecuteRedisCommand("SET %s %b", data_key.c_str(), buffer.data(), buffer.size());
    ExecuteRedisCommand("EXPIRE %s %i", data_key.c_str(), m_expiry);

    // We also set expiry on the channel. (Subsequent Send()s will bump this expiry
    // up). We set to +10, because asgi_redis does... presumably to workaround the
    // fact that time has passed since we put the data into the data_key?
    std::string channel_key = m_prefix + channel;
    ExecuteRedisCommand("RPUSH %s %b", channel_key.c_str(), data_key.c_str(), data_key.length());
    ExecuteRedisCommand("EXPIRE %s %i", channel_key.c_str(), m_expiry + 10);
}

std::tuple<std::string, std::string> RedisChannelLayer::ReceiveMany(const std::vector<std::string>& channels, bool blocking)
{
    std::vector<std::string> prefixed_channels(channels.size());
    for (const auto& it : channels) {
        prefixed_channels.push_back(m_prefix + it);
    }

    // Shuffle to avoid one channel starving the others.
    std::shuffle(prefixed_channels.begin(), prefixed_channels.end(), m_random_engine);

    // Build up command for BLPOP
    // TODO: Allow a non-blocking version. (Possibly only a non-blocking version?)
    //       It looks like we might need a custom Lua script. Eek.
    std::vector<const char*> buffer;
    std::string blpop("BLPOP");
    buffer.push_back(blpop.c_str());
    for (const auto& it : prefixed_channels) {
        buffer.push_back(it.c_str());
    }
    std::string timeout("1");
    buffer.push_back(timeout.c_str());

    // Run the BLPOP. Do this manually for now, rather than trying to figure out a sensible API for
    // ExecuteRedisCommand to allow it to use redisCommandArgv().
    RedisReply reply(
        static_cast<redisReply*>(redisCommandArgv(m_redis_ctx, buffer.size(), buffer.data(), nullptr)),
        freeReplyObject
    );
    if (reply->type == REDIS_REPLY_NIL) {
        return std::make_tuple("", "");
    }
    // Remove the prefix before sharing the channel name with others.
    std::string channel(reply->element[0]->str + m_prefix.size(), reply->element[0]->len - m_prefix.size());

    // The response data is not actually stored in the channel. The channel contains a key.
    reply = ExecuteRedisCommand("GET %s", reply->element[1]->str);
    if (reply->type == REDIS_REPLY_NIL) {
        // This usually means that the message has expired. When this happens,
        // asgi_redis will loop and try to pull the next item from the list.
        // TODO: We should loop here too.
        return std::make_tuple("", "");
    }

    // TODO: Think of a way to avoid extra copies. Perhaps a msgpack::object
    //       with pointers into the original redisReply.
    return std::make_tuple(channel, std::string(reply->str, reply->len));
}

std::string RedisChannelLayer::GenerateRandomAscii(size_t length)
{
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static const size_t max_index = strlen(charset) - 1;
    std::string random_string(length, '0');
    std::generate_n(random_string.begin(), length, [this]() { return charset[m_random_engine() % max_index]; });
    return random_string;
}
