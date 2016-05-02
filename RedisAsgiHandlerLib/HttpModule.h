#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModuleFactory.h"


class HttpModule : public CHttpModule
{
public:
    HttpModule(const HttpModuleFactory& factory);

    virtual REQUEST_NOTIFICATION_STATUS OnAcquireRequestState(
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
};
