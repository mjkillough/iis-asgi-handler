#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>

#include "RequestHandler.h"
#include "WsReader.h"
#include "WsWriter.h"


class WsRequestHandler : public RequestHandler
{
    // Having these as a friend saves injecting a bunch of parameters into the them
    // at construction. Their implementation is quite tightly couple to ours anyway.
    // They both have access to the IWebSocketContext* and access it without locking,
    // as we assume it is safe to do so. However, it might be safer to expose
    // IWebSocketContext* via some private member on this class and take a lock before
    // using it? It's difficult to work out what the WebSocket module expects.
    friend class WsReader;
    friend class WsWriter;

public:
    WsRequestHandler(
        ResponsePump& response_pump, RedisChannelLayer& channels, const Logger& logger, IHttpContext* http_context
    )
        : RequestHandler(response_pump, channels, logger, http_context), m_reader(*this), m_writer(*this)
    { }

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler();
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info);

protected:
    void StartReaderWriter();

    // While we're setting up the connection and sending the initial
    // `websocket.connect` ASGI message, we run this pipeline.
    std::unique_ptr<RequestHandlerStep> m_current_connect_step;

    WsReader m_reader;
    WsWriter m_writer;
    std::string m_reply_channel;
    std::string m_request_path;
};
