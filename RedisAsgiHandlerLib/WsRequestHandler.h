#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>

#include "RequestHandler.h"


class WsRequestHandler;

class AsgiWsReceiveMsg
{
public:
    AsgiWsReceiveMsg()
        : data(BUFFER_CHUNK_SIZE)
    { }

    int order{1};
    std::string reply_channel;
    std::string path;

    // The buffer for holding the data can be up to BUFFER_CHUNK_SIZE bigger than
    // the actual length of the data, as we don't know how big the message is ahead
    // of time. `data_size` contains the true size of the message.
    std::vector<char> data;
    size_t data_size{0};
    bool utf8;

    // Helpers
    template <typename Packer>
    static void pack_string(Packer& packer, const std::string& str)
    {
        packer.pack_str(str.length());
        packer.pack_str_body(str.c_str(), str.length());
    }

    template <typename Packer>
    void msgpack_pack(Packer& packer) const
    {
        packer.pack_map(4);

        pack_string(packer, "reply_channel");
        pack_string(packer, reply_channel);

        pack_string(packer, "path");
        pack_string(packer, path);

        pack_string(packer, "order");
        packer.pack_int(order);

        // TODO: Convert to utf8.
        pack_string(packer, "bytes");
        packer.pack_bin(data_size);
        packer.pack_bin_body(data.data(), data_size);
    }

    // The initial size of the buffer and the size that it will be incremented
    // by each time the current message gets within  BUFFER_CHUNK_INCREASE_THRESHOLD.
    static const std::size_t BUFFER_CHUNK_SIZE = 4096;
    // The amount of freespace that must be remaining in the buffer before the current
    // buffer is expanded.
    static constexpr std::size_t BUFFER_CHUNK_INCREASE_THRESHOLD = BUFFER_CHUNK_SIZE / 4;
};


class WsIoPump
{
public:
    WsIoPump(WsRequestHandler& handler);
    virtual void Start() = 0;
protected:
    const Logger& logger;
    RedisChannelLayer& m_channels;
    IHttpContext* m_http_context;
    IWebSocketContext* m_ws_context;
};

class WsReadPump : public WsIoPump
{
public:
    using WsIoPump::WsIoPump;

    virtual void Start();

private:
    void ReadAsync();
    void ReadAsyncComplete(HRESULT hr, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close);
    void SendToApplicationAsync();

    AsgiWsReceiveMsg m_msg;

    static void WINAPI ReadCallback(HRESULT hr, VOID *context, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close);
};

class WsRequestHandler : public RequestHandler
{
    // This needs to be a friend so that the Read/Write pipelines can access things
    // they probably should have injected into them:
    friend class WsIoPump;

public:
    WsRequestHandler(
        ResponsePump& response_pump, RedisChannelLayer& channels, const Logger& logger, IHttpContext* http_context
    )
        : RequestHandler(response_pump, channels, logger, http_context), m_read_pump(*this)
    { }

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler();
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info);

protected:
    void StartReadWritePipelines();

    // While we're setting up the connection and sending the initial
    // `websocket.connect` ASGI message, we run this pipeline.
    std::unique_ptr<RequestHandlerStep> m_current_connect_step;

    WsReadPump m_read_pump;
};







// Connection Pipeline Steps

class AcceptWebSocketStep : public RequestHandlerStep
{
public:
    using RequestHandlerStep::RequestHandlerStep;

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();
};

class SendConnectToApplicationStep : public RequestHandlerStep
{
public:
    using RequestHandlerStep::RequestHandlerStep;

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();
};
