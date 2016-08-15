// WsReader reads messages from a WebSocket and sends them to the channels
// layer. WsReader is owned and started by WsRequestHandler, but it does not receive
// it's callbacks from WsRequestHandler or IIS' normal request pipeline. It registers
// callbacks with IIS' WebSocket module via the IWebSocketContext.

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>

#include "WsReader.h"
#include "WsRequestHandler.h"
#include "RedisChannelLayer.h"
#include "Logger.h"


WsReader::WsReader(WsRequestHandler & handler)
    : logger(handler.logger), m_channels(handler.m_channels),
      m_http_context(handler.m_http_context)
{ }


void WsReader::Start(const std::string& reply_channel, const std::string& request_path)
{
    auto http_context3 = static_cast<IHttpContext3*>(m_http_context);
    m_ws_context = static_cast<IWebSocketContext*>(
        http_context3->GetNamedContextContainer()->GetNamedContext(L"websockets")
    );

    m_msg.reply_channel = reply_channel;
    m_msg.path = request_path;

    ReadAsync();
}


void WsReader::ReadAsync()
{
    logger.debug() << "ReadAsync()";

    BOOL completion_expected = false;
    while (!completion_expected) {
        DWORD num_bytes = m_msg.data.size();
        BOOL utf8 = false, final_fragment = false, close = false;

        HRESULT hr = m_ws_context->ReadFragment(
            m_msg.data.data(), &num_bytes, true, &utf8, &final_fragment, &close,
            ReadCallback, this, &completion_expected
            );
        if (FAILED(hr)) {
            logger.debug() << "ReadFragment() = " << hr;
            // TODO: Call an Error() or something.
            // TODO: Figure out how to close the request from here.
        }

        if (!completion_expected) {
            logger.debug() << "ReadFragment() returned completion_expected=false";
            ReadAsyncComplete(S_OK, num_bytes, utf8, final_fragment, close);
        }
    }

    logger.debug() << "ReadAsync() returning";
}


void WsReader::ReadAsyncComplete(HRESULT hr, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close)
{
    logger.debug() << "ReadAsyncComplete() " << num_bytes << "  " << utf8 << " " << final_fragment << " " << close;

    if (FAILED(hr)) {
        logger.debug() << "ReadAsyncComplete() hr = " << hr;
        // TODO: Figure out how to propogate an error from here.
    }
    // TODO: Handle close.

    m_msg.data_size += num_bytes;
    m_msg.utf8 = utf8;

    if (final_fragment) {
        // Don't bother re-sizing the buffer to the correct size before sending.
        // The msgpack serializer will take care of it.
        SendToApplication();
    } else {
        // If we're almost at the end of the buffer, increase the buffer size.
        if (m_msg.data_size >= (m_msg.data.size() - AsgiWsReceiveMsg::BUFFER_CHUNK_INCREASE_THRESHOLD)) {
            m_msg.data.resize(m_msg.data.size() + AsgiWsReceiveMsg::BUFFER_CHUNK_SIZE);
        }
    }
}


void WsReader::SendToApplication()
{
    // Send synchronously for now.
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, m_msg);
    m_channels.Send("websocket.receive", buffer);

    // Reset for the next msg.
    m_msg.data.resize(AsgiWsReceiveMsg::BUFFER_CHUNK_SIZE);
    m_msg.data_size = 0;
    m_msg.order += 1;
}


void WINAPI WsReader::ReadCallback(HRESULT hr, VOID* context, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close)
{
    auto reader = static_cast<WsReader*>(context);
    reader->ReadAsyncComplete(hr, num_bytes, utf8, final_fragment, close);
    reader->ReadAsync();
}
