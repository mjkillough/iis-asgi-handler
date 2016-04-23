#pragma once

#include <msgpack.hpp>


struct AsgiHttpRequestMsg {
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
};


namespace msgpack {
	MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
		namespace adaptor {

			template<>
			struct pack<AsgiHttpRequestMsg> {
				template<typename Stream>
                void pack_str_as_str(msgpack::packer<Stream>& packer, const std::string& str) const {
					packer.pack_str(str.length());
					packer.pack_str_body(str.c_str(), str.length());
				}

				template<typename Stream>
                void pack_str_as_bin(msgpack::packer<Stream>& packer, const std::string& str) const {
					packer.pack_bin(str.length());
					packer.pack_bin_body(str.c_str(), str.length());
				}

				template<typename Stream>
				msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& packer, AsgiHttpRequestMsg const& msg) const {
					packer.pack_map(8);

					pack_str_as_str(packer, "reply_channel");
					pack_str_as_str(packer, msg.reply_channel);
                    pack_str_as_str(packer, "http_version");
                    pack_str_as_str(packer, msg.http_version);
                    pack_str_as_str(packer, "method");
                    pack_str_as_str(packer, msg.method);
                    pack_str_as_str(packer, "scheme");
                    pack_str_as_str(packer, msg.scheme);

                    // The values of these entries must be byte strings.
                    pack_str_as_str(packer, "path");
                    pack_str_as_bin(packer, msg.path);
                    pack_str_as_str(packer, "query_string");
                    pack_str_as_bin(packer, msg.query_string);
                    // Django seems to expect a unicode string, not a byte string.
                    // pack_str_as_str(packer, "root_path");
                    // pack_str_as_bin(packer, msg.root_path);

                    pack_str_as_str(packer, "headers");
                    packer.pack_array(msg.headers.size());
                    for (auto header : msg.headers) {
                        packer.pack_array(2);
                        pack_str_as_bin(packer, std::get<0>(header));
                        pack_str_as_bin(packer, std::get<1>(header));
                    }
                    
                    pack_str_as_str(packer, "body");
                    packer.pack_bin(msg.body.size());
                    packer.pack_bin_body(msg.body.data(), msg.body.size());

                    return packer;
				}
			};

		}
	}
}