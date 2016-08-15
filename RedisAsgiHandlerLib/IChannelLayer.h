#pragma once

#include <string>
#include <tuple>

#include <msgpack.hpp>


class IChannelLayer
{
public:
    virtual std::string NewChannel(std::string prefix) = 0;
    virtual void Send(const std::string& channel, const msgpack::sbuffer& buffer) = 0;
    virtual std::tuple<std::string, std::string> ReceiveMany(
        const std::vector<std::string>& channels, bool blocking = false
    ) = 0;
};
