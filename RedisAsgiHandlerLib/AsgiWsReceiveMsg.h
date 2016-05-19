#pragma once

#include <msgpack.hpp>

#include "AsgiMsg.h"


class AsgiWsReceiveMsg : private AsgiMsg
{
public:
    AsgiWsReceiveMsg()
        : data(BUFFER_CHUNK_SIZE)
    { }

    int order{ 1 };
    std::string reply_channel;
    std::string path;

    // The buffer for holding the data can be up to BUFFER_CHUNK_SIZE bigger than
    // the actual length of the data, as we don't know how big the message is ahead
    // of time. `data_size` contains the true size of the message.
    std::vector<char> data;
    size_t data_size{ 0 };
    bool utf8;

    template <typename Packer>
    void msgpack_pack(Packer& packer) const
    {
        packer.pack_map(4);

        pack_string(packer, "reply_channel");
        pack_string(packer, reply_channel);

        pack_string(packer, "path");
        pack_string(packer, path);

        pack_string(packer, "order");
        packer.pack_int(order);

        // TODO: Convert to utf8.
        pack_string(packer, "bytes");
        packer.pack_bin(data_size);
        packer.pack_bin_body(data.data(), data_size);
    }

    // The initial size of the buffer and the size that it will be incremented
    // by each time the current message gets within  BUFFER_CHUNK_INCREASE_THRESHOLD.
    static const std::size_t BUFFER_CHUNK_SIZE = 4096;
    // The amount of freespace that must be remaining in the buffer before the current
    // buffer is expanded.
    static const std::size_t BUFFER_CHUNK_INCREASE_THRESHOLD = BUFFER_CHUNK_SIZE / 4;
};
