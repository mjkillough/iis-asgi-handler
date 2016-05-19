#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "AsgiWsReceiveMsg.h"
#include "Logger.h"
#include "RedisChannelLayer.h"


class WsRequestHandler;

class WsReader
{
public:
    WsReader(WsRequestHandler& handler);

    void Start(const std::string& reply_channel, const std::string& request_path);

private:
    void ReadAsync();
    void ReadAsyncComplete(HRESULT hr, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close);
    void SendToApplicationAsync();

    const Logger& logger;
    RedisChannelLayer& m_channels;
    IHttpContext* m_http_context;
    IWebSocketContext* m_ws_context;
    AsgiWsReceiveMsg m_msg;

    static void WINAPI ReadCallback(HRESULT hr, VOID *context, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close);
};
