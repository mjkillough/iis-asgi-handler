#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <ppltasks.h>

#include "WsRequestHandler.h"
#include "AsgiHttpRequestMsg.h"

#include "Logger.h"


REQUEST_NOTIFICATION_STATUS WsRequestHandler::OnExecuteRequestHandler()
{
    m_current_connect_step = std::make_unique<AcceptWebSocketStep>(*this);

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->Enter() being called";
    auto result = m_current_connect_step->Enter();
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->Enter() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}

REQUEST_NOTIFICATION_STATUS WsRequestHandler::OnAsyncCompletion(IHttpCompletionInfo* completion_info)
{
    HRESULT hr = completion_info->GetCompletionStatus();
    DWORD bytes = completion_info->GetCompletionBytes();

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() being called";
    auto result = m_current_connect_step->OnAsyncCompletion(hr, bytes);
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}


StepResult AcceptWebSocketStep::Enter()
{
    auto resp = m_http_context->GetResponse();
    // Setting the status as 101 and Flushing causes the IIS WebSocket module to
    // kick into action. We have to set fMoreData=true in order for it to work.
    resp->SetStatus(101, "");
    resp->Flush(true, true, nullptr, nullptr);
    return kStepAsyncPending;
}

StepResult AcceptWebSocketStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    // Nothing to do but to go to the next step. (We don't know how many bytes
    // were in the response, so num_bytes is meaningless to us).
    // Our caller will handle FAILED(hr).
    return kStepGotoNext;
}

std::unique_ptr<RequestHandlerStep> AcceptWebSocketStep::GetNextStep()
{
    return std::make_unique<SendConnectToApplicationStep>(m_handler);
}


StepResult SendConnectToApplicationStep::Enter()
{
    auto task = concurrency::create_task([this]() {
        AsgiHttpRequestMsg msg;
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, msg);
        m_channels.Send("websocket.connect", buffer);
    }).then([this]() {
        logger.debug() << "SendConnectToApplicationStep calling PostCompletion()";
        m_http_context->PostCompletion(0);
    });

    return kStepAsyncPending;
}

StepResult SendConnectToApplicationStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    if (FAILED(hr)) {
        // TODO: Call an Error() or something.
        return kStepFinishRequest;
    }

    // We return kStepFinishRequest to signal the end of this connection pipeline,
    // but it won't actually complete the request. (We need to send
    // IWebSocketContext->SendConnectionClose() for that).
    return kStepFinishRequest;
}

std::unique_ptr<RequestHandlerStep> SendConnectToApplicationStep::GetNextStep()
{
    throw std::runtime_error("SendConnectToApplicationStep::GetNextStep() should never get called.");
}
