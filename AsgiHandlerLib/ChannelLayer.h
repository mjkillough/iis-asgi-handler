#pragma once

#include <string>
#include <tuple>
#include <algorithm>
#include <codecvt>
#include <random>

#include <msgpack.hpp>


class ChannelLayer
{
public:
    ChannelLayer()
        : m_random_engine(std::random_device{}())
    { }

    std::string NewChannel(std::string prefix)
    {
        return prefix + GenerateRandomAscii(10);
    }

    virtual void Send(const std::string& channel, const msgpack::sbuffer& buffer) = 0;
    virtual std::tuple<std::string, std::string> ReceiveMany(
        const std::vector<std::string>& channels, bool blocking = false
    ) = 0;

protected:
    std::default_random_engine m_random_engine;

    std::string GenerateRandomAscii(size_t length)
    {
        static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        static const size_t max_index = strlen(charset) - 1;
        std::string random_string(length, '0');
        std::generate_n(random_string.begin(), length, [this]() { return charset[m_random_engine() % max_index]; });
        return random_string;
    }
};
