#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModuleFactory.h"
#include "ResponsePump.h"
#include "Logger.h"


class HttpModule : public CHttpModule
{
public:
    HttpModule(
        const HttpModuleFactory& factory, ResponsePump& response_pump,
        const Logger& logger
    );
    ~HttpModule();

    virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler(
        IHttpContext* httpContext, IHttpEventProvider* provider
    );

    virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(
        IHttpContext* http_context,
        DWORD notification,
        BOOL post_notification,
        IHttpEventProvider* provider,
        IHttpCompletionInfo* completion_info
    );

private:
    const HttpModuleFactory& m_factory;
    ResponsePump& m_response_pump;
    RedisChannelLayer m_channels;
    Logger logger;
};
