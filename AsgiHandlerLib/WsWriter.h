#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>

#include "AsgiWsSendMsg.h"
#include "ResponsePump.h"
#include "Logger.h"


class WsRequestHandler;

class WsWriter
{
public:
    WsWriter(WsRequestHandler& handler);

    void Start(const std::string& reply_channel);

private:
    void RegisterChannelsCallback();
    void WriteAsync();
    // Returns whether there's more of the current message to write:
    bool WriteAsyncComplete(HRESULT hr, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close);

    const Logger& logger;
    ResponsePump& m_response_pump;
    IHttpContext* m_http_context;
    IWebSocketContext* m_ws_context;
    std::string m_reply_channel;
    std::unique_ptr<AsgiWsSendMsg> m_asgi_send_msg;
    size_t m_bytes_written{0};

    static void WINAPI WriteCallback(HRESULT hr, VOID *context, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close);
};
