#pragma once

#include "RequestHandler.h"


class AcceptWebSocketStep : public RequestHandlerStep
{
public:
    AcceptWebSocketStep(RequestHandler& handler)
        : RequestHandlerStep(handler)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();
};


class SendConnectToApplicationStep : public RequestHandlerStep
{
public:
    SendConnectToApplicationStep(RequestHandler& handler)
        : RequestHandlerStep(handler)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();
};


class WsRequestHandler : public RequestHandler
{
public:
    using RequestHandler::RequestHandler;

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler();
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info);

protected:
    // While we're setting up the connection and sending the initial
    // `websocket.connect` ASGI message, we run this pipeline.
    std::unique_ptr<RequestHandlerStep> m_current_connect_step;

    // When we get going, we'll have two pipelines running in parallel.
    // These will receive events from IWebSocketContext callbacks, rather
    // than from the usual IIS OnAsyncCompletion().
    std::unique_ptr<RequestHandlerStep> m_current_read_step;
    std::unique_ptr<RequestHandlerStep> m_current_write_step;
};
