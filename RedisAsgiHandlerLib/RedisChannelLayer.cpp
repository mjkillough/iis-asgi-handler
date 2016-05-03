#include <iostream>
#include <algorithm>

#include <WinSock2.h>
#include <rpc.h>

#include <hiredis.h>
#include <msgpack.hpp>

#include "RedisChannelLayer.h"


RedisChannelLayer::RedisChannelLayer(std::string ip, int port, std::string prefix)
    : m_redis_ctx(nullptr), m_prefix(prefix), m_expiry(60)
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

std::string RedisChannelLayer::NewChannel(std::string prefix) const
{
    return prefix + "not_unique";
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
    std::string channel(reply->element[0]->str);

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

std::string RedisChannelLayer::GenerateUuid()
{
    UUID uuid = { 0 };
    ::UuidCreate(&uuid);

    // TODO: Bleh, I don't think this is right. I don't think we end
    // up with ASCII.
    RPC_WSTR uuid_rpc_string = nullptr;
    ::UuidToStringW(&uuid, &uuid_rpc_string);
    std::string uuid_string = m_utf8_conv.to_bytes((wchar_t*)&uuid_rpc_string);
    ::RpcStringFreeW(&uuid_rpc_string);

    return uuid_string;
}
