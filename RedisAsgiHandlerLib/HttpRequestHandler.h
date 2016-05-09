#pragma once

#include "AsgiHttpRequestMsg.h"
#include "AsgiHttpResponseMsg.h"
#include "HttpModuleFactory.h"
#include "ResponsePump.h"
#include "RedisChannelLayer.h"
#include "Logger.h"
#include "HttpRequestHandlerSteps.h"


class HttpRequestHandler : public IHttpStoredContext
{
public:
    HttpRequestHandler(ResponsePump& response_pump, RedisChannelLayer& channels, const Logger& logger, IHttpContext* http_context);

    virtual void CleanupStoredContext()
    {
        delete this;
    }

    virtual REQUEST_NOTIFICATION_STATUS OnAcquireRequestState();
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info);

protected:
    bool ReturnError(USHORT status = 500, const std::string& reason = "");
    REQUEST_NOTIFICATION_STATUS HandlerStateMachine(StepResult result);

    static std::string GetRequestHttpVersion(const IHttpRequest* request);
    static std::string GetRequestScheme(const HTTP_REQUEST* raw_request);
    static std::string GetRequestPath(const HTTP_REQUEST* raw_request);
    static std::string GetRequestQueryString(const HTTP_REQUEST* raw_request);
    static std::vector<std::tuple<std::string, std::string>> GetRequestHeaders(const HTTP_REQUEST* raw_request);

    IHttpContext* m_http_context;
    ResponsePump& m_response_pump;
    const Logger& logger;
    std::unique_ptr<HttpRequestHandlerStep> m_current_step;
    RedisChannelLayer& m_channels;
    friend class HttpRequestHandlerStep;
};
