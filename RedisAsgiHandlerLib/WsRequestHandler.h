#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>

#include "RequestHandler.h"
#include "WsReader.h"


class WsRequestHandler : public RequestHandler
{
    // Having these as a friend saves injecting a bunch of parameters into the them
    // at construction. Their implementation is quite tightly couple to ours anyway.
    friend class WsReader;
    // friend class WsWriter;

public:
    WsRequestHandler(
        ResponsePump& response_pump, RedisChannelLayer& channels, const Logger& logger, IHttpContext* http_context
    )
        : RequestHandler(response_pump, channels, logger, http_context), m_reader(*this)
    { }

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler();
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info);

protected:
    void StartReadWritePipelines();

    // While we're setting up the connection and sending the initial
    // `websocket.connect` ASGI message, we run this pipeline.
    std::unique_ptr<RequestHandlerStep> m_current_connect_step;

    WsReader m_reader;
    std::string m_reply_channel;
    std::string m_request_path;
};
