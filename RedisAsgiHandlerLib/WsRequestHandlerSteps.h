#pragma once

#include "RequestHandler.h"


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
