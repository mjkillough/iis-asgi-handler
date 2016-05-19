#pragma once

#include <msgpack.hpp>


// Base class for ASGI messages that provides some helper functions for packing.
class AsgiMsg
{
protected:
    template <typename Packer>
    static void pack_string(Packer& packer, const std::string& str)
    {
        packer.pack_str(str.length());
        packer.pack_str_body(str.c_str(), str.length());
    }
};
