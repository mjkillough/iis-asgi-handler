#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include <string>
#include <vector>

#include "ResponsePump.h"
#include "RedisChannelLayer.h"
#include "Logger.h"


enum StepResult {
    kStepAsyncPending,
    kStepRerun, // Re-Enter()s the same state.
    kStepGotoNext, // Causes us to call GetNextStep().
    kStepFinishRequest
};


class RequestHandler;


class RequestHandlerStep
{
public:
    RequestHandlerStep(RequestHandler& handler);

    virtual StepResult Enter() = 0;
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes) = 0;
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep() = 0;

protected:
    RequestHandler& m_handler;
    // Expose some of m_handler's protected members here, so that they are
    // accessible from Step subclasses.
    const Logger& logger;
    IHttpContext* m_http_context;
    ResponsePump& m_response_pump;
    RedisChannelLayer& m_channels;
};


class RequestHandler : public IHttpStoredContext
{
    friend class RequestHandlerStep;
public:
    RequestHandler::RequestHandler(
        ResponsePump& response_pump, RedisChannelLayer& channels, const Logger& logger, IHttpContext* http_context
    )
        : m_response_pump(response_pump), m_channels(channels), logger(logger), m_http_context(http_context)
    { }

    virtual void CleanupStoredContext() { delete this; }

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler() = 0;
    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(IHttpCompletionInfo* completion_info) = 0;

protected:
    REQUEST_NOTIFICATION_STATUS HandlerStateMachine(std::unique_ptr<RequestHandlerStep>& step, StepResult result);

    IHttpContext* m_http_context;
    ResponsePump& m_response_pump;
    const Logger& logger;
    RedisChannelLayer& m_channels;

    static std::string GetRequestHttpVersion(const IHttpRequest* request);
    static std::string GetRequestScheme(const HTTP_REQUEST* raw_request);
    static std::string GetRequestPath(const HTTP_REQUEST* raw_request);
    static std::string GetRequestQueryString(const HTTP_REQUEST* raw_request);
    static std::vector<std::tuple<std::string, std::string>> GetRequestHeaders(const HTTP_REQUEST* raw_request);
};
