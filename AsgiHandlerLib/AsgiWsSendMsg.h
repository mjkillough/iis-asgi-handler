#pragma once

#include <msgpack.hpp>


class AsgiWsSendMsg
{
public:
    std::vector<char> bytes;
    bool close{false};
    // TODO: Custom unpacker so that we can unpack `text` into `bytes`?

    MSGPACK_DEFINE_MAP(bytes, close);
};
