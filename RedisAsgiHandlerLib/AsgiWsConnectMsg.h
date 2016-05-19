#pragma once

#include <msgpack.hpp>

#include "AsgiMsg.h"


class AsgiWsConnectMsg : private AsgiMsg
{
public:
    std::string reply_channel;
    std::string scheme;
    std::string path;
    std::string query_string;
    std::string root_path;
    std::vector<std::tuple<std::string, std::string>> headers;
    // `order` forced to 0 in pack function.

    // TODO: implement `client` and `server` fields.

    template <typename Packer>
    void msgpack_pack(Packer& packer) const
    {
        packer.pack_map(7);

        pack_string(packer, "reply_channel");
        pack_string(packer, reply_channel);
        pack_string(packer, "scheme");
        pack_string(packer, scheme);
        pack_string(packer, "path");
        pack_string(packer, path);
        pack_string(packer, "query_string");
        pack_bytestring(packer, query_string);
        pack_string(packer, "root_path");
        pack_bytestring(packer, root_path);

        pack_string(packer, "headers");
        packer.pack_array(headers.size());
        for (auto header : headers) {
            packer.pack_array(2);
            pack_bytestring(packer, std::get<0>(header));
            pack_bytestring(packer, std::get<1>(header));
        }

        pack_string(packer, "order");
        packer.pack_int(0);
    }
};