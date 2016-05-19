#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "WsRequestHandler.h"
#include "WsRequestHandlerSteps.h"

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

    StartReadWritePipelines();

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() being called";
    auto result = m_current_connect_step->OnAsyncCompletion(hr, bytes);
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}

void WsRequestHandler::StartReadWritePipelines()
{
    m_reader.Start();
}
