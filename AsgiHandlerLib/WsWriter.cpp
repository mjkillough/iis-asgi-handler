// WsWriter reads AsgiWsSendMsges from the Channels layer and then writes them
// to the web socket. WsWriter is owned and started by WsRequestHandler. Like WsReader,
// it doesn't receive any callbacks from IIS' normal request pipeline, and instead
// receives callbacks directly fromt he WebSocket module via the IWebSocketContext.

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>
#include <ppltasks.h>

#include <msgpack.hpp>

#include "WsWriter.h"
#include "AsgiWsSendMsg.h"
#include "WsRequestHandler.h"
#include "ResponsePump.h"
#include "Logger.h"


WsWriter::WsWriter(WsRequestHandler & handler)
    : logger(handler.logger), m_response_pump(handler.m_response_pump),
      m_http_context(handler.m_http_context)
{ }


void WsWriter::Start(const std::string & reply_channel)
{
    auto http_context3 = static_cast<IHttpContext3*>(m_http_context);
    m_ws_context = static_cast<IWebSocketContext*>(
        http_context3->GetNamedContextContainer()->GetNamedContext(L"websockets")
    );

    m_reply_channel = reply_channel;

    RegisterChannelsCallback();
}


void WsWriter::RegisterChannelsCallback()
{
    logger.debug() << "RegisterChannelsCallback()";

    m_response_pump.AddChannel(m_reply_channel, [this](std::string data) {
        logger.debug() << "Received AsgiWsSendMsg on channel " << m_reply_channel;
        // This makes me a bit nervous... we're calling WriteAsync() which operates
        // on IWebSocketContext from whichever thread our ResponsePump callback
        // happens to come on. This is unlikely to be on the IIS (or WebSocket module)
        // thread pool and it is difficult to know whether it should be.
        m_asgi_send_msg = std::make_unique<AsgiWsSendMsg>(
            msgpack::unpack(data.data(), data.length()).get().as<AsgiWsSendMsg>()
        );
        WriteAsync();
    });
}

void WsWriter::WriteAsync()
{
    // TODO: Handle m_asgi_send_msg->close.
    logger.debug() << "WriteAsync()";

    BOOL completion_expected = false;
    while (!completion_expected) {
        DWORD num_bytes = m_asgi_send_msg->bytes.size() - m_bytes_written;
        BOOL utf8 = true; // TODO: utf8/bytes mode
        BOOL final_fragment = true;

        HRESULT hr = m_ws_context->WriteFragment(
            m_asgi_send_msg->bytes.data(), &num_bytes, true, utf8, final_fragment,
            WriteCallback, this, &completion_expected
        );
        if (FAILED(hr)) {
            logger.debug() << "WriteFragment() = " << hr;
            // TODO: Call an Error() or something.
            // TODO: Figure out how to close the request from here.
        }

        if (!completion_expected) {
            logger.debug() << "WriteFragment() returned completion_expected=false";
            auto more_to_write = WriteAsyncComplete(S_OK, num_bytes, utf8, final_fragment, false);
            if (!more_to_write) {
                return;
            }
        }
    }

    logger.debug() << "WriteAsync() returning";
}


bool WsWriter::WriteAsyncComplete(HRESULT hr, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close)
{
    logger.debug() << "WriteAsyncComplete() " << num_bytes << "  " << utf8 << " " << final_fragment << " " << close;

    if (FAILED(hr)) {
        logger.debug() << "WriteAsyncComplete() hr = " << hr;
        // TODO: Figure out how to propogate an error from here.
    }
    // TODO: Handle close. Will it be given for the WriteFragment() callback,
    //       given that it isn't in the WriteFragment() signature?

    m_bytes_written += num_bytes;

    // If we're finished, start waiting for the next message.
    if (m_bytes_written >= m_asgi_send_msg->bytes.size()) {
        // Reset ready for the next message. We do this here so that an idle
        // websocket consumes as little memory as possible.
        m_asgi_send_msg.release();
        m_bytes_written = 0;
        RegisterChannelsCallback();
        return false; // No more to write.
    } else {
        return true; // More to write.
    }
}


void WsWriter::WriteCallback(HRESULT hr, VOID * context, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close)
{
    auto writer = static_cast<WsWriter*>(context);
    auto more_to_write = writer->WriteAsyncComplete(hr, num_bytes, utf8, final_fragment, close);
    if (more_to_write) {
        writer->WriteAsync();
    }
}
