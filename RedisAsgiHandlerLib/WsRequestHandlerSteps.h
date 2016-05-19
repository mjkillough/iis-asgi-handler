#pragma once

#include "RequestHandler.h"
#include "AsgiWsConnectMsg.h"


class AcceptWebSocketStep : public RequestHandlerStep
{
public:
    AcceptWebSocketStep(
        RequestHandler& handler, std::unique_ptr<AsgiWsConnectMsg>& asgi_connect_msg
    ) : RequestHandlerStep(handler), m_asgi_connect_msg(std::move(asgi_connect_msg))
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

private:
    std::unique_ptr<AsgiWsConnectMsg> m_asgi_connect_msg;
};

class SendConnectToApplicationStep : public RequestHandlerStep
{
public:
    SendConnectToApplicationStep(
        RequestHandler& handler, std::unique_ptr<AsgiWsConnectMsg>& asgi_connect_msg
    ) : RequestHandlerStep(handler), m_asgi_connect_msg(std::move(asgi_connect_msg))
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

private:
    std::unique_ptr<AsgiWsConnectMsg> m_asgi_connect_msg;
};
