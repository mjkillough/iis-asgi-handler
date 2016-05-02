#include <iostream>
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

std::tuple<std::string, RedisData> RedisChannelLayer::ReceiveMany(std::vector<std::string> channels, bool blocking)
{
    // TODO: Wait on more than one channel.
    // TODO: Respect blocking.
    // TODO: Investigate if there's any way we can avoid a copy when we put into unique_ptr
    auto reply = ExecuteRedisCommand("BLPOP %s%s 0", m_prefix.c_str(), channels[0].c_str());
    // NOTE: If we set a timeout, we should change this to check the type of the reply.
    std::string channel(reply->element[0]->str);
    // This gives us the name of another key (including m_prefix) which contains the data.
    reply = ExecuteRedisCommand("GET %s", reply->element[1]->str);
    // TODO: Check success.
    return std::make_tuple(
        channel,
        std::move(reply) // only makes [1]->str available to caller
    );
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
