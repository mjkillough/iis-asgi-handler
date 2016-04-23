#pragma once

#include <msgpack.hpp>


/*
status:       Integer HTTP status code.
headers:      A list of [name, value] pairs, where name is the byte string header name,
              and value is the byte string header value. Order should be preserved in
              the HTTP response. Header names must be lowercased.
content:      Byte string of HTTP body content. Optional, defaults to empty string.
more_content: Boolean value signifying if there is additional content to come (as
              part of a Response Chunk message). If False, response will be taken
              as complete and closed off, and any further messages on the channel
              will be ignored. Optional, defaults to False.
*/

class AsgiHttpResponseMsg
{
public:
    int status;
    std::vector<std::tuple<std::string, std::string>> headers;
    std::string content;
    bool more_content;

    MSGPACK_DEFINE_MAP(status, headers, content, more_content);
};
