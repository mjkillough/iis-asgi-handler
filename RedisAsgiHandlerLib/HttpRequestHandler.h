#pragma once

#include "RequestHandler.h"
#include "HttpRequestHandlerSteps.h"


class HttpRequestHandler : public RequestHandler
{
public:
    using RequestHandler::RequestHandler;

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler();
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info);

protected:
    bool ReturnError(USHORT status = 500, const std::string& reason = "");

    std::unique_ptr<RequestHandlerStep> m_current_step;
};
