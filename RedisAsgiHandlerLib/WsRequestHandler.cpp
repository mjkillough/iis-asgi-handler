#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "WsRequestHandler.h"
#include "WsRequestHandlerSteps.h"

#include "ChannelLayer.h"
#include "Logger.h"


REQUEST_NOTIFICATION_STATUS WsRequestHandler::OnExecuteRequestHandler()
{
    auto request = m_http_context->GetRequest();
    auto raw_request = request->GetRawHttpRequest();

    // We need to remember these so that we can pass them to WsReader. IT'll need
    //to send them with every `websocket.receive` message.
    m_reply_channel = m_channels.NewChannel("websocket.send!");
    m_request_path = GetRequestPath(raw_request);

    // The HTTP_COOKED_URL seems to contain the wrong scheme for web sockets.
    auto scheme = GetRequestScheme(raw_request) == "https" ? "wss" : "ws";

    auto asgi_connect_msg = std::make_unique<AsgiWsConnectMsg>();
    asgi_connect_msg->reply_channel = m_reply_channel;
    asgi_connect_msg->scheme = scheme;
    asgi_connect_msg->path = m_request_path;
    asgi_connect_msg->query_string = GetRequestQueryString(raw_request);
    asgi_connect_msg->root_path = ""; // TODO: Same as SCRIPT_NAME in WSGI. What r that?
    asgi_connect_msg->headers = GetRequestHeaders(raw_request);

    m_current_connect_step = std::make_unique<AcceptWebSocketStep>(*this, std::move(asgi_connect_msg));

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->Enter() being called";
    auto result = m_current_connect_step->Enter();
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->Enter() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}

REQUEST_NOTIFICATION_STATUS WsRequestHandler::OnAsyncCompletion(IHttpCompletionInfo* completion_info)
{
    HRESULT hr = completion_info->GetCompletionStatus();
    DWORD bytes = completion_info->GetCompletionBytes();

    StartReaderWriter();

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() being called";
    auto result = m_current_connect_step->OnAsyncCompletion(hr, bytes);
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}

void WsRequestHandler::StartReaderWriter()
{
    m_reader.Start(m_reply_channel, m_request_path);
    m_writer.Start(m_reply_channel);
}
