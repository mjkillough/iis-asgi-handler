#pragma once

#include <msgpack.hpp>


// Base class for ASGI messages that provides some helper functions for packing.
// We need to use these in custom packer functions (rather than relying on MSGPACK_MAP
// and friends) because we need control over whether std::string/std::vector<char> are
// written as binary or text, so that asgi_redis/channels decodes them correctly.
class AsgiMsg
{
protected:
    template <typename Packer>
    static void pack_string(Packer& packer, const std::string& str)
    {
        packer.pack_str(str.length());
        packer.pack_str_body(str.c_str(), str.length());
    }

    template <typename Packer>
    static void pack_bytestring(Packer& packer, const std::string& str)
    {
        packer.pack_bin(str.length());
        packer.pack_bin_body(str.c_str(), str.length());
    }
};
