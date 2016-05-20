#pragma once

#include <msgpack.hpp>

#include "AsgiMsg.h"


class AsgiHttpRequestMsg : private AsgiMsg
{
public:
    std::string reply_channel;
    std::string http_version;
    std::string method;
    std::string scheme;
    std::string path;
    std::string query_string;
    std::string root_path;
    std::vector<std::tuple<std::string, std::string>> headers;
    std::vector<char> body;

    // TODO: body_channel and chunking.

    template <typename Packer>
    void msgpack_pack(Packer& packer) const
    {
        packer.pack_map(8);

        pack_string(packer, "reply_channel");
        pack_string(packer, reply_channel);
        pack_string(packer, "http_version");
        pack_string(packer, http_version);
        pack_string(packer, "method");
        pack_string(packer, method);
        pack_string(packer, "scheme");
        pack_string(packer, scheme);

        // The values of these entries must be byte strings.
        pack_string(packer, "path");
        pack_bytestring(packer, path);
        pack_string(packer, "query_string");
        pack_bytestring(packer, query_string);
        // Django seems to expect a unicode string, not a byte string.
        // pack_string(packer, "root_path");
        // pack_bytestring(packer, root_path);

        pack_string(packer, "headers");
        packer.pack_array(headers.size());
        for (auto header : headers) {
            packer.pack_array(2);
            pack_bytestring(packer, std::get<0>(header));
            pack_bytestring(packer, std::get<1>(header));
        }

        pack_string(packer, "body");
        packer.pack_bin(body.size());
        packer.pack_bin_body(body.data(), body.size());
    }
};
